#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <glib.h>

// provider interface
#include <api_module_queue.h>

// queue provider functions
module_vtable_queue*	core_queue_provider_init(const gchar* module_name);
gboolean				core_queue_provider_destroy();

module_vtable_queue*	core_queue_provider();

// queue functions
queue*		core_queue_new();
void		core_queue_push(queue* object, GByteArray* data);
GByteArray*	core_queue_pop(queue* object);
gint		core_queue_length(queue* object);
void		core_queue_destroy(queue* object);

#endif
