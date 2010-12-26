#include <api_module_msg_input.h>
#include <api_core_messaging.h>
#include <api_core_config.h>
#include <api_core_options.h>
#include <api_core.h>
#include <api_core_log.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <smpp34.h>
#include <smpp34_structs.h>
#include <smpp34_params.h>

#include <camqp.h>

#include <errno.h>

static module_info*				g_module_info;
static module_vtable_msg_input*	g_vtable;

static gint						g_socket_server;
static gint						g_socket_client;
static int						g_state;

static camqp_context*			g_context_msg;
static camqp_context*			g_context_sms;

#define RETRY_TIMEOUT			500000 // usec

/// init & destroy
void msg_in_smpp_init() {
	// create socket
	g_socket_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// listening configuration
	gchar* listen_ip =		core_config_get_text("msg_in_smpp", "listen_ip");
	guint32 listen_port =	core_config_get_int("msg_in_smpp", "listen_port");

	// name socket
	struct sockaddr_in address =	{0};
	address.sin_family =			PF_INET;
	address.sin_port =				htons(listen_port);
	address.sin_addr.s_addr =		inet_addr(listen_ip);//htonl(INADDR_ANY);

	{
		gchar* wk = g_strdup_printf("listening: %s:%d", listen_ip, listen_port);
		core_log("net", LOG_NOTICE, 1111, wk);
		g_free(wk);
	}

	g_free(listen_ip);

	// bind socket to address
	bind(g_socket_server, (struct sockaddr*) &address, sizeof(address));

	// listen on socket
	listen(g_socket_server, 1/*max clients in queue*/);

	g_state = 0;

	// -- initialize contexts
	GHashTable* opts = core_options_get();

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

void msg_in_smpp_destroy() {
	close(g_socket_server);

	camqp_context_free(g_context_sms);
	camqp_context_free(g_context_msg);
}
// ---

/// producer_type
module_producer_type msg_in_smpp_producer_type() { return MODULE_PRODUCER_TYPE_PUSH; }
// ---

/// push_forward
/**
 * iteration of thread_forward_push
 * if blocking, you have to check for core_terminated() from time to time
 */
void msg_in_smpp_invoker_push_forward() {
	// do nothing if core have terminated already
	if (core_terminated())
		return;

	// wait for connection
	if (g_state == 0) {
		// init waiting structures
		fd_set	waiting_sockets;
		FD_ZERO(&waiting_sockets);
		FD_SET(g_socket_server, &waiting_sockets);

		// try every 1/4 second to check for connection
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = RETRY_TIMEOUT;

		// check for connection on socket
		int ready = select(g_socket_server + 1, &waiting_sockets, NULL, NULL, &tv);

		// check errors
		if (ready == -1)
			return;

		// do nothing if no connection received (will be repeated)
		if (!FD_ISSET(g_socket_server, &waiting_sockets))
			return;

		// accept connection on socket
		struct sockaddr	address;
		socklen_t		addr_len = sizeof(address);

		g_socket_client = accept(g_socket_server, &address, &addr_len);
		core_log("net", LOG_NOTICE, 1111, "SMPP client connected");

		// connected
		g_state = 1;

		//=== receive bind

		bind_transmitter_t res;
		memset(&res, 0, sizeof(bind_transmitter_t));

		//--
		int ret = 0;
		char local_buffer[1024];
		int  local_buffer_len = 0;
//		char print_buffer[2048];
		uint32_t tempo = 0;
		uint32_t cmd_id = 0;
		//--
		memset(local_buffer, 0, sizeof(local_buffer));
		/* Read from socket (This is a sample, must be more complex) **********/
		ret = recv(g_socket_client, local_buffer, 4, MSG_PEEK);

		if( ret != 4 ){ printf("Error in recv(PEEK)\n"); g_state = 0; return;};
		memcpy(&tempo, local_buffer, sizeof(uint32_t)); /* get lenght PDU */
		local_buffer_len = ntohl( tempo );
		ret = recv(g_socket_client, local_buffer, local_buffer_len, 0); 
		if( ret != local_buffer_len ){
			printf("Error in recv(%d bytes)\n", local_buffer_len);return;};
		/* Print Buffer *******************************************************/
/*		memset(print_buffer, 0, sizeof(print_buffer));
		ret = smpp34_dumpBuf(print_buffer, sizeof(print_buffer),
				local_buffer, local_buffer_len);
		if( ret != 0 ){ printf("Error in smpp34_dumpBuf():%d:\n%s\n",
				smpp34_errno, smpp34_strerror ); return; };
		printf("--\n");
		printf("RECEIVE BUFFER \n%s\n", print_buffer);*/
		/* unpack PDU *********************************************************/
		memcpy(&tempo, local_buffer+4, sizeof(uint32_t)); /* get command_id PDU */
		cmd_id = ntohl( tempo );
		ret = smpp34_unpack(cmd_id, (void*)&res, (uint8_t*) local_buffer, local_buffer_len);
		if( ret != 0){ printf( "Error in smpp34_unpack():%d:%s\n",
				smpp34_errno, smpp34_strerror); return; };
		/* Print PDU **********************************************************/
/*		memset(print_buffer, 0, sizeof(print_buffer));
		ret = smpp34_dumpPdu( res.command_id, print_buffer,
				sizeof(print_buffer), (void*)&res);
		if( ret != 0){ printf("Error in smpp34_dumpPdu():%d:\n%s\n",
				smpp34_errno, smpp34_strerror); return; };
		printf("RECEIVE PDU \n%s\n", print_buffer);*/

		//=== reply bind
		bind_transmitter_resp_t req;
		memset(&req, 0, sizeof(bind_transmitter_resp_t));
		//--
		req.command_length   = 0;
		req.command_id       = BIND_TRANSMITTER_RESP;
		req.command_status   = ESME_ROK;
		req.sequence_number  = res.sequence_number;
		//--
		#include "pack_and_send.inc"
		//--

		// bond
		g_state  = 2;

		core_log("net", LOG_NOTICE, 1111, "SMPP client bond");

		return;
	}

	if (g_state != 2)
		return;

	// get command (deliver & unbind)

	//--
	int ret = 0;
	char local_buffer[1024];
	int  local_buffer_len = 0;
//	char print_buffer[2048];
	uint32_t tempo = 0;
	uint32_t cmd_id = 0;
	//--
	memset(local_buffer, 0, sizeof(local_buffer));
	ret = recv(g_socket_client, local_buffer, 4, MSG_PEEK | MSG_DONTWAIT);
	if (ret == -1 && errno == EAGAIN) {
		usleep(RETRY_TIMEOUT);
		return;
	}

	if( ret != 4 ){ printf("Error in recv(PEEK) %d\n", ret); g_state = 0; return;};

	memcpy(&tempo, local_buffer, sizeof(uint32_t)); /* get lenght PDU */
	local_buffer_len = ntohl( tempo );

	ret = recv(g_socket_client, local_buffer, local_buffer_len, 0);
	if( ret != local_buffer_len ){
		printf("Error in recv(%d bytes)\n", local_buffer_len);return;};

	memcpy(&tempo, local_buffer+4, sizeof(uint32_t)); /* get command_id PDU */
	cmd_id = ntohl( tempo );
	//--

	if (cmd_id == SUBMIT_SM) {
		//=== deliver sms
		submit_sm_t res;
		memset(&res, 0, sizeof(submit_sm_t));

		ret = smpp34_unpack(cmd_id, (void*)&res, (uint8_t*) local_buffer, local_buffer_len);
		if( ret != 0){ printf( "Error in smpp34_unpack():%d:%s\n",
				smpp34_errno, smpp34_strerror); return; };

/*		memset(print_buffer, 0, sizeof(print_buffer));
		ret = smpp34_dumpPdu( res.command_id, print_buffer,
				sizeof(print_buffer), (void*)&res);
		if( ret != 0){ printf("Error in smpp34_dumpPdu():%d:\n%s\n",
				smpp34_errno, smpp34_strerror); return; };
		printf("RECEIVE PDU \n%s\n", print_buffer);*/

		// ===

			// create message
			message* msg = message_new();

//			g_print("generating msg [%p]\n", msg);

			core_log("msg", LOG_INFO, 1111, "new message");

			// encode sms
			camqp_data* encoded_sms;
			camqp_composite* sms = camqp_composite_new(g_context_sms, (camqp_char*) "sms", 0);
			camqp_composite_field_put(sms, (camqp_char*) "id", (camqp_element*) camqp_scalar_uint(g_context_sms, CAMQP_TYPE_UINT, res.sequence_number));
			camqp_composite_field_put(sms, (camqp_char*) "sender", (camqp_element*) camqp_scalar_string(g_context_sms, CAMQP_TYPE_STRING, (camqp_char*) res.source_addr));
			camqp_composite_field_put(sms, (camqp_char*) "recipient", (camqp_element*) camqp_scalar_string(g_context_sms, CAMQP_TYPE_STRING, (camqp_char*) res.destination_addr));
			camqp_composite_field_put(sms, (camqp_char*) "text", (camqp_element*) camqp_scalar_string(g_context_sms, CAMQP_TYPE_STRING, (camqp_char*) res.short_message));
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
/*
			{
				char* wk = camqp_data_dump(encoded_msg);
				puts(wk);
				free(wk);
			}
*/
			// add it to message
			g_byte_array_append(msg, (guint8*) encoded_msg->bytes, encoded_msg->size);
			camqp_data_free(encoded_msg);

			// add to batch
			message_batch* batch = NULL;
			batch = g_slist_append(batch, msg);

			// send batch to core
			core_handler_push_forward(batch);

		//=== send response
		submit_sm_resp_t req;
		memset(&req, 0, sizeof(submit_sm_resp_t));

		req.command_length   = 0;
		req.command_id       = SUBMIT_SM_RESP;
		req.command_status   = ESME_ROK;
		req.sequence_number  = res.sequence_number;
		//--
		#include "pack_and_send.inc"
		//--
	} else if (cmd_id == UNBIND) {
		unbind_t res;
		memset(&res, 0, sizeof(unbind_t));

		ret = smpp34_unpack(cmd_id, (void*)&res, (uint8_t*) local_buffer, local_buffer_len);
		if( ret != 0){ printf( "Error in smpp34_unpack():%d:%s\n",
				smpp34_errno, smpp34_strerror); return; };
/*
		memset(print_buffer, 0, sizeof(print_buffer));
		ret = smpp34_dumpPdu( res.command_id, print_buffer,
				sizeof(print_buffer), (void*)&res);
		if( ret != 0){ printf("Error in smpp34_dumpPdu():%d:\n%s\n",
				smpp34_errno, smpp34_strerror); return; };
		printf("RECEIVE PDU \n%s\n", print_buffer);
*/
		// unbind
		g_state = 0;

		//=== reply bind
		unbind_resp_t req;
		memset(&req, 0, sizeof(unbind_resp_t));
		//--
		req.command_length   = 0;
		req.command_id       = UNBIND_RESP;
		req.command_status   = ESME_ROK;
		req.sequence_number  = res.sequence_number;
		//--
		#include "pack_and_send.inc"
		//--

		close(g_socket_client);

		core_log("net", LOG_NOTICE, 1111, "SMPP client disconnected");
	} else {
		// close connection to client
		close(g_socket_client);
		core_log("net", LOG_NOTICE, 1111, "SMPP client disconnected");
	}
}
// ---

/// receive_feedback
gboolean msg_in_smpp_handler_receive_feedback(const message* const data) {
	// doing nothing with feedback messages
	core_log("msg", LOG_INFO, 1111, "ignoring feedback");

	return TRUE;
}
// ---

/// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_MESSAGE_INPUT;
	g_module_info->name = g_strdup("smpp");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_msg_input, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->producer_type = msg_in_smpp_producer_type;
	g_vtable->invoker_push_forward = msg_in_smpp_invoker_push_forward;
	g_vtable->handler_pull_forward = NULL;
	g_vtable->handler_receive_feedback = msg_in_smpp_handler_receive_feedback;
	g_vtable->terminate_blocking = NULL;

	msg_in_smpp_init();

	return g_module_info;
}

gboolean UnloadModule() {
	msg_in_smpp_destroy();

	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_msg_input* InputModule() {
	return g_vtable;
}
// ---

