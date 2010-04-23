#include "config.h"
#include "global.h"
#include "module.h"

gboolean core_config_provider_init() {
	GError* err;
	ConfigModuleFunc entry;

	// load config module
	module_loaded* module = core_module_load(MODULE_TYPE_CONFIG, g_hash_table_lookup(g_options, "config"), &err);
	if (module == NULL)
		return FALSE;

	// find ConfigModule
	g_module_symbol(module->module, "ConfigModule", (gpointer*) &entry);
	if (entry == NULL) {
		g_warning("%s", g_module_error());
		return FALSE;
	}

	// get vtable from module
	module->vtable = (module_vtable*) entry();

	// setup module as config_provider
	g_provider_config = module;

	return TRUE;
}

gboolean core_config_provider_destroy() {
	core_module_unload(MODULE_TYPE_CONFIG, g_hash_table_lookup(g_options, "config"));

	return TRUE;
}

module_vtable_config* core_config_provider() {
	return (module_vtable_config*) g_provider_config->vtable;
}


void core_config_preload() {
	module_vtable_config* tbl = core_config_provider();
	if (tbl->preload != NULL)
		return tbl->preload();
}

gboolean core_config_isset(gchar* group, gchar* option) {
	module_vtable_config* tbl = core_config_provider();
	if (tbl->isset != NULL)
		return tbl->isset(group, option);
	else
		g_error("Function 'isset' not defined in config module!");
}

gboolean core_config_get_bool(gchar* group, gchar* option) {
	module_vtable_config* tbl = core_config_provider();
	if (tbl->get_bool != NULL)
		return tbl->get_bool(group, option);
	else
		g_error("Function 'get_bool' not defined in config module!");
}

gint32 core_config_get_int(gchar* group, gchar* option) {
	module_vtable_config* tbl = core_config_provider();
	if (tbl->get_int != NULL)
		return tbl->get_int(group, option);
	else
		g_error("Function 'get_int' not defined in config module!");
}

gfloat core_config_get_real(gchar* group, gchar* option) {
	module_vtable_config* tbl = core_config_provider();
	if (tbl->get_real != NULL)
		return tbl->get_real(group, option);
	else
		g_error("Function 'get_real' not defined in config module!");
}

gchar* core_config_get_text(gchar* group, gchar* option) {
	module_vtable_config* tbl = core_config_provider();
	if (tbl->get_text != NULL)
		return tbl->get_text(group, option);
	else
		g_error("Function 'get_text' not defined in config module!");
}

GByteArray* core_config_get_bin(gchar* group, gchar* option) {
	module_vtable_config* tbl = core_config_provider();
	if (tbl->get_bin != NULL)
		return tbl->get_bin(group, option);
	else
		g_error("Function 'get_bin' not defined in config module!");
}
