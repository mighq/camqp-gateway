#include <api_module_msg_output.h>
#include <api_core_messaging.h>
#include <api_core_options.h>
#include <api_core.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

#include <camqp.h>

#include <smpp34.h>
#include <smpp34_structs.h>
#include <smpp34_params.h>

static module_info*					g_module_info;
static module_vtable_msg_output*	g_vtable;

static GCond*						g_cnd_switch;
static GMutex*						g_lck_switch;

static message_batch*				acks;

static camqp_context*				g_context_msg;
static camqp_context*				g_context_sms;

guint								g_state;

gint								g_socket;

	extern int  smpp34_errno;
	extern char smpp34_strerror[2048];

void msg_out_smpp_init() {
	GHashTable* opts = core_options_get();

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

	g_state = 0;

	// connect to SMSC
	g_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_socket == -1) {
		g_print("Error in socket()\n");
		return;
	};

	// connection configuration
	gchar* listen_ip =		core_config_get_text("msg_out_smpp", "connect_ip");
	guint32 listen_port =	core_config_get_int("msg_out_smpp", "connect_port");

	struct sockaddr_in address;
	socklen_t addr_len = sizeof(address);
	memset(&address, '\x00', addr_len);

	address.sin_family =		PF_INET;
	address.sin_port =			htons(listen_port);
	address.sin_addr.s_addr =	inet_addr(listen_ip);

	g_free(listen_ip);

	if (connect(g_socket, (struct sockaddr*) &address, addr_len) != 0){
		g_print("Error in connect %d\n", errno);
		perror(NULL);
		return;
	};

	g_state = 1;

	// === bind as transmiter

	//--
	bind_transmitter_t      req;
	bind_transmitter_resp_t res;
	memset(&req, 0, sizeof(bind_transmitter_t));
	memset(&res, 0, sizeof(bind_transmitter_resp_t));
	//--
	req.command_length   = 0;
	req.command_id       = BIND_TRANSMITTER;
	req.command_status   = ESME_ROK;
	req.sequence_number  = 1; // TODO
	strcpy(req.system_id, "pavel");
	strcpy(req.password, "wpsd");
	strcpy(req.system_type, "type01");
	req.interface_version = SMPP_VERSION;
	//--
	int ret = 0;
	char local_buffer[1024];
	int  local_buffer_len = 0;
	char print_buffer[2048];
	uint32_t tempo = 0;
	uint32_t cmd_id = 0;
	//--
#include "pack_and_send.inc"
	//--
#include "recv_and_unpack.inc"
	//--
	destroy_tlv( res.tlv );
	//--
	if( res.command_id != BIND_TRANSMITTER_RESP ||
			res.command_status != ESME_ROK ){
		printf("Error in BIND(BIND_TRANSMITTER)[%d:%d]\n",
				res.command_id, res.command_status);
		return;
	};

	g_state = 2;
}

void msg_out_smpp_destroy() {
	camqp_context_free(g_context_sms);
	camqp_context_free(g_context_msg);

	// === unbind
	if (g_state == 2) {
		//--
		unbind_t      req;
		unbind_resp_t res;
		memset(&req, 0, sizeof(unbind_t));
		memset(&res, 0, sizeof(unbind_resp_t));
		//--
		req.command_length   = 0;
		req.command_id       = UNBIND;
		req.command_status   = ESME_ROK;
		req.sequence_number  = 2; //TODO
		//--
		int ret = 0;
		char local_buffer[1024];
		int  local_buffer_len = 0;
		char print_buffer[2048];
		uint32_t tempo = 0;
		uint32_t cmd_id = 0;
		//--
#include "pack_and_send.inc"
		//--
#include "recv_and_unpack.inc"
		//--
		if( res.command_id != UNBIND_RESP ||
				res.command_status != ESME_ROK ){
			printf("Error in send(UNBIND)[%d:%d]\n",
					res.command_id, res.command_status);
			return;
		};

		g_state = 1;
	}

	if (g_state == 1) {
		// disconnect
		close(g_socket);
		g_state = 0;
	}
}

// exported functions
module_producer_type msg_out_smpp_producer_type() { return MODULE_PRODUCER_TYPE_PUSH; }

void msg_out_smpp_terminate_blocking(thread_type thread) {
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

/**
 * message data can't be changed
 */
gboolean msg_out_smpp_handler_receive_forward(const message* const data) {
	g_print("received out [%p]\n", data);

	gint32 msg_pk;
	gchar* txt_sender = NULL;
	gchar* txt_recipient = NULL;
	gchar* txt_text = NULL;

	// === parse data
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

	// save uuid
	camqp_element* field;
	field = camqp_composite_field_get(el_req2, (camqp_char*) "id");
	camqp_uuid* id_obj = camqp_value_uuid(field);
	camqp_uuid request_id;
	uuid_copy(request_id, *id_obj);

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

	// === send message

	//---
	submit_sm_t      req;
	submit_sm_resp_t res;
	memset(&req, 0, sizeof(submit_sm_t));
	memset(&res, 0, sizeof(submit_sm_resp_t));
	//---
	req.command_length   = 0;
	req.command_id       = SUBMIT_SM;
	req.command_status   = ESME_ROK;
	req.sequence_number  = msg_pk;
	strcpy(req.source_addr, txt_sender);
	strcpy(req.destination_addr, txt_recipient);
	req.sm_length           = strlen(txt_text);
	strcpy(req.short_message, txt_text);
	//---
	int ret = 0;
	char local_buffer[1024];
	int  local_buffer_len = 0;
	char print_buffer[2048];
	uint32_t tempo = 0;
	uint32_t cmd_id = 0;
	//
#include "pack_and_send.inc"
	//---
#include "recv_and_unpack.inc"
	//---
	gboolean send_success = TRUE;
	if (res.command_id != SUBMIT_SM_RESP || res.command_status != ESME_ROK ) {
		send_success = FALSE;
		printf("Error in send(SUBMIT_SM)[%d:%d]\n", res.command_id, res.command_status);
	};

	camqp_element_free((camqp_element*) el_req3);

	// === create reply message

	// encode result
	camqp_composite* x_res = camqp_composite_new(g_context_sms, (camqp_char*) "sms_response", 0);
	camqp_composite_field_put(x_res, (camqp_char*) "id", (camqp_element*) camqp_scalar_uint(g_context_sms, CAMQP_TYPE_UINT, msg_pk));
	camqp_composite_field_put(x_res, (camqp_char*) "result", (camqp_element*) camqp_scalar_bool(g_context_sms, send_success));
	camqp_data* e_res = camqp_element_encode(x_res);
	camqp_element_free(x_res);

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
void msg_out_smpp_invoker_push_feedback() {
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

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_MESSAGE_OUTPUT;
	g_module_info->name = g_strdup("smpp");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_msg_output, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->producer_type = msg_out_smpp_producer_type;
	g_vtable->handler_receive_forward = msg_out_smpp_handler_receive_forward;
	g_vtable->invoker_push_feedback = msg_out_smpp_invoker_push_feedback;
	g_vtable->handler_pull_feedback = NULL;
	g_vtable->terminate_blocking = msg_out_smpp_terminate_blocking;

	// context switching, init condition & register it to core
	g_lck_switch = g_mutex_new();
	g_cnd_switch = g_cond_new();

	acks = NULL;

	msg_out_smpp_init();

	return g_module_info;
}

gboolean UnloadModule() {
	msg_out_smpp_destroy();

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
