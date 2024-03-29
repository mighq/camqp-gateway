#include "queue.h"

#include "global.h"
#include "module.h"
#include "log.h"

#include <api_core.h>
#include <api_core_options.h>
#include <api_core_config.h>

//

gboolean core_queue_provider_init(GError** error) {
	*error = NULL;
	QueueModuleFunc entry;

	// determine queue module from settings
	if (!core_config_isset("core", "queue_module")) {
		g_set_error(error, 0, 0, "Setting 'core.queue_module' was not specified for this instance (%d)", core_instance());
		return FALSE;
	}

	gchar* module_name = core_config_get_text("core", "queue_module");

	// load config module
	module_loaded* module = core_module_load(MODULE_TYPE_QUEUE, module_name, error);
	if (module == NULL)
		return FALSE;

	g_free(module_name);

	// find ConfigModule
	g_module_symbol(module->module, "QueueModule", (gpointer*) &entry);
	if (entry == NULL) {
		core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());
		return FALSE;
	}

	// get vtable from module
	module->vtable = (module_vtable*) entry();

	// setup module as queue_provider
	g_provider_queue = module;

	return TRUE;
}

gboolean core_queue_provider_destroy() {
	// will be destroyed with g_modules hash
	return TRUE;
}

module_vtable_queue* core_queue_provider() {
	return (module_vtable_queue*) g_provider_queue->vtable;
}

//

queue* core_queue_new() {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->new != NULL)
		return tbl->new();
	else {
		core_log("core", LOG_CRIT, 1111, "Function 'new' not defined in queue module!");
		return NULL;
	}
}

void core_queue_push(queue* object, message* data) {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->push != NULL)
		return tbl->push(object, data);
	else {
		core_log("core", LOG_CRIT, 1111, "Function 'push' not defined in queue module!");
		return;
	}
}

message* core_queue_pop(queue* object) {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->pop != NULL)
		return tbl->pop(object);
	else {
		core_log("core", LOG_CRIT, 1111, "Function 'pop' not defined in queue module!");
		return NULL;
	}
}

gint core_queue_length(queue* object) {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->length != NULL)
		return tbl->length(object);
	else {
		core_log("core", LOG_CRIT, 1111, "Function 'length' not defined in queue module!");
		return 0;
	}
}

void core_queue_destroy(queue* object) {
	module_vtable_queue* tbl = core_queue_provider();
	if (tbl->destroy != NULL)
		return tbl->destroy(object);
	else {
		core_log("core", LOG_CRIT, 1111, "Function 'destroy' not defined in queue module!");
		return;
	}
}

//

void core_queues_init() {
	g_queue_forward = core_queue_new();
	g_queue_feedback = core_queue_new();
	g_queue_trash = core_queue_new();
}

void core_queues_destroy() {
	core_queue_destroy(g_queue_forward);
	core_queue_destroy(g_queue_feedback);
	core_queue_destroy(g_queue_trash);
}
