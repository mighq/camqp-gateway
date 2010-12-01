#include "log.h"
#include "global.h"
#include "module.h"
#include "config.h"

gboolean core_log_provider_init() {
	GError* err;
	LogModuleFunc entry;

	// determine log module name from config
	gchar* module_name = core_config_get_text("core", "log_module");
	if (!module_name || g_strcmp0("", module_name) == 0) {
		g_free(module_name);
		g_warning("Setting for 'core.log_module' is missing!");
		return FALSE;
	}

	// load logger module
	module_loaded* module = core_module_load(MODULE_TYPE_LOG, module_name, &err);
	g_free(module_name);

	if (module == NULL) {
		g_warning("%s", g_module_error());
		return FALSE;
	}

	// find LogModule
	g_module_symbol(module->module, "LogModule", (gpointer*) &entry);
	if (entry == NULL) {
		g_warning("%s", g_module_error());
		return FALSE;
	}

	// get vtable from module
	module->vtable = (module_vtable*) entry();

	// setup module as log provider
	g_provider_log = module;

	// do specific things for logging
	module_vtable_log* vtl = (module_vtable_log*) module->vtable;

	// init logging
	gboolean init_ret = vtl->init(&err);
	if (!init_ret) {
		if (err)
			g_warning("%s", err->message);

		return FALSE;
	}

	return TRUE;
}

gboolean core_log_provider_destroy() {
	// do specific things for logging
	module_vtable_log* vtl = (module_vtable_log*) g_provider_log->vtable;

	// init logging
	vtl->destroy();

	core_module_unload(g_provider_log->info->type, g_provider_log->info->name);

	return TRUE;
}

module_vtable_log* core_log_provider() {
	return (module_vtable_log*) g_provider_log->vtable;
}

gboolean core_log(gchar* domain, guchar level, guint code, gchar* message) {
	return ((module_vtable_log*) g_provider_log->vtable)->log(domain, level, code, message);
}
