#include <api_module_log.h>
#include <api_core_config.h>

#include <stdio.h>
#include <glib.h>

static module_info*			g_module_info;
static module_vtable_log*	g_vtable;
static FILE*				g_log_fp;

gboolean file_log(gchar* domain, guchar level, guint code, gchar* message);

gboolean file_init(GError** error) {
	*error = NULL;

	// get log file name from configuration
	gchar* fn = core_config_get_text("core", "log_filename");
	if (!fn || g_strcmp0(fn, "") == 0) {
		g_free(fn);
		g_set_error(error, 0, 0, "No setting found for 'core.log_filename'!");
		return FALSE;
	}

	g_print("Using log file: %s\n", fn);

	g_log_fp = NULL;

	// TODO: determine if relative path, for relatives use log dir of application

	FILE* fp = fopen(fn, "a");
	if (!fp) {
		g_set_error(error, 0, 0, "Cannot open file for logging '%s'!", fn);
		g_free(fn);
		return FALSE;
	}
	g_free(fn);

	// store file pointer
	g_log_fp = fp;

	return TRUE;
}

gboolean file_destroy() {
	if (g_log_fp) {
		file_log("core", 0, 0, "closing log file");
		fclose(g_log_fp);
	}

	return TRUE;
}

gchar* current_time_string() {
	GTimeVal now;
	g_get_current_time(&now);

	gchar* formatted = g_time_val_to_iso8601(&now);

	return formatted;
}

gboolean file_log(gchar* domain, guchar level, guint code, gchar* message) {
	gchar* now = current_time_string();

	fprintf(
		g_log_fp,
		"%s; fc=%s, lv=%d, id=%d; %s\n",
			now,
			domain,
			level,
			code,
			message
	);
	g_free(now);

	fflush(g_log_fp);

	return TRUE;
}

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_LOG;
	g_module_info->name = g_strdup("file");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_log, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->log = file_log;
	g_vtable->init = file_init;
	g_vtable->destroy = file_destroy;

	return g_module_info;
}

gboolean UnloadModule() {
	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_log* LogModule() {
	return g_vtable;
}
