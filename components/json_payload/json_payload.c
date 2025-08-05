
#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>

#include "json_payload.h"

/* Register log module */
LOG_MODULE_REGISTER(json_payload);

int json_payload_construct(char *message, size_t size, struct payload *payload)
{
	int err;

	const struct json_obj_descr reported[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "sleep_time",
	                          state.reported.sleep_time, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "sensor_enable",
	                          state.reported.sensor_co2_enable, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "co2_medium_threshold",
	                          state.reported.co2_medium_threshold, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "co2_high_threshold",
	                          state.reported.co2_high_threshold, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "temp_high_threshold",
	                          state.reported.temp_high_threshold, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "temp_low_threshold",
	                          state.reported.temp_low_threshold, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "hum_high_threshold",
	                          state.reported.hum_high_threshold, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "hum_low_threshold",
	                          state.reported.hum_low_threshold, JSON_TOK_NUMBER),
};


	const struct json_obj_descr state[] = {
		JSON_OBJ_DESCR_OBJECT_NAMED(struct payload, "reported",
		                            state.reported, reported),
	};

	const struct json_obj_descr root[] = {
		JSON_OBJ_DESCR_OBJECT(struct payload, state, state),
	};

	err = json_obj_encode_buf(root, ARRAY_SIZE(root), payload, message, size);
	if (err) {
		LOG_ERR("json_obj_encode_buf, error: %d", err);
		return err;
	}

	return 0;
}

