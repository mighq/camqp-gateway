#include <api_module_msg_output.h>
#include <api_core.h>
#include <api_core_messaging.h>
#include <api_core_options.h>

#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include <camqp.h>

static module_info*					g_module_info;
static module_vtable_msg_output*	g_vtable;

static GCond*						g_cnd_switch;
static GMutex*						g_lck_switch;

static sqlite3*						g_db_o;
static message_batch*				acks;

static camqp_context*				g_context_msg;
static camqp_context*				g_context_sms;


char* dump_data(unsigned char* pointer, unsigned int length)
{
	if (pointer == NULL || length == 0)
		return NULL;

	char* ret = malloc(length*5+1);
	if (!ret)
		return NULL;

	memset(ret, '_', length*5);

	unsigned int i;
	for (i = 0; i < length; i++) {
		memcpy(ret+(5*i), "0x", 2);
		ret[5*i + 2] = "0123456789ABCDEF"[pointer[i] >> 4];
		ret[5*i + 3] = "0123456789ABCDEF"[pointer[i] & 0x0F];
		ret[5*i + 4] = ' ';
	}
	ret[length*5] = 0x00;

	return ret;
}

camqp_char* camqp_data_dump(camqp_data* data) {
	if (data == NULL)
		return NULL;

	return (camqp_char*) dump_data(data->bytes, data->size);
}


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

	// decode
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

	// save uuid & PK
	camqp_element* field;
	field = camqp_composite_field_get(el_req2, (camqp_char*) "id");
	const camqp_uuid* id_obj = camqp_value_uuid(field);
	camqp_uuid request_id;
	uuid_copy(request_id, *id_obj);
	gint32 msg_pk;

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
	field = camqp_composite_field_get(el_req4, (camqp_char*) "sender");
	txt_sender = (gchar*) camqp_value_string(field);
	field = camqp_composite_field_get(el_req4, (camqp_char*) "recipient");
	txt_recipient = (gchar*) camqp_value_string(field);
	field = camqp_composite_field_get(el_req4, (camqp_char*) "text");
	txt_text = (gchar*) camqp_value_string(field);
	field = camqp_composite_field_get(el_req4, (camqp_char*) "id");
	msg_pk = (gint32) camqp_value_uint(field);

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
	gboolean send_success = TRUE;

	// encode result
	camqp_composite* x_res = camqp_composite_new(g_context_sms, (camqp_char*) "sms_response", 0);
	camqp_composite_field_put(x_res, (camqp_char*) "id", (camqp_element*) camqp_scalar_uint(g_context_sms, CAMQP_TYPE_UINT, msg_pk));
	camqp_composite_field_put(x_res, (camqp_char*) "result", (camqp_element*) camqp_scalar_bool(g_context_sms, send_success));
	camqp_data* e_res = camqp_element_encode((camqp_element*) x_res);
	camqp_element_free((camqp_element*) x_res);

	// encode message
	camqp_composite* x_msg = camqp_composite_new(g_context_msg, (camqp_char*) "response", 0);
	camqp_composite_field_put(x_msg, (camqp_char*) "id", (camqp_element*) camqp_scalar_uuid(g_context_msg));
	camqp_scalar* corr = camqp_scalar_uuid(g_context_msg);
	uuid_copy(corr->data.uid, request_id);
	camqp_composite_field_put(x_msg, (camqp_char*) "correlation", (camqp_element*) corr);
	camqp_composite_field_put(x_msg, (camqp_char*) "protocol", (camqp_element*) camqp_scalar_string(g_context_msg, CAMQP_TYPE_STRING, (camqp_char*) "sms-v1.0"));
	camqp_composite_field_put(x_msg, (camqp_char*) "content", (camqp_element*) camqp_scalar_binary(g_context_msg, e_res));
	camqp_data_free(e_res);
	camqp_data* e_msg = camqp_element_encode((camqp_element*) x_msg);
	camqp_element_free((camqp_element*) x_msg);

	message* msg = message_new();
	g_print("generating reply msg [%p]\n", msg);

	g_byte_array_append(msg, (guint8*) e_msg->bytes, e_msg->size);
	camqp_data_free(e_msg);

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

	// -- initialize contexts
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

void  msg_out_sqlite_destroy() {
	sqlite3_close(g_db_o);

	camqp_context_free(g_context_sms);
	camqp_context_free(g_context_msg);
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
