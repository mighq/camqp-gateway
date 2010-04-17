#include "options.h"

#include "global.h"

#include <gmodule.h>

GHashTable* core_options_get() {
	return g_options;
}

gboolean core_options_environment(GError** error) {
	*error = NULL;

	// check support for plugins
	if (!g_module_supported()) {
		g_set_error(error, 0, 0, "Plugin system not supported!");
		return FALSE;
	}

	// detect & save program executable directory
	gchar* program_bin = NULL;
#ifdef G_OS_UNIX
	program_bin = g_file_read_link("/proc/self/exe", NULL);
	if (program_bin == NULL) {
		g_set_error(error, 0, 0, "Cannot detect program executable!");
		return FALSE;
	}
#else
	#error "TODO: not x-platform"
#endif

	// extract dir name
	gchar* program_dir = g_path_get_dirname(program_bin);
	g_free(program_bin);

	// save to options hash
	g_hash_table_insert(g_options, "program", program_dir);

	return TRUE;
}

gboolean core_options_treat(gint argc, gchar** argv, GError** error) {
	*error = NULL;

	// setup option variables
	gchar* cmd_option_modules = NULL;
	gchar* cmd_option_config = NULL;
	gchar* cmd_option_queue = NULL;

	// setup option entries
	GOptionEntry cmd_options[] =
	{
		{ "modules", 'm', 0, G_OPTION_ARG_STRING, &cmd_option_modules, "Path to modules directory [PROGRAM_BIN/modules]", NULL },
		{ "config", 'c', 0, G_OPTION_ARG_STRING, &cmd_option_config, "Use specified configuration module [sqlite]", NULL },
		{ "queue", 'q', 0, G_OPTION_ARG_STRING, &cmd_option_queue, "Use specified queue module[memory]", NULL },
		{ NULL }
	};

	// parse
	GOptionContext* cli_context;
	cli_context = g_option_context_new(NULL);
	g_option_context_add_main_entries(cli_context, cmd_options, NULL);
	if (!g_option_context_parse(cli_context, &argc, &argv, error)) {
		g_option_context_free(cli_context);
		return FALSE;
	}
	g_option_context_free(cli_context);

	// set defaults
	if (cmd_option_modules == NULL)
		cmd_option_modules = g_strdup("modules");

	if (cmd_option_config == NULL)
		cmd_option_config = g_strdup("sqlite");

	if (cmd_option_queue == NULL)
		cmd_option_queue = g_strdup("memory");

	// treat specified params

	// modules directory
	if (!g_path_is_absolute(cmd_option_modules)) {
		// modules, modify to full path
		gchar* modules_dir = g_build_filename(g_hash_table_lookup(g_options, "program"), cmd_option_modules, NULL);
		g_free(cmd_option_modules);
		cmd_option_modules = modules_dir;
	}

	// save command line options to global hash
	g_hash_table_insert(g_options, "modules",	cmd_option_modules);
	g_hash_table_insert(g_options, "config",	cmd_option_config);
	g_hash_table_insert(g_options, "queue",		cmd_option_queue);

	return TRUE;
}
