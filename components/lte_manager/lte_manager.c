#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <modem/lte_lc.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include "aws_iot_mqtt.h"
#include "lte_manager.h"

LOG_MODULE_REGISTER(LTE_MANAGER);

bool LTE_CONNECTED = false;
bool REGISTER_NET_MGMT_HANDLERS = false;

// 3GPP eDRX cycle values in seconds and their corresponding 4-bit string
static const struct
{
    int seconds;
    const char *val;
} edrx_table[] = {
    {5, "0000"},    // 5.12 s
    {10, "0001"},   // 10.24 s
    {20, "0010"},   // 20.48 s
    {41, "0011"},   // 40.96 s
    {61, "0100"},   // 61.44 s
    {82, "0101"},   // 81.92 s
    {102, "0110"},  // 102.4 s
    {123, "0111"},  // 122.88 s
    {143, "1000"},  // 143.36 s
    {164, "1001"},  // 163.84 s
    {328, "1010"},  // 327.68 s
    {655, "1011"},  // 655.36 s
    {1311, "1100"}, // 1310.72 s
    {2621, "1101"}, // 2621.44 s
    {5243, "1110"}, // 5242.88 s
    {10486, "1111"} // 10485.76 s
};

static void lte_event_handler(const struct lte_lc_evt *const evt)
{
    switch (evt->type)
    {
    case LTE_LC_EVT_EDRX_UPDATE:
        LOG_INF("eDRX parameter update: eDRX: %f, PTW: %f",
                (double)evt->edrx_cfg.edrx, (double)evt->edrx_cfg.ptw);
        break;
    default:
        break;
    }
}

static const char *closest_edrx_val(int seconds)
{
    int min_diff = abs(seconds - edrx_table[0].seconds);
    int idx = 0;
    for (size_t i = 1; i < sizeof(edrx_table) / sizeof(edrx_table[0]); ++i)
    {
        int diff = abs(seconds - edrx_table[i].seconds);
        if (diff < min_diff)
        {
            min_diff = diff;
            idx = i;
        }
    }
    return edrx_table[idx].val;
}

int device_sleep_request_edrx(int requested_time)
{
    int err;
    int requested_seconds = requested_time * 60;
    const char *edrx_val = closest_edrx_val(requested_seconds);

    err = lte_lc_edrx_param_set(LTE_LC_LTE_MODE_LTEM, edrx_val);
    if (err)
    {
        LOG_ERR("lte_lc_edrx_param_set, error: %d", err);
        return err;
    }
    err = lte_lc_edrx_req(true);
    if (err)
    {
        LOG_ERR("lte_lc_edrx_req, error: %d", err);
        return err;
    }
    LOG_INF("Requested eDRX interval: %d s (mapped to %s)", requested_seconds, edrx_val);
    return 0;
}

int disable_edrx(void)
{
    int err = lte_lc_edrx_req(false);
    if (err)
    {
        LOG_ERR("lte_lc_edrx_req (disable), error: %d", err);
        return err;
    }
    LOG_INF("eDRX disabled");
    return 0;
}

void register_lte_lc_event_handler(void)
{
    lte_lc_register_handler(lte_event_handler);
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
                             uint32_t event,
                             struct net_if *iface)
{
    switch (event)
    {
    case NET_EVENT_L4_CONNECTED:
        LOG_INF("Network connectivity established");

        LTE_CONNECTED = true;
        on_net_event_l4_connected();
        break;
    case NET_EVENT_L4_DISCONNECTED:
        LOG_INF("Network connectivity lost");
        on_net_event_l4_disconnected();
        break;
    default:
        /* Don't care */
        return;
    }
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
                                       uint32_t event,
                                       struct net_if *iface)
{
    if (event == NET_EVENT_CONN_IF_FATAL_ERROR)
    {
        LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
        FATAL_ERROR();
        return;
    }
}

void lte_net_mgmt_register_handlers(void)
{
    /* Zephyr NET management event callback structures. */
    static struct net_mgmt_event_callback l4_cb;
    static struct net_mgmt_event_callback conn_cb;

    /* Setup handler for Zephyr NET Connection Manager events. */
    net_mgmt_init_event_callback(&l4_cb, l4_event_handler, (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED));
    net_mgmt_add_event_callback(&l4_cb);

    /* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
    net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, NET_EVENT_CONN_IF_FATAL_ERROR);
    net_mgmt_add_event_callback(&conn_cb);

    REGISTER_NET_MGMT_HANDLERS = true;
}

int lte_net_mgmt_connect(void)
{
    int err;

    if (!REGISTER_NET_MGMT_HANDLERS)
    {
        lte_net_mgmt_register_handlers();
    }

    /* Connecting to the configured connectivity layer.
     * Wi-Fi or LTE depending on the board that the sample was built for.
     */
    LOG_INF("Bringing network interface up and connecting to the network");

    err = conn_mgr_all_if_up(true);
    if (err)
    {
        LOG_ERR("conn_mgr_all_if_up, error: %d", err);
        return err;
    }

    err = conn_mgr_all_if_connect(true);
    if (err)
    {
        LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
        return err;
    }
    return err;
}

int lte_net_mgmt_disconnect(void)
{
    int err;

    err = conn_mgr_all_if_disconnect(true);
    if (err)
    {
        LOG_ERR("conn_mgr_all_if_disconnect, error: %d", err);
        return err;
    }

    err = conn_mgr_all_if_down(true);
    if (err)
    {
        LOG_ERR("conn_mgr_all_if_down, error: %d", err);
        return err;
    }
    LOG_INF("LTE Powered Off successfully\n\r");
    return err;
}