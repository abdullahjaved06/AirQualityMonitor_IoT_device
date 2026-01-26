#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>
#include "json_payload.h"

LOG_MODULE_REGISTER(json_payload);

int json_payload_construct(char *message, size_t size, struct payload *payload)
{
	int err;

	const struct json_obj_descr reported_descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "sleep_time",
			state.reported.sleep_time, JSON_TOK_NUMBER),

		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "sensor_enable",
			state.reported.sensor_enable, JSON_TOK_TRUE),
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "lsout_feature",
			state.reported.lsout_enable, JSON_TOK_TRUE),
		JSON_OBJ_DESCR_PRIM_NAMED(struct payload, "poweron_delay",
			state.reported.power_on_delay, JSON_TOK_NUMBER),
	};

	const struct json_obj_descr state_descr[] = {
		JSON_OBJ_DESCR_OBJECT_NAMED(struct payload, "reported",
			state.reported, reported_descr),
	};

	const struct json_obj_descr root_descr[] = {
		JSON_OBJ_DESCR_OBJECT_NAMED(struct payload, "state",
			state, state_descr),
	};

	err = json_obj_encode_buf(root_descr, ARRAY_SIZE(root_descr),
				  payload, message, size);
	if (err) {
		LOG_ERR("json_obj_encode_buf failed, error: %d", err);
		return err;
	}

	return 0;
}
