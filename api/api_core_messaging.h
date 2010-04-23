#ifndef _API_CORE_MESSAGING_H_
#define _API_CORE_MESSAGING_H_

#include <api_messaging.h>

#ifndef DLLIMPORT
#define DLLIMPORT extern
#endif

DLLIMPORT void core_handler_push_forward(message_batch* data);
DLLIMPORT void core_handler_push_feedback(message_batch* data);
DLLIMPORT void core_handler_push_trash(message* data);


#endif
