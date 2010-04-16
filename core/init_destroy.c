#include "init_destroy.h"

#include "global.h"
#include "module.h"

#include <glib.h>
#include <locale.h>	// locales

void core_init() {
	// setup runtime
	setlocale(LC_ALL, "C.UTF-8");

	// allocate global variables
	g_options = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	g_modules = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) core_module_unload_ptr);
}

void core_destroy() {
	// destroy global variables
	g_hash_table_destroy(g_options);
	g_hash_table_destroy(g_modules);
}
