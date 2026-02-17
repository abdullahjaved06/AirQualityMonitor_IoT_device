#include "npm1300.h"

#include <zephyr/drivers/fuel_gauge.h>


LOG_MODULE_REGISTER(NPM1300);

//  const char *topic=NULL; //TODO: FIX
 const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));


#define FAST_FLASH_MS 100
#define SLOW_FLASH_MS 500
#define PRESS_SHORT_MS 1000
#define PRESS_MEDIUM_MS 5000

static volatile int flash_time_ms = SLOW_FLASH_MS;
volatile bool vbus_connected;

//  Work queue for power events
static struct k_work power_event_work;
 power_event_t pending_power_event = POWER_EVENT_NONE;
static power_event_callback_t power_callback = NULL;

// Work handler (runs in thread context)
static void power_event_work_handler(struct k_work *work)
{
    if (power_callback && pending_power_event != POWER_EVENT_NONE) {
        power_callback(pending_power_event);
        pending_power_event = POWER_EVENT_NONE;
    }
}

// Register callback from main
void npm1300_register_power_callback(power_event_callback_t callback)
{
    power_callback = callback;
    k_work_init(&power_event_work, power_event_work_handler);
}

 void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	static int press_t;
	LOG_INF("in event callback npm1300.");

	if (pins & BIT(NPM1300_EVENT_SHIPHOLD_PRESS))
	{
		press_t = k_uptime_get();
	}

	if (pins & BIT(NPM1300_EVENT_SHIPHOLD_RELEASE))
	{
		press_t = k_uptime_get() - press_t;

		if (press_t < PRESS_SHORT_MS)
		{
			LOG_INF("Short press\n");
			if (!regulator_is_enabled(ldsw2))
			{
				regulator_enable(ldsw2);
			}
			flash_time_ms = FAST_FLASH_MS;
		}
		else if (press_t < PRESS_MEDIUM_MS)
		{
			LOG_INF("Medium press\n");
			if (regulator_is_enabled(ldsw2))
			{
				regulator_disable(ldsw2);
			}
			flash_time_ms = SLOW_FLASH_MS;
		}
		else
		{
			LOG_INF("Long press\n");
			if (vbus_connected)
			{
				LOG_INF("Ship mode entry not possible with USB connected\n");
			}
			else
			{
				regulator_parent_ship_mode(regulators);
			}
		}
	}

	if (pins & BIT(NPM1300_EVENT_VBUS_DETECTED))
	{
		LOG_INF("Vbus connected\n");
		printk("Vbus connected\n");
		printf("Vbus connected\n");


		vbus_connected = true;
		      // Schedule work to notify main
        pending_power_event = POWER_EVENT_USB_CONNECTED;
        k_work_submit(&power_event_work);
	
	}

	if (pins & BIT(NPM1300_EVENT_VBUS_REMOVED))
	{
		printk("Vbus removed\n");
		vbus_connected = false;
		    // Schedule work to notify main
        pending_power_event = POWER_EVENT_USB_DISCONNECTED;
        k_work_submit(&power_event_work);
	}
}

bool configure_events(void)
{
	if (!device_is_ready(pmic))
	{
		LOG_INF("Pmic device not ready.\n");
		return false;
	}

	if (!device_is_ready(regulators))
	{
		LOG_INF("Regulator device not ready.\n");
		return false;
	}

	if (!device_is_ready(ldsw2))
	{
		LOG_INF("Load switch 2 device not ready.\n");
		return false;
	}

	if (!device_is_ready(charger))
	{
		LOG_INF("Charger device not ready.\n");
		return false;
	}
	if (fuel_gauge_init(charger) < 0)
	{
		printk("Could not initialise fuel gauge.\n");
		return 0;
	}

	// 	err = gpio_pin_configure(npm1300_gpio, 1, NPM1300_GPIO_PWRLOSSWARN_ON);
	// if (err) {
	// 	LOG_INF("NPM GPIO PIN CONFIG ERROR gpio_pin_configure=%d\n", err);
	// 	return false;
	// }

	static struct gpio_callback event_cb;

	gpio_init_callback(&event_cb, event_callback,
					   BIT(NPM1300_EVENT_SHIPHOLD_PRESS) | BIT(NPM1300_EVENT_SHIPHOLD_RELEASE) |
						   BIT(NPM1300_EVENT_VBUS_DETECTED) |
						   BIT(NPM1300_EVENT_VBUS_REMOVED));

	mfd_npm1300_add_callback(pmic, &event_cb);

	/* Initialise vbus detection status */
	struct sensor_value val;
	int ret = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);

	if (ret < 0)
	{
		return false;
	}

	vbus_connected = (val.val1 != 0) || (val.val2 != 0);

	return true;
}

void enable_regulator()
{
	if (!configure_events())
	{
		LOG_INF("Error: could not configure events\n");
		return 0;
	}

	LOG_INF("PMIC device ok\n");
	regulator_enable(ldsw2);
	fuel_gauge_update(charger, vbus_connected);
}

void disable_regulator() {
	regulator_disable(ldsw2);
}

/* nPM1300 CHARGER.BCHGCHARGESTATUS register bitmasks */
#define NPM1300_CHG_STATUS_COMPLETE_MASK BIT(1)
#define NPM1300_CHG_STATUS_TRICKLE_MASK BIT(2)
#define NPM1300_CHG_STATUS_CC_MASK BIT(3)
#define NPM1300_CHG_STATUS_CV_MASK BIT(4)

