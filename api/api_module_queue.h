#ifndef _API_MODULE_QUEUE_H_
#define _API_MODULE_QUEUE_H_

#include <api_module_interface.h>
#include <api_messaging.h>

// queue representation
typedef struct {
	gpointer object;
} queue;

// vtable for config module
typedef struct {
	queue*		(*new)		(void);
	void		(*push)		(queue* object, message* data);
	message*	(*pop)		(queue* object);
	gint		(*length)	(queue* object);
	void		(*destroy)	(queue* object);
} module_vtable_queue;

// function which returns vtable for that module
typedef module_vtable_queue* (*QueueModuleFunc) (void); // QueueModule

#endif
