#ifndef __DONUT_INTERNAL_H__
#define __DONUT_INTERNAL_H__

/**
 * Copyright 2016 International Business Machines
 * Copyright 2016 Rackspace Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __unused
#  define __unused __attribute__((unused))
#endif

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(a)  (sizeof((a)) / sizeof((a)[0]))
#endif

#ifndef ABS
#  define ABS(a)	 (((a) < 0) ? -(a) : (a))
#endif

#ifndef MAX
#  define MAX(a,b)	({ __typeof__ (a) _a = (a); \
			   __typeof__ (b) _b = (b); \
			   (_a) > (_b) ? (_a) : (_b); })
#endif

#ifndef MIN
#  define MIN(a,b)	({ __typeof__ (a) _a = (a); \
			   __typeof__ (b) _b = (b); \
			   (_a) < (_b) ? (_a) : (_b); })
#endif

#define	CACHELINE_BYTES		128

#define	ACTION_BASE_M	0x10000		/* Base when in Master Mode */
#define	ACTION_BASE_S	0x0F000		/* Base when in Slave Mode */


struct dnut_funcs {
	void * (* card_alloc_dev)(const char *path, uint16_t vendor_id,
		uint16_t device_id);
	int (* attach_action)(void *card, uint32_t action, int flags,
		int timeout_sec);
	int (* detach_action)(void *card);
	int (* mmio_write32)(void *card, uint64_t offset, uint32_t data);
	int (* mmio_read32)(void *card, uint64_t offset, uint32_t *data);
	int (* mmio_write64)(void *card, uint64_t offset, uint64_t data);
	int (* mmio_read64)(void *card, uint64_t offset, uint64_t *data);
	void (* card_free)(void *card);
};

int action_trace_enabled(void);

#define act_trace(fmt, ...) do {					\
		if (action_trace_enabled())				\
			fprintf(stderr, "A " fmt, ## __VA_ARGS__);	\
	} while (0)

/**
 * Register a software version of the FPGA action to enable us
 * simulating high-level behavior of the same and allowing us to
 * implement the host applications even before the real hardware
 * implementation is completely working.
 */
enum dnut_action_state {
	ACTION_IDLE = 0,
	ACTION_RUNNING,
	ACTION_ERROR,
};

struct dnut_action;

typedef int (*action_main_t)(struct dnut_action *action,
			     void *job, unsigned int job_len);

struct dnut_action {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t action_type;

	enum dnut_action_state state;
	void *priv_data;
	uint8_t job[CACHELINE_BYTES];
	uint32_t retc;
	action_main_t main;

	int (* mmio_write32)(void *card, uint64_t offset, uint32_t data);
	int (* mmio_read32)(void *card, uint64_t offset, uint32_t *data);
	int (* mmio_write64)(void *card, uint64_t offset, uint64_t data);
	int (* mmio_read64)(void *card, uint64_t offset, uint64_t *data);

	struct dnut_action *next;
};

int dnut_action_register(struct dnut_action *action);

#ifdef __cplusplus
}
#endif

#endif	/* __DONUT_INTERNAL_H__ */
