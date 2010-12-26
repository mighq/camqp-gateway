#include <api_module_msg_trash.h>
#include <api_core_options.h>
#include <api_core_log.h>

#include <sqlite3.h>
#include <string.h>

static module_info*					g_module_info;
static module_vtable_msg_trash*		g_vtable;

static sqlite3*						g_db_t;

void msg_trash_sqlite_init() {
	GHashTable* opts = core_options_get();

	if (sqlite3_threadsafe() == 0)
		g_warning("ThreadSafe disabled for SQLite build!");
	sqlite3_config(SQLITE_CONFIG_SERIALIZED);

	// create config DB file name
	gchar* db_file_name = g_build_filename(
			(gchar*) g_hash_table_lookup(opts, "program"),
			"dbs",
			"trash.db",
			NULL
	);

	// check if exists
	if (!g_file_test(db_file_name, G_FILE_TEST_IS_REGULAR)) {
		gchar* wk = g_strdup_printf("Trash file '%s' doesn't exist!", db_file_name);
		core_log("msg", LOG_CRIT, 1111, wk);
		g_free(wk);
	}

	//
	gint ret = 0;
	g_db_t = NULL;

	ret = sqlite3_open(db_file_name, &g_db_t);
	if (ret != SQLITE_OK)
		core_log("db", LOG_CRIT, 1111, "Cannot open sqlite db");

	// free file name
	g_free(db_file_name);
}

void  msg_trash_sqlite_destroy() {
	sqlite3_close(g_db_t);
}

// exported functions
gboolean msg_trash_sqlite_handler_receive_trash(const message* const data) {
	core_log("msg", LOG_NOTICE, 1111, "received trash message");

	// current timestamp
	GTimeVal now_t;
	memset(&now_t, 0x00, sizeof(GTimeVal));
	g_get_current_time(&now_t);

	gchar* query = g_strdup_printf(
		"													\
			INSERT INTO \"message_trash\" (					\
				message_trash_pk,							\
				message,									\
				date										\
			) VALUES (										\
				coalesce(									\
					(										\
						SELECT								\
							max(\"message_trash_pk\") + 1	\
						FROM								\
							\"message_trash\"				\
					),										\
					1										\
				),											\
				?,											\
				%d.%06d										\
			)												\
		",
			(int) now_t.tv_sec,
			(int) now_t.tv_usec
	);

	sqlite3_stmt* stmt;

	gint ret;
	ret = sqlite3_prepare_v2(
		g_db_t,
		query,
		strlen(query)+1,
		&stmt,
		NULL
	);
	g_free(query);

	if (ret != SQLITE_OK) {
		gchar* wk = g_strdup_printf("Error '%d' during statement preparation: %s!", ret, sqlite3_errmsg(g_db_t));
		core_log("db", LOG_CRIT, 1111, wk);
		g_free(wk);

		return FALSE;
	}

	sqlite3_bind_blob(stmt,1, data->data, data->len, SQLITE_TRANSIENT);

	ret = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (ret != SQLITE_DONE) {
		gchar* wk = g_strdup_printf("Unexpected result '%d' from DB insert!", ret);
		core_log("db", LOG_CRIT, 1111, wk);
		g_free(wk);

		return FALSE;
	}

	return TRUE;
}

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_MESSAGE_TRASH;
	g_module_info->name = g_strdup("sqlite");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_msg_trash, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->handler_receive_trash = msg_trash_sqlite_handler_receive_trash;

	msg_trash_sqlite_init();

	return g_module_info;
}

gboolean UnloadModule() {
	msg_trash_sqlite_destroy();

	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_msg_trash* TrashModule() {
	return g_vtable;
}
