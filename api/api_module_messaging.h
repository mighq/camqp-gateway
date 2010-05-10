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

typedef enum {
	THREAD_TYPE_TRASH_RECV = 0,
	THREAD_TYPE_FORWARD_PUSH,
	THREAD_TYPE_FORWARD_PULL,
	THREAD_TYPE_FORWARD_RECV,
	THREAD_TYPE_FEEDBACK_PUSH,
	THREAD_TYPE_FEEDBACK_PULL,
	THREAD_TYPE_FEEDBACK_RECV
} thread_type;

#endif