static int64_t ref_time;

static const struct battery_model battery_model = {
#include "battery_model.inc"
};

static int read_sensors(const struct device *charger, float *voltage, float *current, float *temp,
						int32_t *chg_status)
{
	struct sensor_value value;
	int ret;

	ret = sensor_sample_fetch(charger);
	if (ret < 0)
	{
		return ret;
	}

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &value);
	*voltage = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_TEMP, &value);
	*temp = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
	*current = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(charger, SENSOR_CHAN_NPM1300_CHARGER_STATUS, &value);
	*chg_status = value.val1;

	return 0;
}

static int charge_status_inform(int32_t chg_status)
{
	union nrf_fuel_gauge_ext_state_info_data state_info;

	if (chg_status & NPM1300_CHG_STATUS_COMPLETE_MASK)
	{
		printk("Charge complete\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_COMPLETE;
	}
	else if (chg_status & NPM1300_CHG_STATUS_TRICKLE_MASK)
	{
		printk("Trickle charging\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_TRICKLE;
	}
	else if (chg_status & NPM1300_CHG_STATUS_CC_MASK)
	{
		printk("Constant current charging\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CC;
	}
	else if (chg_status & NPM1300_CHG_STATUS_CV_MASK)
	{
		printk("Constant voltage charging\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CV;
	}
	else
	{
		printk("Charger idle\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_IDLE;
	}

	return nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_STATE_CHANGE,
										   &state_info);
}

int fuel_gauge_init(const struct device *charger)
{
	struct sensor_value value;
	struct nrf_fuel_gauge_init_parameters parameters = {
		.model = &battery_model,
		.opt_params = NULL,
		.state = NULL,
	};
	float max_charge_current;
	float term_charge_current;
	int32_t chg_status;
	int ret;

	printk("nRF Fuel Gauge version: %s\n", nrf_fuel_gauge_version);

	ret = read_sensors(charger, &parameters.v0, &parameters.i0, &parameters.t0, &chg_status);
	if (ret < 0)
	{
		return ret;
	}

	/* Store charge nominal and termination current, needed for ttf calculation */
	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, &value);
	max_charge_current = (float)value.val1 + ((float)value.val2 / 1000000);
	term_charge_current = max_charge_current / 10.f;

	ret = nrf_fuel_gauge_init(&parameters, NULL);
	if (ret < 0)
	{
		printk("Error: Could not initialise fuel gauge\n");
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_CURRENT_LIMIT,
										  &(union nrf_fuel_gauge_ext_state_info_data){
											  .charge_current_limit = max_charge_current});
	if (ret < 0)
	{
		printk("Error: Could not set fuel gauge state\n");
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_TERM_CURRENT,
										  &(union nrf_fuel_gauge_ext_state_info_data){
											  .charge_term_current = term_charge_current});
	if (ret < 0)
	{
		printk("Error: Could not set fuel gauge state\n");
		return ret;
	}

	ret = charge_status_inform(chg_status);
	if (ret < 0)
	{
		printk("Error: Could not set fuel gauge state\n");
		return ret;
	}

	ref_time = k_uptime_get();

	return 0;
}

int fuel_gauge_update(const struct device *charger, bool vbus_connected)
{
	static int32_t chg_status_prev;

	float voltage;
	float current;
	float temp;
	float soc;
	float tte;
	float ttf;
	float delta;
	int32_t chg_status;
	int ret;

	ret = read_sensors(charger, &voltage, &current, &temp, &chg_status);
	if (ret < 0)
	{
		printk("Error: Could not read from charger device\n");
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(
		vbus_connected ? NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_CONNECTED
					   : NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_DISCONNECTED,
		NULL);
	if (ret < 0)
	{
		printk("Error: Could not inform of state\n");
		return ret;
	}

	if (chg_status != chg_status_prev)
	{
		chg_status_prev = chg_status;

		ret = charge_status_inform(chg_status);
		if (ret < 0)
		{
			printk("Error: Could not inform of charge status\n");
			return ret;
		}
	}

	delta = (float)k_uptime_delta(&ref_time) / 1000.f;

	soc = nrf_fuel_gauge_process(voltage, current, temp, delta, NULL);
	tte = nrf_fuel_gauge_tte_get();
	ttf = nrf_fuel_gauge_ttf_get();

	printk("V: %.3f, I: %.3f, T: %.2f, ", (double)voltage, (double)current, (double)temp);
	printk("SoC: %.2f, TTE: %.0f, TTF: %.0f\n", (double)soc, (double)tte, (double)ttf);

	return 0;
}

//  function to get SoC percentage
float get_battery_soc(void)
{
    float voltage, current, temp;
    int32_t chg_status;
    
    if (read_sensors(charger, &voltage, &current, &temp, &chg_status) < 0) {
        return -1.0f;
    }
    
    float delta = (float)k_uptime_delta(&ref_time) / 1000.f;
    float soc = nrf_fuel_gauge_process(voltage, current, temp, delta, NULL);
    
    return soc;  // Returns 0.0 to 100.0
}

float get_battery_voltage(void)
{
    float voltage, current, temp;
    int32_t chg_status;
    
    if (read_sensors(charger, &voltage, &current, &temp, &chg_status) < 0) {
        return -1.0f;
    }
    return voltage;
}