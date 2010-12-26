#include "module.h"
#include "log.h"

#include "global.h"

#include <string.h>

gchar* core_module_file(module_type type, const gchar* name) {
	// prefixes for various module types (indexes correspond to "module_type" enum)
	gchar* module_prefixes[6] = {
		"config",
		"log",
		"queue",
		"msg_in",
		"msg_out",
		"msg_trash"
	};

	// build file name
	gchar* plugin_file = g_strdup_printf(
			"%s_%s.%s",
				module_prefixes[type],
				name,
				G_MODULE_SUFFIX
	);
	gchar* plugin_path = g_build_filename(
			(gchar*) g_hash_table_lookup(g_options, "modules"),
			plugin_file,
			NULL
	);
	g_free(plugin_file);

	return plugin_path;
}

module_loaded* core_module_load(module_type type, const gchar* name, GError** error) {
	module_loaded*		module;

	GModule*			plugin;
	LoadModuleFunc		plugin_load;
	UnloadModuleFunc	plugin_unload;

	*error = NULL;

	// get plugin path
	gchar* plugin_path = core_module_file(type, name);

	// try to load module
	plugin = g_module_open(plugin_path, G_MODULE_BIND_LAZY);
	if (!plugin) {
		g_set_error(error, 0, 0, "Cannot open module file '%s'!", plugin_path);
		g_free(plugin_path);
		return NULL;
	}

	// find load
	if (!g_module_symbol(plugin, "LoadModule", (gpointer*) &plugin_load)) {
		g_set_error(error, 0, 0, "%s", g_module_error());

		if (!g_module_close(plugin))
			core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

		return NULL;
	} else {
		if (plugin_load == NULL) {
			g_set_error(error, 0, 0, "Plugin 'LoadModule' symbol is not present!");

			if (!g_module_close(plugin))
				core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

			return NULL;
		}
	}

	// find unload (just to be sure, that exists)
	if (!g_module_symbol(plugin, "UnloadModule", (gpointer*) &plugin_unload)) {
		g_set_error(error, 0, 0, "%s", g_module_error());

		if (!g_module_close(plugin))
			core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

		return NULL;
	} else {
		if (plugin_unload == NULL) {
			g_set_error(error, 0, 0, "Plugin 'UnloadModule' symbol is not present!");

			if (!g_module_close(plugin))
				core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

			return NULL;
		}
	}

	// load info from plugin
	module_info* plugin_info = plugin_load();
	if (plugin_info == NULL) {
		g_set_error(error, 0, 0, "Error getting plugin info!");

		if (!g_module_close(plugin))
			core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

		return NULL;
	}

	// check module
	if (plugin_info->type != type) {
		gchar* wk = g_strdup_printf("Module has different type (%d) than requested (%d)!", plugin_info->type, type);
		core_log("core", LOG_WARNING, 1111, wk);
		g_free(wk);
	}

	if (strcmp(plugin_info->name, name) != 0) {
		gchar* wk = g_strdup_printf("Module '%s' has different name than requested ('%s')!", plugin_info->name, name);
		core_log("core", LOG_WARNING, 1111, wk);
		g_free(wk);
	}

	// save to loaded modules
	module = g_new0(module_loaded, 1);
	module->module = plugin;
	module->info = plugin_info;
	module->vtable = NULL;
	g_hash_table_insert(g_modules, plugin_path, module);

	return module;
}

gboolean core_module_unload(module_type type, const gchar* name) {
	// get loaded module identifier
	gchar* plugin_path = core_module_file(type, name);

	// find it between loaded modules
	GModule* mod = g_hash_table_lookup(g_modules, plugin_path);
	if (mod) {
		// remove it
		g_hash_table_remove(g_modules, plugin_path); // module will be unloaded automatically using "core_module_unload_ptr"
		g_free(plugin_path);
		return TRUE;
	} else {
		return FALSE;
	}
}

void core_module_unload_ptr(module_loaded* data) {
	UnloadModuleFunc	plugin_unload;

	// find unload
	if (!g_module_symbol(data->module, "UnloadModule", (gpointer*) &plugin_unload)) {
		core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

		if (!g_module_close(data->module))
			core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

		return;
	} else {
		if (plugin_unload == NULL) {
			core_log("core", LOG_WARNING, 1111, "Plugin 'UnloadModule' symbol is not present!");

			if (!g_module_close(data->module))
				core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

			return;
		}
	}

	// call plugin unload
	plugin_unload();

	// cleanup plugin reference
	if (!g_module_close(data->module))
		core_log("core", LOG_WARNING, 1111, (gchar*) g_module_error());

	// data->info will be freed in module unload

	// frea module_loaded
	g_free(data);
}
