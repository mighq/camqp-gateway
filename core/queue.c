#include "queue.h"

#include "global.h"
#include "module.h"

//

module_vtable_queue* core_queue_provider_init(const gchar* module_name) {
	GError* err;
	QueueModuleFunc entry;

	// load config module
	module_loaded* module = core_module_load(MODULE_TYPE_QUEUE, module_name, &err);
	if (module == NULL)
		return NULL;

	// find ConfigModule
	g_module_symbol(module->module, "QueueModule", (gpointer*) &entry);
	if (entry == NULL) {
		g_warning("%s", g_module_error());
		return NULL;
	}

	// get vtable from module
	module_vtable_queue* ret = entry();

	// setup vtable as config_provider
	g_provider_queue = ret;

	return ret;
}

gboolean core_queue_provider_destroy() {
	// will be destroyed with g_modules hash
	return TRUE;
}

module_vtable_queue* core_queue_provider() {
	return g_provider_queue;
}

//

queue* core_queue_new() {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->new != NULL)
		return tbl->new();
	else
		g_error("Function 'new' not defined in queue module!");
}

void core_queue_push(queue* object, GByteArray* data) {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->push != NULL)
		return tbl->push(object, data);
	else
		g_error("Function 'push' not defined in queue module!");
}

GByteArray* core_queue_pop(queue* object) {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->pop != NULL)
		return tbl->pop(object);
	else
		g_error("Function 'pop' not defined in queue module!");
}

gint core_queue_length(queue* object) {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->length != NULL)
		return tbl->length(object);
	else
		g_error("Function 'length' not defined in queue module!");
}

void core_queue_destroy(queue* object) {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->destroy != NULL)
		return tbl->destroy(object);
	else
		g_error("Function 'destroy' not defined in queue module!");
}
