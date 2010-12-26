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
		core_log("core", LOG_WARNING, 1111, "Setting for 'core.log_module' is missing!");
		return FALSE;
	}

	// load logger module
	module_loaded* module = core_module_load(MODULE_TYPE_LOG, module_name, &err);
	g_free(module_name);

	if (module == NULL) {
		core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());
		return FALSE;
	}

	// find LogModule
	g_module_symbol(module->module, "LogModule", (gpointer*) &entry);
	if (entry == NULL) {
		core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());
		return FALSE;
	}

	// get vtable from module
	module->vtable = (module_vtable*) entry();

	// do specific things for logging
	module_vtable_log* vtl = (module_vtable_log*) module->vtable;

	// init logging
	gboolean init_ret = vtl->init(&err);
	if (!init_ret) {
		if (err)
			core_log("core", LOG_WARNING, 1111, err->message);

		return FALSE;
	}

	// setup module as log provider
	g_provider_log = module;

	return TRUE;
}

gboolean core_log_provider_destroy() {
	// do specific things for logging
	module_vtable_log* vtl = (module_vtable_log*) g_provider_log->vtable;

	// destroy logging
	vtl->destroy();

	core_module_unload(g_provider_log->info->type, g_provider_log->info->name);

	g_provider_log = NULL;

	return TRUE;
}

module_vtable_log* core_log_provider() {
	return (module_vtable_log*) g_provider_log->vtable;
}

gboolean core_log(gchar* domain, guchar level, guint code, gchar* message) {
	if (g_provider_log != NULL) {
		// log using provider
		return ((module_vtable_log*) g_provider_log->vtable)->log(domain, level, code, message);
	} else {
		// if not provider set yet, use GLib logging
		GTimeVal now;
		g_get_current_time(&now);
		gchar* formatted = g_time_val_to_iso8601(&now);

		g_print("%s: %s\n", formatted, message);

		return TRUE;
	}
}
