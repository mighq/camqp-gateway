#include <glib.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

// !!!pouzi subsystem console vo win32

#include "api/manager_interface.h"

#include <gmodule.h>
#include <sqlite3.h>

gpointer Func2(gpointer data) {
	g_debug("Func2");

	// attach to queue
	GAsyncQueue* q1 = g_async_queue_ref(data);

	// fill queue
	g_async_queue_push(q1, (gpointer) 123);
	g_async_queue_push(q1, (gpointer) 135);
	g_async_queue_push(q1, (gpointer) 198);

	// free queue ref
	g_async_queue_unref(q1);

	return NULL;
}

gpointer Func1(gpointer data) {
	g_debug("Func1");

	// attach to queue
	GAsyncQueue* q1 = g_async_queue_ref(data);

	// fill queue
	g_async_queue_push(q1, (gpointer) 22);
	g_async_queue_push(q1, (gpointer) 16);
	g_async_queue_push(q1, (gpointer) 99);

	// free queue ref
	g_async_queue_unref(q1);

	return NULL;
}

int main(int argc, const char* argv[]) {
	// check plugin support
	if (!g_module_supported())
		return 1;

	LoadManagerModuleFunc	plugin_load;
	UnloadManagerModuleFunc	plugin_unload;
	GModule*				plugin;

	gchar* program_bin;

#ifdef G_OS_WIN32
	program_bin = g_find_program_in_path(g_get_prgname());
#endif
#ifdef G_OS_UNIX
	program_bin = g_file_read_link("/proc/self/exe", NULL);
	if (program_bin == NULL) {
		g_error("Cannot determine executable path!");
		return 1;
	}
#endif
	g_debug("%s", program_bin);

	gchar* program_dir = g_path_get_dirname(program_bin);
	g_free(program_bin);

	gchar* plugin_dir = g_build_filename(program_dir, "plugins", NULL);

	gchar* plugin_file = g_build_path(".", "in_sample", G_MODULE_SUFFIX, NULL);
	gchar* plugin_path = g_build_filename(plugin_dir, plugin_file, NULL);
	g_free(plugin_file);
	g_free(plugin_dir);

	plugin = g_module_open(plugin_path, G_MODULE_BIND_LAZY);
	g_debug("%s", plugin_path);
	g_free(plugin_path);
	if (!plugin) {
		g_error("%s", g_module_error());
		return 2;
	}

	if (!g_module_symbol(plugin, "LoadManagerModule", (gpointer*) &plugin_load)) {
		g_error("%s", g_module_error());

		if (!g_module_close(plugin))
			g_warning("%s", g_module_error());

		return 3;
	} else {
		if (plugin_load == NULL) {
			g_error("Plugin load symbol is NULL!");

			if (!g_module_close(plugin))
				g_warning("%s", g_module_error());

			return 4;
		}
	}

	if (!g_module_symbol(plugin, "UnloadManagerModule", (gpointer*) &plugin_unload)) {
		g_error("%s", g_module_error());

		if (!g_module_close(plugin))
			g_warning("%s", g_module_error());

		return 5;
	} else {
		if (plugin_unload == NULL) {
			g_error("Plugin unload symbol is NULL!");

			if (!g_module_close(plugin))
				g_warning("%s", g_module_error());

			return 6;
		}
	}

	manager_interface* plugin_input_info = plugin_load();
	if (plugin_input_info != NULL) {
		g_debug(
			"%d.%d %d %d",
				plugin_input_info->version.major,
				plugin_input_info->version.minor,
				plugin_input_info->direction,
				plugin_input_info->transfer_content_type
		);

		plugin_unload();
	} else {
		g_error("Error getting plugin info!");

		if (!g_module_close(plugin))
			g_warning("%s", g_module_error());

		return 7;
	}

	if (!g_module_close(plugin))
		g_warning("%s", g_module_error());

	// init threads
	if (!g_thread_supported())
		g_thread_init(NULL);

	// init queue
	GAsyncQueue* q1 = g_async_queue_new();

	// start & wait for filler
	GThread* t1 = g_thread_create(Func1, q1, TRUE, NULL);
	GThread* t2 = g_thread_create(Func2, q1, TRUE, NULL);
	g_thread_join(t1);
	g_thread_join(t2);

	// print queue contents
	guint x;

	x = (guint) g_async_queue_pop(q1);
	g_debug("%d", x);
	g_debug("%d", g_async_queue_length(q1));

	x = (guint) g_async_queue_pop(q1);
	g_debug("%d", x);
	g_debug("%d", g_async_queue_length(q1));

	x = (guint) g_async_queue_pop(q1);
	g_debug("%d", x);
	g_debug("%d", g_async_queue_length(q1));

	// free queue
	g_async_queue_unref(q1);

	// strings
	GString* str = g_string_new("Piecka");

	g_debug("%d:%s", str->len, str->str);

	g_string_append(str, " Presmahnuta");

	g_debug("%d:%s", str->len, str->str);

	g_string_free(str, TRUE);

	// sqlite
	sqlite3* db;

	gchar* config_path = g_build_filename(program_dir, "config.db", NULL);
	g_free(program_dir);

	gint db_res = sqlite3_open(config_path, &db);
	if (db_res != SQLITE_OK)
		g_debug("cannot open sqlite db!\n");

	g_free(config_path);

	sqlite3_close(db);

	return 0;
}
