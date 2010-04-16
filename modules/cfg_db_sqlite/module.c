#include <api_module_config.h>

static module_info*				g_module_info;
static module_vtable_config*	g_vtable;

// exported functions
void preload() {}

gboolean isset(gchar* group, gchar* option) {
	return FALSE;
}

gboolean get_bool(gchar* group, gchar* option) {
	return FALSE;
}

gint32 get_int(gchar* group, gchar* option) {
	return 35;
}
gfloat get_real(gchar* group, gchar* option) {
	return 3.14;
}
gchar* get_text(gchar* group, gchar* option) {
	return "pela_hopa";
}
GByteArray* get_bin(gchar* group, gchar* option) {
	return NULL;
}

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_CONFIG;
	g_module_info->name = g_strdup("db_sqlite");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_config, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->preload = preload;
	g_vtable->isset = isset;
	g_vtable->get_bool = get_bool;
	g_vtable->get_int = get_int;
	g_vtable->get_real = get_real;
	g_vtable->get_text = get_text;
	g_vtable->get_bin = get_bin;

	return g_module_info;
}

gboolean UnloadModule() {
	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_config* ConfigModule() {
	return g_vtable;
}
