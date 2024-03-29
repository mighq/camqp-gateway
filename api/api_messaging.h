#ifndef _API_MESSAGING_H_
#define _API_MESSAGING_H_

#include <glib.h>

typedef GByteArray	message;
typedef GSList		message_batch;

message* message_new();
void message_free(message* data);

#endif
