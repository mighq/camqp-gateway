#include <api_module_config.h>
#include <api_core_options.h>
#include <api_core.h>
#include <api_core_log.h>

#include <string.h>
#include <sqlite3.h>

static module_info*				g_module_info;
static module_vtable_config*	g_vtable;
static sqlite3*					g_db;

//
void config_sqlite_module_init() {
	GHashTable* opts = core_options_get();

	if (sqlite3_threadsafe() == 0)
		g_warning("ThreadSafe disabled for SQLite build!");
	sqlite3_config(SQLITE_CONFIG_SERIALIZED);

	// create config DB file name
	gchar* db_file_name = g_build_filename(
			(gchar*) g_hash_table_lookup(opts, "program"),
			"dbs",
			"config.db",
			NULL
	);

	// check if exists
	if (!g_file_test(db_file_name, G_FILE_TEST_IS_REGULAR)) {
		gchar* wk = g_strdup_printf("Config file '%s' doesn't exist!", db_file_name);
		core_log("core", LOG_WARNING, 1111, wk);
		g_free(wk);
	}

	// 
	gint ret = 0;
	g_db = NULL;

	ret = sqlite3_open(db_file_name, &g_db);
	if (ret != SQLITE_OK) {
		core_log("db", LOG_ERR, 1111, "Cannot open sqlite db");
	}

	// free file name
	g_free(db_file_name);
}

void config_sqlite_module_destroy() {
	sqlite3_close(g_db);
}

// convenience functions
sqlite3_stmt* config_sqlite_prepare_config_query(gchar* column, gchar* group, gchar* option) {
	gchar* query = g_strdup_printf(
		"									\
			SELECT							\
				%s							\
			FROM							\
				setting						\
					JOIN					\
				\"group\" AS g				\
						USING (group_pk)	\
					JOIN					\
				option AS o					\
						USING (option_pk)	\
			WHERE							\
				instance_id = %d			\
					AND						\
				g.name = '%s'				\
					AND						\
				o.name = '%s'				\
					AND						\
				%s IS NOT NULL				\
			ORDER BY						\
				setting_pk ASC				\
			LIMIT							\
				1							\
		",
			column,
			core_instance(),
			group,
			option,
			column
	);

	sqlite3_stmt* stmt;

	gint ret;
	ret = sqlite3_prepare_v2(
		g_db,
		query,
		strlen(query)+1,
		&stmt,
		NULL
	);
	g_free(query);

	if (ret != SQLITE_OK) {
		core_log("db", LOG_WARNING, 1111, "Error during statement preparation");
		return 0;
	}

	ret = sqlite3_step(stmt);
	if (ret != SQLITE_ROW) {
		if (ret == SQLITE_DONE) {
			// no result returned
			return NULL;
		} else {
			core_log("db", LOG_WARNING, 1111, "Unexpected result from DB fetch");
			return NULL;
		}
	}

	return stmt;
}

// exported functions
gboolean config_sqlite_isset(gchar* group, gchar* option) {
	gchar* query = g_strdup_printf(
		"									\
			SELECT							\
				count(*)					\
			FROM							\
				setting						\
					JOIN					\
				\"group\" AS g				\
						USING (group_pk)	\
					JOIN					\
				option AS o					\
						USING (option_pk)	\
			WHERE							\
				instance_id = %d			\
					AND						\
				g.name = '%s'				\
					AND						\
				o.name = '%s'				\
		",
			core_instance(),
			group,
			option
	);

	sqlite3_stmt* stmt;

	gint ret;
	ret = sqlite3_prepare_v2(
		g_db,
		query,
		strlen(query)+1,
		&stmt,
		NULL
	);
	g_free(query);

	if (ret != SQLITE_OK) {
		core_log("db", LOG_WARNING, 1111, "Error during statement preparation");
		return FALSE;
	}

	ret = sqlite3_step(stmt);
	if (ret != SQLITE_ROW) {
		if (ret == SQLITE_DONE) {
			// no result returned
			return FALSE;
		} else {
			core_log("db", LOG_WARNING, 1111, "Unexpected result from DB fetch");
			return FALSE;
		}
	}

	gint data = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);

	return (gboolean) (data > 0);
}

gint config_sqlite_get_int(gchar* group, gchar* option) {
	gint default_value = 0;

	sqlite3_stmt* stmt = config_sqlite_prepare_config_query("data_int", group, option);
	if (stmt == NULL)
		return default_value;

	gint data = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);

	return data;
}

gboolean config_sqlite_get_bool(gchar* group, gchar* option) {
	gint res = config_sqlite_get_int(group, option);

	return (gboolean) (res > 0);
}

gfloat config_sqlite_get_real(gchar* group, gchar* option) {
	gfloat default_value = 0;

	sqlite3_stmt* stmt = config_sqlite_prepare_config_query("data_real", group, option);
	if (stmt == NULL)
		return default_value;

	double data = sqlite3_column_double(stmt, 0);
	sqlite3_finalize(stmt);

	return (gfloat) data;
}

gchar* config_sqlite_get_text(gchar* group, gchar* option) {
	gchar* default_value = "";

	sqlite3_stmt* stmt = config_sqlite_prepare_config_query("data_text", group, option);
	if (stmt == NULL)
		return default_value;

	gchar* data = g_strdup((gchar*)sqlite3_column_text(stmt, 0));
	sqlite3_finalize(stmt);

	return data;
}

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_CONFIG;
	g_module_info->name = g_strdup("sqlite");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_config, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->preload = NULL;
	g_vtable->isset = config_sqlite_isset;
	g_vtable->get_bool = config_sqlite_get_bool;
	g_vtable->get_int = config_sqlite_get_int;
	g_vtable->get_real = config_sqlite_get_real;
	g_vtable->get_text = config_sqlite_get_text;
	g_vtable->get_bin = NULL;

	// initialize module specific things
	config_sqlite_module_init();

	return g_module_info;
}

gboolean UnloadModule() {
	config_sqlite_module_destroy();

	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_config* ConfigModule() {
	return g_vtable;
}
