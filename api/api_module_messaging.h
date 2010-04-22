#ifndef _API_MODULE_MESSAGING_H_
#define _API_MODULE_MESSAGING_H_

#include <glib.h>

typedef GByteArray	message;
typedef GSList		message_batch;

typedef enum {
	MODULE_PRODUCER_TYPE_PUSH = 1,
	MODULE_PRODUCER_TYPE_PULL = 2,
	MODULE_PRODUCER_TYPE_BOTH = 3,
} module_producer_type;

#endif
