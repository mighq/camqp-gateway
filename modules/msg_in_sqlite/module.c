#include <api_module_msg_input.h>
#include <api_core_messaging.h>
#include <api_core_options.h>
#include <api_core_log.h>

#include <string.h>
#include <sqlite3.h>

#include <arpa/inet.h> // htonl

#include <camqp.h>

static module_info*					g_module_info;
static module_vtable_msg_input*		g_vtable;

static sqlite3*						g_db_i;

static camqp_context*				g_context_msg;
static camqp_context*				g_context_sms;

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
		gchar* wk = g_strdup_printf("Unexpected result '%d' from DB insert: %s!", ret, sqlite3_errmsg(g_db_i));
		core_log("db", LOG_CRIT, 1111, wk);
		g_free(wk);

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
		gchar* wk = g_strdup_printf("Error '%d' during statement preparation: %s!", ret, sqlite3_errmsg(g_db_i));
		core_log("db", LOG_CRIT, 1111, wk);
		g_free(wk);

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
			"dbs",
			"sms.db",
			NULL
	);

	// check if exists
	if (!g_file_test(db_file_name, G_FILE_TEST_IS_REGULAR)) {
		gchar* wk = g_strdup_printf("SMS database file '%s' doesn't exist!", db_file_name);
		core_log("db", LOG_CRIT, 1111, wk);
		g_free(wk);
	}

	//
	gint ret = 0;
	g_db_i = NULL;

	ret = sqlite3_open(db_file_name, &g_db_i);
	if (ret != SQLITE_OK)
		core_log("db", LOG_CRIT, 1111, "Cannot open sqlite db!");

	// free file name
	g_free(db_file_name);

	// --- initialize contexts
	gchar* proto_1 = g_build_filename(
			(gchar*) g_hash_table_lookup(opts, "program"),
			"protocols",
			"messaging-v1.0.xml",
			NULL
	);
	g_context_msg = camqp_context_new((camqp_char*) "messaging-v1.0", (camqp_char*) proto_1);
	g_free(proto_1);

	gchar* proto_2 = g_build_filename(
			(gchar*) g_hash_table_lookup(opts, "program"),
			"protocols",
			"sms-v1.0.xml",
			NULL
	);
	g_context_sms = camqp_context_new((camqp_char*) "sms-v1.0", (camqp_char*) proto_2);
	g_free(proto_2);
}

void  msg_in_sqlite_destroy() {
	sqlite3_close(g_db_i);
	camqp_context_free(g_context_sms);
	camqp_context_free(g_context_msg);
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
			core_log("msg", LOG_INFO, 1111, "nothing to send in SMS db");
			return NULL;
		} else {
			core_log("db", LOG_CRIT, 1111, "Unexpected result from DB fetch!");
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

			gchar* msg_sender = (gchar*) sqlite3_column_text(st1, 1);
			gchar* msg_recipient = (gchar*) sqlite3_column_text(st1, 2);
			gchar* msg_text = (gchar*) sqlite3_column_text(st1, 3);

			{
				gchar* wk = g_strdup_printf("new SMS to send: %s:%s:%s", msg_sender, msg_recipient, msg_text);
				core_log("msg", LOG_INFO, 1111, wk);
				g_free(wk);
			}

			// encode sms
			camqp_data* encoded_sms;
			camqp_composite* sms = camqp_composite_new(g_context_sms, (camqp_char*) "sms", 0);
			camqp_composite_field_put(sms, (camqp_char*) "id", (camqp_element*) camqp_scalar_uint(g_context_sms, CAMQP_TYPE_UINT, msg_pk));
			camqp_composite_field_put(sms, (camqp_char*) "sender", (camqp_element*) camqp_scalar_string(g_context_sms, CAMQP_TYPE_STRING, (camqp_char*) msg_sender));
			camqp_composite_field_put(sms, (camqp_char*) "recipient", (camqp_element*) camqp_scalar_string(g_context_sms, CAMQP_TYPE_STRING, (camqp_char*) msg_recipient));
			camqp_composite_field_put(sms, (camqp_char*) "text", (camqp_element*) camqp_scalar_string(g_context_sms, CAMQP_TYPE_STRING, (camqp_char*) msg_text));
			encoded_sms = camqp_element_encode((camqp_element*) sms);
			camqp_element_free((camqp_element*) sms);

			// encode message
			camqp_data* encoded_msg;
			camqp_composite* x_msg = camqp_composite_new(g_context_msg, (camqp_char*) "request", 0);
			camqp_composite_field_put(x_msg, (camqp_char*) "id", (camqp_element*) camqp_scalar_uuid(g_context_msg));
			camqp_composite_field_put(x_msg, (camqp_char*) "protocol", (camqp_element*) camqp_scalar_string(g_context_msg, CAMQP_TYPE_STRING, (camqp_char*) "sms-v1.0"));
			camqp_composite_field_put(x_msg, (camqp_char*) "content", (camqp_element*) camqp_scalar_binary(g_context_msg, encoded_sms));
			camqp_data_free(encoded_sms);
			encoded_msg = camqp_element_encode((camqp_element*) x_msg);
			camqp_element_free((camqp_element*) x_msg);

			g_byte_array_append(msg, (guint8*) encoded_msg->bytes, encoded_msg->size);

			camqp_data_free(encoded_msg);

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
	guint32 msg_pk;
	gboolean msg_result;

	// --- parsing
	camqp_data left;
	camqp_data to_parse;
	to_parse.bytes = data->data;
	to_parse.size = data->len;

	camqp_element* el_req1 = camqp_element_decode(g_context_msg, &to_parse, &left);
	if (!el_req1)
		return FALSE;

	if (!camqp_element_is_composite(el_req1)) {
		camqp_element_free((camqp_element*) el_req1);
		return FALSE;
	}

	camqp_composite* el_req2 = (camqp_composite*) el_req1;

	camqp_element* field;

	// save content
	field = camqp_composite_field_get(el_req2, (camqp_char*) "content");
	camqp_data* content_data = camqp_value_binary(field);

	// parse request
	camqp_element* el_req3 = camqp_element_decode(g_context_sms, content_data, &left);
	camqp_element_free((camqp_element*) el_req1);
	if (!el_req3) {
		return FALSE;
	}

	if (!camqp_element_is_composite(el_req3)) {
		camqp_element_free((camqp_element*) el_req3);
		return FALSE;
	}

	camqp_composite* el_req4 = (camqp_composite*) el_req3;

	// get fields
	field = camqp_composite_field_get(el_req4, (camqp_char*) "id");
	msg_pk = (gint32) camqp_value_uint(field);

	field = camqp_composite_field_get(el_req4, (camqp_char*) "result");
	msg_result = (gboolean) camqp_value_bool(field);

	core_log("msg", LOG_INFO, 1111, "received feedback");

	// --- treating

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
