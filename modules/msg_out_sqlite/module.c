#include <api_module_msg_output.h>
#include <api_core.h>
#include <api_core_messaging.h>
#include <api_core_options.h>

#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

static module_info*					g_module_info;
static module_vtable_msg_output*	g_vtable;

static GCond*						g_cnd_switch;
static GMutex*						g_lck_switch;

static sqlite3*						g_db_o;
static message_batch*				acks;

// exported functions
module_producer_type msg_out_sqlite_producer_type() { return MODULE_PRODUCER_TYPE_PUSH; }

void msg_out_sqlite_terminate_blocking(thread_type thread) {
	switch (thread) {
		case THREAD_TYPE_FEEDBACK_PUSH:
			g_mutex_lock(g_lck_switch);
			g_cond_signal(g_cnd_switch);
			g_mutex_unlock(g_lck_switch);
			break;
		default:
			// do nothing
			break;
	}
}

gboolean db_query_empty(gchar* query) {
	gint ret;
	sqlite3_stmt* stmt;

	ret = sqlite3_prepare_v2(
		g_db_o,
		query,
		strlen(query)+1,
		&stmt,
		NULL
	);

	ret = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (ret != SQLITE_DONE) {
		g_warning("Unexpected result '%d' from DB insert: %s!", ret, sqlite3_errmsg(g_db_o));
		return FALSE;
	}

	return TRUE;
}

gint db_query_int(gchar* query) {
	gint default_value = 0;

	gint ret;
	sqlite3_stmt* stmt;

	ret = sqlite3_prepare_v2(
		g_db_o,
		query,
		strlen(query)+1,
		&stmt,
		NULL
	);

	if (ret != SQLITE_OK) {
		g_warning("Error '%d' during statement preparation!", ret);
		return default_value;
	}

	ret = sqlite3_step(stmt);
	if (ret != SQLITE_ROW) {
		if (ret == SQLITE_DONE) {
			// no result returned
			return default_value;
		} else {
			g_warning("Unexpected result '%d' from DB fetch!", ret);
			return default_value;
		}
	}

	gint data = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);

	return data;
}

/**
 * message data can't be changed
 */
gboolean msg_out_sqlite_handler_receive_forward(const message* const data) {
	g_print("received out [%p]\n", data);

	gchar* txt_sender = NULL;
	gchar* txt_recipient = NULL;
	gchar* txt_text = NULL;

	txt_sender = (gchar*) &data->data[5];

	for (txt_recipient = txt_sender; ; txt_recipient++) {
		if (*txt_recipient == '\x00')
			break;
	}
	txt_recipient++;

	for (txt_text = txt_recipient; ; txt_text++) {
		if (*txt_text == '\x00')
			break;
	}
	txt_text++;

	// current timestamp
	GTimeVal now_t;
	memset(&now_t, 0x00, sizeof(GTimeVal));
	g_get_current_time(&now_t);

	gchar* query;
	db_query_empty("BEGIN TRANSACTION");

	gint new_pk = db_query_int("select coalesce((select max(message_pk)+1 from message), 1)");

	query = g_strdup_printf(
		"							\
			INSERT INTO message (	\
				message_pk,			\
				sender,				\
				recipient,			\
				text				\
			) VALUES (				\
				%d,					\
				'%s',				\
				'%s',				\
				'%s'				\
			)						\
		",
			new_pk,
			txt_sender,
			txt_recipient,
			txt_text
	);
	db_query_empty(query);
	g_free(query);

	query = g_strdup_printf(
		"									\
			INSERT INTO message_incoming (	\
				message_pk,					\
				date_received				\
			) VALUES (						\
				%d,							\
				%d.%06d						\
			)								\
		",
			new_pk,
			(int) now_t.tv_sec,
			(int) now_t.tv_usec
	);
	db_query_empty(query);
	g_free(query);

	db_query_empty("COMMIT");

	// create reply message
	message* msg = message_new();
	g_print("generating reply msg [%p]\n", msg);

	g_byte_array_append(msg, (guint8*) &data->data[0],	4); // sequence no of original
	g_byte_array_append(msg, (guint8*) "\x00",			1);
	g_byte_array_append(msg, (guint8*) "1",				1); // positive ack
	g_byte_array_append(msg, (guint8*) "\x00",			1);

	// add to ack batch
	g_mutex_lock(g_lck_switch);
	acks = g_slist_append(acks, msg);

	// signalize, to send batch of replies
	g_cond_signal(g_cnd_switch);
	g_mutex_unlock(g_lck_switch);

	return TRUE;
}


/**
 * iteration of thread_feedback_push
 * if blocking, you have to check for core_terminated() from time to time
 */
void msg_out_sqlite_invoker_push_feedback() {
	// do nothing if core have terminated already
	if (core_terminated())
		return;

	// wait till something exists
	g_mutex_lock(g_lck_switch);
	g_cond_wait(g_cnd_switch, g_lck_switch);

	// send batch of replies
	if (acks != NULL) {
		g_print("sending replies\n");

		core_handler_push_feedback(acks);

		// setup them to sent
		acks = NULL;
	}

	g_mutex_unlock(g_lck_switch);
}

void msg_out_sqlite_init() {
	GHashTable* opts = core_options_get();

	if (sqlite3_threadsafe() == 0)
		g_warning("ThreadSafe disabled for SQLite build!");
	sqlite3_config(SQLITE_CONFIG_SERIALIZED);

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
	g_db_o = NULL;

	ret = sqlite3_open(db_file_name, &g_db_o);
	if (ret != SQLITE_OK)
		g_error("Cannot open sqlite db!\n");

	// free file name
	g_free(db_file_name);
}

void  msg_out_sqlite_destroy() {
	sqlite3_close(g_db_o);
}

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_MESSAGE_OUTPUT;
	g_module_info->name = g_strdup("sqlite");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_msg_output, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->producer_type = msg_out_sqlite_producer_type;
	g_vtable->handler_receive_forward = msg_out_sqlite_handler_receive_forward;
	g_vtable->invoker_push_feedback = msg_out_sqlite_invoker_push_feedback;
	g_vtable->handler_pull_feedback = NULL;
	g_vtable->terminate_blocking = msg_out_sqlite_terminate_blocking;

	// context switching, init condition & register it to core
	g_lck_switch = g_mutex_new();
	g_cnd_switch = g_cond_new();

	acks = NULL;
	msg_out_sqlite_init();

	return g_module_info;
}

gboolean UnloadModule() {
	msg_out_sqlite_destroy();

	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	g_mutex_free(g_lck_switch);
	g_cond_free(g_cnd_switch);

	return TRUE;
}

module_vtable_msg_output* OutputModule() {
	return g_vtable;
}
