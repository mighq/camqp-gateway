#include <glib.h>			// GLib framework
#include <glib/gprintf.h>	// UTF-8 printf

#include "global.h"
#include "module.h"
#include "init_destroy.h"
#include "config.h"

gboolean core_treat_cmd_options(gint argc, gchar** argv, GError** error) {
	*error = NULL;

	// setup option variables
	gchar* cmd_option_config = NULL;
	gchar* cmd_option_modules = NULL;

	// setup option entries
	GOptionEntry cmd_options[] =
	{
		{ "config", 'c', 0, G_OPTION_ARG_STRING, &cmd_option_config, "Use specified configuration module [sqlite]", NULL },
		{ "modules", 'm', 0, G_OPTION_ARG_STRING, &cmd_option_modules, "Path to modules directory [PROGRAM_BIN/modules]", NULL },
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
	if (cmd_option_config == NULL)
		cmd_option_config = g_strdup("sqlite");

	if (cmd_option_modules == NULL) {
		cmd_option_modules = g_strdup("modules");
	}

	// treat specified params

	// modules directory
	if (!g_path_is_absolute(cmd_option_modules)) {
		// modules, modify to full path
		gchar* program_bin = NULL;

#ifdef G_OS_UNIX
		program_bin = g_file_read_link("/proc/self/exe", NULL);
		if (program_bin == NULL) {
			g_set_error(error, 0, 0, "Cannot determine executable path!");
			return FALSE;
		}
#else
		#error "TODO: not x-platform"
#endif

		gchar* program_dir = g_path_get_dirname(program_bin);
		g_free(program_bin);

		gchar* modules_dir = g_build_filename(program_dir, cmd_option_modules, NULL);
		g_free(program_dir);
		g_free(cmd_option_modules);
		cmd_option_modules = modules_dir;
	}

	// save command line options to global hash
	g_hash_table_insert(g_options, "config",	cmd_option_config);
	g_hash_table_insert(g_options, "modules",	cmd_option_modules);

	return TRUE;
}

int main(gint argc, gchar* argv[]) {
	GError* err = NULL;

	// check for feature support
	if (!g_module_supported())
		return 1;

	// initialize
	core_init();

	// treat cmd-line options
	if (!core_treat_cmd_options(argc, argv, &err)) {
		g_print("%s\n", err->message);
		g_error_free(err);
		core_destroy();
		return 3;
	}

	// init config
	if (!core_config_init()) {
		g_print("Cannot initialize config module!\n");
		core_destroy();
		return 4;
	}

	// do config preloading
	core_config_preload();

	g_print("%s\n", core_config_get_text("pela", "hopa"));

	// destroy config
	core_config_destroy();

	// do cleanup
	core_destroy();

	return 0;
}
