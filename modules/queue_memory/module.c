#include <api_module_queue.h>

static module_info*			g_module_info;
static module_vtable_queue*	g_vtable;

// exported funcs
queue* queue_memory_new(void) {
	queue* obj = g_new0(queue, 1);
	if (obj == NULL)
		return NULL;

	obj->object = g_async_queue_new();

	return obj;
}

void queue_memory_push(queue* object, message* data) {
	// attach to queue
	GAsyncQueue* q = g_async_queue_ref(object->object);

	// fill queue
	g_async_queue_push(q, (gpointer) data);

	// free queue ref
	g_async_queue_unref(q);
}

message* queue_memory_pop(queue* object) {
	// attach to queue
	GAsyncQueue* q = g_async_queue_ref(object->object);

	message* ret = (message*) g_async_queue_try_pop(q);

	// free queue ref
	g_async_queue_unref(q);

	return ret;
}

gint queue_memory_length(queue* object) {
	// attach to queue
	GAsyncQueue* q = g_async_queue_ref(object->object);

	gint ret = g_async_queue_length(q);

	// free queue ref
	g_async_queue_unref(q);

	return ret;
}

void queue_memory_destroy(queue* object) {
	g_async_queue_unref(object->object);
	g_free(object);
}

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_QUEUE;
	g_module_info->name = g_strdup("memory");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_queue, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->new = queue_memory_new;
	g_vtable->push = queue_memory_push;
	g_vtable->pop = queue_memory_pop;
	g_vtable->length = queue_memory_length;
	g_vtable->destroy = queue_memory_destroy;

	return g_module_info;
}

gboolean UnloadModule() {
	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_queue* QueueModule() {
	return g_vtable;
}
