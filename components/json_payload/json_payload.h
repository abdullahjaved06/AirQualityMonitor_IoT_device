#ifndef JSON_PAYLOAD_H_
#define JSON_PAYLOAD_H_

#include <zephyr/types.h>
#include <stdbool.h>

struct payload {
	struct {
		struct {
			uint32_t sleep_time;
			bool sensor_enable;
		} reported;
	} state;
};

int json_payload_construct(char *message, size_t size, struct payload *payload);

#endif /* JSON_PAYLOAD_H_ */
