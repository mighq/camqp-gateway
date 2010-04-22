#include "messaging.h"

#include "global.h"
#include "module.h"

#include <api_core_options.h>
#include <api_core_config.h>

module_vtable_msg_input* core_messaging_handlers_input() {
	return (module_vtable_msg_input*) g_handlers_input->vtable;
}

module_vtable_msg_output* core_messaging_handlers_output() {
	return (module_vtable_msg_output*) g_handlers_output->vtable;
}

module_vtable_msg_trash* core_messaging_handlers_trash() {
	return (module_vtable_msg_trash*) g_handlers_trash->vtable;
}

gboolean core_messaging_init_module(module_type type, GError** error) {
	*error = NULL;

	// prepare variant options
	gchar*		conf = NULL;
	ModuleFunc	entry = NULL;
	gchar*		func = NULL;
	switch (type) {
		case MODULE_TYPE_MESSAGE_INPUT:
			conf = g_strdup("input_module");
			func = g_strdup("InputModule");
			break;
		case MODULE_TYPE_MESSAGE_OUTPUT:
			conf = g_strdup("output_module");
			func = g_strdup("OutputModule");
			break;
		case MODULE_TYPE_MESSAGE_TRASH:
			conf = g_strdup("trash_module");
			func = g_strdup("TrashModule");
			break;
		default:
			g_set_error(error, 0, 0, "Invalid messaging module type %d!", type);
			return FALSE;
	}

	// determine input module from settings
	if (!core_config_isset("core", conf)) {
		g_set_error(error, 0, 0, "Setting 'core.%s' was not specified for this instance (%d)", conf, core_instance());

		g_free(conf);
		g_free(func);

		return FALSE;
	}

	// load module
	gchar* module_name = core_config_get_text("core", conf);
	module_loaded* module = core_module_load(type, module_name, error);
	g_free(module_name);
	g_free(conf);

	if (module == NULL) {
		g_free(func);
		return FALSE;
	}

	// find xxxModule function
	g_module_symbol(module->module, func, (gpointer*) &entry);
	g_free(func);

	if (entry == NULL)
		return FALSE;

	// get vtable from module
	gpointer ret = (gpointer) entry();
	if (ret == NULL)
		return FALSE;

	// assign vtable to global pointer
	switch (type) {
		case MODULE_TYPE_MESSAGE_INPUT:
			module->vtable = ret;
			g_handlers_input = module;
			break;
		case MODULE_TYPE_MESSAGE_OUTPUT:
			module->vtable = ret;
			g_handlers_output = module;
			break;
		case MODULE_TYPE_MESSAGE_TRASH:
			module->vtable = ret;
			g_handlers_trash = module;
			break;
		default:
			return FALSE;
	}

	// do checking of assigned handlers
	module_vtable_msg_input*	tbl_i = NULL;
	module_vtable_msg_output*	tbl_o = NULL;
	module_vtable_msg_trash*	tbl_t = NULL;

	switch (type) {
		case MODULE_TYPE_MESSAGE_INPUT:
			tbl_i = core_messaging_handlers_input();
			if (tbl_i->handler_receive_feedback == NULL) {
				g_set_error(error, 0, 0, "Handler 'receive_feedback' was not set!");
				return FALSE;
			}
			break;
		case MODULE_TYPE_MESSAGE_OUTPUT:
			tbl_o = core_messaging_handlers_output();
			if (tbl_o->handler_receive_forward == NULL) {
				g_set_error(error, 0, 0, "Handler 'receive_forward' was not set!");
				return FALSE;
			}
			break;
		case MODULE_TYPE_MESSAGE_TRASH:
			tbl_t = core_messaging_handlers_trash();
			if (tbl_t->handler_receive_trash == NULL) {
				g_set_error(error, 0, 0, "Handler 'receive_trash' was not set!");
				return FALSE;
			}
			break;
		default:
			return FALSE;
	}

	return TRUE;
}
