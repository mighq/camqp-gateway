#include <api_module_msg_input.h>
#include <api_core_messaging.h>
#include <api_core.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>

static module_info*				g_module_info;
static module_vtable_msg_input*	g_vtable;

static gint						g_socket_server;

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

	// wait for new connection on socket
	struct sockaddr	address;
	socklen_t		addr_len = sizeof(address);
	gint connection = accept(g_socket_server, &address, &addr_len);

	// do nothing if core have terminated already
	if (core_terminated())
		return;

	g_print("new connection\n");

	// get message from socket
	message* raw = message_new();
	guint buflen = 128;
	guchar* buf = g_try_new0(guchar, buflen);
	guint len = 0;
	do {
		len = recv(connection, buf, buflen, 0);
		if (len > 0)
			g_byte_array_append(raw, (guint8*) buf, len);
		else
			break;
	} while (len == buflen);
	g_free(buf);

	// parse data

	gchar* txt_sender = NULL;
	gchar* txt_recipient = NULL;
	gchar* txt_text = NULL;

	txt_sender = (gchar*) raw->data;

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

	// create message
	message* msg = message_new();

	g_print("generating msg [%p]\n", msg);

	guint32 seq = core_sequence_next();
	guint32 tmp = htonl(seq);

	g_byte_array_append(msg, (guint8*) &tmp, 4);
	g_byte_array_append(msg, (guint8*) "\x00", 1);
	g_byte_array_append(msg, (guint8*) txt_sender, strlen(txt_sender));
	g_byte_array_append(msg, (guint8*) "\x00", 1);
	g_byte_array_append(msg, (guint8*) txt_recipient, strlen(txt_recipient));
	g_byte_array_append(msg, (guint8*) "\x00", 1);
	g_byte_array_append(msg, (guint8*) txt_text, strlen(txt_text));
	g_byte_array_append(msg, (guint8*) "\x00", 1);

	message_free(raw);

	// add to batch
	message_batch* batch = NULL;
	batch = g_slist_append(batch, msg);

	// send batch to core
	core_handler_push_forward(batch);

	// send reply to client
	send(connection, "1\x00", 2, 0);

	// close connection to client
	g_print("closing connection\n");
	close(connection);
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
