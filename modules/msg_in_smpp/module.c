#include <api_module_msg_input.h>
#include <api_core_messaging.h>
#include <api_core.h>

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

#include <errno.h>

static module_info*				g_module_info;
static module_vtable_msg_input*	g_vtable;

static gint						g_socket_server;
static gint						g_socket_client;
static int						g_state;

// init & destroy
void msg_in_smpp_init() {
	// create socket
	g_socket_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// name socket
	struct sockaddr_in address =	{0};
	address.sin_family =			PF_INET;
	address.sin_port =				htons(12345/*PORT*/);
	address.sin_addr.s_addr =		inet_addr("192.168.108.2");//htonl(INADDR_ANY);

	// bind socket to address
	bind(g_socket_server, (struct sockaddr*) &address, sizeof(address));

	// listen on socket
	listen(g_socket_server, 3/*max clients in queue*/);

	g_state = 0;
}

void msg_in_smpp_destroy() {
	close(g_socket_server);
}

// exported function
module_producer_type msg_in_smpp_producer_type() { return MODULE_PRODUCER_TYPE_PUSH; }

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
		tv.tv_usec = 250000;

		// check for connection on socket
		int ready = select(g_socket_server + 1, &waiting_sockets, NULL, NULL, &tv);

		// do nothing if no connection received (will be repeated)
		if (!FD_ISSET(g_socket_server, &waiting_sockets))
			return;

		// accept connection on socket
		struct sockaddr	address;
		socklen_t		addr_len = sizeof(address);

		g_socket_client = accept(g_socket_server, &address, &addr_len);
		g_print("new connection\n");

		// connected
		g_state = 1;

		//=== receive bind

		bind_transmitter_t res;
		memset(&res, 0, sizeof(bind_transmitter_t));

		//---
		int ret = 0;
		char local_buffer[1024];
		int  local_buffer_len = 0;
		char print_buffer[2048];
		uint32_t tempo = 0;
		uint32_t cmd_id = 0;
		//---
		memset(local_buffer, 0, sizeof(local_buffer));
		/* Read from socket (This is a sample, must be more complex) **********/
		ret = recv(g_socket_client, local_buffer, 4, MSG_PEEK); 
		if( ret != 4 ){ printf("Error in recv(PEEK)\n");return;};
		memcpy(&tempo, local_buffer, sizeof(uint32_t)); /* get lenght PDU */
		local_buffer_len = ntohl( tempo );
		ret = recv(g_socket_client, local_buffer, local_buffer_len, 0); 
		if( ret != local_buffer_len ){
			printf("Error in recv(%d bytes)\n", local_buffer_len);return;};
		/* Print Buffer *******************************************************/
		memset(print_buffer, 0, sizeof(print_buffer));
		ret = smpp34_dumpBuf(print_buffer, sizeof(print_buffer), 
				local_buffer, local_buffer_len);
		if( ret != 0 ){ printf("Error in smpp34_dumpBuf():%d:\n%s\n",
				smpp34_errno, smpp34_strerror ); return; };
		printf("-----------------------------------------------------------\n");
		printf("RECEIVE BUFFER \n%s\n", print_buffer);
		/* unpack PDU *********************************************************/
		memcpy(&tempo, local_buffer+4, sizeof(uint32_t)); /* get command_id PDU */
		cmd_id = ntohl( tempo );
		ret = smpp34_unpack(cmd_id, (void*)&res, local_buffer, local_buffer_len);
		if( ret != 0){ printf( "Error in smpp34_unpack():%d:%s\n",
				smpp34_errno, smpp34_strerror); return; };
		/* Print PDU **********************************************************/
		memset(print_buffer, 0, sizeof(print_buffer));
		ret = smpp34_dumpPdu( res.command_id, print_buffer,
				sizeof(print_buffer), (void*)&res);
		if( ret != 0){ printf("Error in smpp34_dumpPdu():%d:\n%s\n",
				smpp34_errno, smpp34_strerror); return; };
		printf("RECEIVE PDU \n%s\n", print_buffer);

		//=== reply bind
		bind_transmitter_resp_t req;
		memset(&req, 0, sizeof(bind_transmitter_resp_t));
		//---
		req.command_length   = 0;
		req.command_id       = BIND_TRANSMITTER_RESP;
		req.command_status   = ESME_ROK;
		req.sequence_number  = res.sequence_number;
		//---
		#include "pack_and_send.inc"
		//---

		// bond
		g_state  = 2;

		return;
	}

	if (g_state != 2)
		return;

	// get command (deliver & unbind)

	//---
	int ret = 0;
	char local_buffer[1024];
	int  local_buffer_len = 0;
	char print_buffer[2048];
	uint32_t tempo = 0;
	uint32_t cmd_id = 0;
	//---
	memset(local_buffer, 0, sizeof(local_buffer));

	ret = recv(g_socket_client, local_buffer, 4, MSG_PEEK);
	if( ret != 4 ){ printf("Error in recv(PEEK) %d\n", ret); return;};

	memcpy(&tempo, local_buffer, sizeof(uint32_t)); /* get lenght PDU */
	local_buffer_len = ntohl( tempo );

	ret = recv(g_socket_client, local_buffer, local_buffer_len, 0);
	if( ret != local_buffer_len ){
		printf("Error in recv(%d bytes)\n", local_buffer_len);return;};

	memcpy(&tempo, local_buffer+4, sizeof(uint32_t)); /* get command_id PDU */
	cmd_id = ntohl( tempo );
	//---

	if (cmd_id == SUBMIT_SM) {
		//=== deliver sms
		submit_sm_t res;
		memset(&res, 0, sizeof(submit_sm_t));

		ret = smpp34_unpack(cmd_id, (void*)&res, local_buffer, local_buffer_len);
		if( ret != 0){ printf( "Error in smpp34_unpack():%d:%s\n",
				smpp34_errno, smpp34_strerror); return; };

		memset(print_buffer, 0, sizeof(print_buffer));
		ret = smpp34_dumpPdu( res.command_id, print_buffer,
				sizeof(print_buffer), (void*)&res);
		if( ret != 0){ printf("Error in smpp34_dumpPdu():%d:\n%s\n",
				smpp34_errno, smpp34_strerror); return; };
		printf("RECEIVE PDU \n%s\n", print_buffer);


		// ===

			// create message
			message* msg = message_new();

			g_print("generating msg [%p]\n", msg);

			guint32 seq = core_sequence_next();
			guint32 tmp = htonl(seq);

			g_byte_array_append(msg, (guint8*) &tmp, 4);
			g_byte_array_append(msg, (guint8*) "\x00", 1);
			g_byte_array_append(msg, (guint8*) res.source_addr, strlen(res.source_addr));
			g_byte_array_append(msg, (guint8*) "\x00", 1);
			g_byte_array_append(msg, (guint8*) res.destination_addr, strlen(res.destination_addr));
			g_byte_array_append(msg, (guint8*) "\x00", 1);
			g_byte_array_append(msg, (guint8*) res.short_message, strlen(res.short_message));
			g_byte_array_append(msg, (guint8*) "\x00", 1);

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
		//---
		#include "pack_and_send.inc"
		//---
	} else if (cmd_id == UNBIND) {
		unbind_t res;
		memset(&res, 0, sizeof(unbind_t));

		ret = smpp34_unpack(cmd_id, (void*)&res, local_buffer, local_buffer_len);
		if( ret != 0){ printf( "Error in smpp34_unpack():%d:%s\n",
				smpp34_errno, smpp34_strerror); return; };

		memset(print_buffer, 0, sizeof(print_buffer));
		ret = smpp34_dumpPdu( res.command_id, print_buffer,
				sizeof(print_buffer), (void*)&res);
		if( ret != 0){ printf("Error in smpp34_dumpPdu():%d:\n%s\n",
				smpp34_errno, smpp34_strerror); return; };
		printf("RECEIVE PDU \n%s\n", print_buffer);

		// unbind
		g_state = 0;

		//=== reply bind
		unbind_resp_t req;
		memset(&req, 0, sizeof(unbind_resp_t));
		//---
		req.command_length   = 0;
		req.command_id       = UNBIND_RESP;
		req.command_status   = ESME_ROK;
		req.sequence_number  = res.sequence_number;
		//---
		#include "pack_and_send.inc"
		//---

		close(g_socket_client);
	} else {
		// close connection to client
		close(g_socket_client);
	}
}

gboolean msg_in_smpp_handler_receive_feedback(const message* const data) {
	g_print("received feedback [%p]\n", data);

	// doing nothing with feedback messages

	return TRUE;
}

// module interface
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
