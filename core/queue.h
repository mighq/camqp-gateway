#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <glib.h>

// provider interface
#include <api_module_queue.h>

// queue provider functions
gboolean				core_queue_provider_init();
gboolean				core_queue_provider_destroy();

module_vtable_queue*	core_queue_provider();

// common queue functions
queue*		core_queue_new();
void		core_queue_push(queue* object, message* data);
message*	core_queue_pop(queue* object);
gint		core_queue_length(queue* object);
void		core_queue_destroy(queue* object);

// app queues functions
void		core_queues_init();
void		core_queues_destroy();

#endif
