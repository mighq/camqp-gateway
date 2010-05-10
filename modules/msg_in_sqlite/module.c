#include <api_module_msg_input.h>
#include <api_core_messaging.h>
#include <api_core_options.h>

#include <string.h>
#include <sqlite3.h>

#include <arpa/inet.h> // htonl

static module_info*					g_module_info;
static module_vtable_msg_input*		g_vtable;

static sqlite3*						g_db_i;

// exported functions
module_producer_type msg_in_sqlite_producer_type() { return MODULE_PRODUCER_TYPE_PULL; }

gboolean db_query_empty(gchar* query) {
	gint ret;
	sqlite3_stmt* stmt;

	ret = sqlite3_prepare_v2(
		g_db_i,
		query,
		strlen(query)+1,
		&stmt,
		NULL
	);

	ret = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (ret != SQLITE_DONE) {
		g_warning("Unexpected result '%d' from DB insert: %s!", ret, sqlite3_errmsg(g_db_i));
		return FALSE;
	}

	return TRUE;
}

sqlite3_stmt* db_query_init(gchar* query) {
	gint ret;
	sqlite3_stmt* stmt;

	ret = sqlite3_prepare_v2(
		g_db_i,
		query,
		strlen(query)+1,
		&stmt,
		NULL
	);

	if (ret != SQLITE_OK) {
		g_warning("Error '%d' during statement preparation: %s!", ret, sqlite3_errmsg(g_db_i));
		return NULL;
	}

	return stmt;
}

void msg_in_sqlite_init() {
	if (sqlite3_threadsafe() == 0)
		g_warning("ThreadSafe disabled for SQLite build!");
	sqlite3_config(SQLITE_CONFIG_SERIALIZED);

	GHashTable* opts = core_options_get();

	// create config DB file name
	gchar* db_file_name = g_build_filename(
			(gchar*) g_hash_table_lookup(opts, "program"),
			"sms.db",
			NULL
	);

	// check if exists
	if (!g_file_test(db_file_name, G_FILE_TEST_IS_REGULAR))
		g_error("Trash file '%s' doesn't exist!", db_file_name);

	//
	gint ret = 0;
	g_db_i = NULL;

	ret = sqlite3_open(db_file_name, &g_db_i);
	if (ret != SQLITE_OK)
		g_error("Cannot open sqlite db!\n");

	// free file name
	g_free(db_file_name);
}

void  msg_in_sqlite_destroy() {
	sqlite3_close(g_db_i);
}

//---
message_batch* msg_in_sqlite_handler_pull_forward() {
	gint ret;
	gchar* q = NULL;

	db_query_empty("BEGIN TRANSACTION");

	q = g_strdup(
		"									\
			SELECT							\
				message_pk,					\
				sender,						\
				recipient,					\
				text						\
			FROM							\
				message_outgoing			\
					JOIN					\
				message						\
						USING (message_pk)	\
			WHERE							\
				status = 0					\
					AND						\
				date_sent IS NULL			\
			ORDER BY						\
				message_pk ASC				\
		"
	);
	sqlite3_stmt* st1 = db_query_init(q);
	g_free(q);

	ret = sqlite3_step(st1);

	message_batch* smss = NULL;

	if (ret != SQLITE_ROW) {
		sqlite3_finalize(st1);
		db_query_empty("ROLLBACK");
		if (ret == SQLITE_DONE) {
			// no result returned
			g_print("nothing to send\n");
			return NULL;
		} else {
			g_warning("Unexpected result '%d' from DB fetch!", ret);
			return NULL;
		}
	} else {
		// create sending stmt
		sqlite3_stmt* st2;

		q = g_strdup("UPDATE message_outgoing SET status = 1 WHERE message_pk = ?");
		sqlite3_prepare_v2(
			g_db_i,
			q,
			strlen(q)+1,
			&st2,
			NULL
		);
		g_free(q);

		// foreach row
		while (ret == SQLITE_ROW) {
			// crete new message for sms
			message* msg = message_new();

			// treat current row
			gint32 msg_pk = sqlite3_column_int(st1, 0);
			guint32 tmp = htonl(msg_pk);

			gchar* msg_sender = (gchar*) sqlite3_column_text(st1, 1);
			gchar* msg_recipient = (gchar*) sqlite3_column_text(st1, 2);
			gchar* msg_text = (gchar*) sqlite3_column_text(st1, 3);

			g_print("new sms to send %d:%s:%s:%s\n", msg_pk, msg_sender, msg_recipient, msg_text);

			g_byte_array_append(msg, (guint8*) &tmp,	4);
			g_byte_array_append(msg, (guint8*) "\x00",	1);
			g_byte_array_append(msg, (guint8*) msg_sender, strlen(msg_sender));
			g_byte_array_append(msg, (guint8*) "\x00",	1);
			g_byte_array_append(msg, (guint8*) msg_recipient, strlen(msg_recipient));
			g_byte_array_append(msg, (guint8*) "\x00",	1);
			g_byte_array_append(msg, (guint8*) msg_text, strlen(msg_text));
			g_byte_array_append(msg, (guint8*) "\x00",	1);

			// update entry to "sending" state
			sqlite3_bind_int(st2, 1, msg_pk);
			sqlite3_step(st2);

			// add msg to list
			smss = g_slist_append(smss, msg);

			// next row
			ret = sqlite3_step(st1);
		}

		sqlite3_finalize(st2);

		// expecting this
		g_assert(ret == SQLITE_DONE);

		sqlite3_finalize(st1);
	}

	db_query_empty("COMMIT");

	return smss;
}

gboolean msg_in_sqlite_handler_receive_feedback(const message* const data) {
	g_print("received feedback [%p]\n", data);

	guint32 tmp = *((guint32*) &data->data[0]);
	guint32 msg_pk = ntohl(tmp);
	gboolean msg_result = (data->data[5] != '0');

	g_print("reply pk = %d: %d\n", msg_pk, msg_result);

	// current timestamp
	GTimeVal now_t;
	memset(&now_t, 0x00, sizeof(GTimeVal));
	g_get_current_time(&now_t);

	// mark sms in db
	db_query_empty("BEGIN TRANSACTION");

	// create sending stmt
	sqlite3_stmt* st;
	gchar* q = g_strdup_printf(
		"							\
			UPDATE					\
				message_outgoing	\
			SET						\
				status = %d,		\
				date_sent = %d.%06d	\
			WHERE					\
				message_pk = %d		\
		",
			(msg_result ? 2 : 3),
			(int) now_t.tv_sec, (int) now_t.tv_usec,
			msg_pk
	);
	sqlite3_prepare_v2(
		g_db_i,
		q,
		strlen(q)+1,
		&st,
		NULL
	);
	g_free(q);

	sqlite3_step(st);
	sqlite3_finalize(st);

	db_query_empty("COMMIT");

	return TRUE;
}

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_MESSAGE_INPUT;
	g_module_info->name = g_strdup("sqlite");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_msg_input, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->producer_type = msg_in_sqlite_producer_type;
	g_vtable->invoker_push_forward = NULL;
	g_vtable->handler_pull_forward = msg_in_sqlite_handler_pull_forward;
	g_vtable->handler_receive_feedback = msg_in_sqlite_handler_receive_feedback;
	g_vtable->terminate_blocking = NULL;

	msg_in_sqlite_init();

	return g_module_info;
}

gboolean UnloadModule() {
	msg_in_sqlite_destroy();

	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_msg_input* InputModule() {
	return g_vtable;
}
