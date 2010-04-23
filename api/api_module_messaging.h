#ifndef _API_MODULE_MESSAGING_H_
#define _API_MODULE_MESSAGING_H_

#include <api_messaging.h>

typedef enum {
	MODULE_PRODUCER_TYPE_PUSH = 1,
	MODULE_PRODUCER_TYPE_PULL = 2,
	MODULE_PRODUCER_TYPE_BOTH = 3,
} module_producer_type;

typedef struct {
	module_producer_type (*producer_type) ();
} module_vtable_msg;

#endif
