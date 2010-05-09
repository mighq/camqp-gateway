#include <api_module_msg_output.h>
#include <api_core_messaging.h>
#include <api_core.h>
#include <stdlib.h>

static module_info*					g_module_info;
static module_vtable_msg_output*	g_vtable;

static GCond*						g_cnd_switch;
static GMutex*						g_lck_switch;

static message_batch*				acks;

// exported functions
module_producer_type msg_out_smpp_producer_type() { return MODULE_PRODUCER_TYPE_PUSH; }

/**
 * message data can't be changed
 */
gboolean msg_out_smpp_handler_receive_forward(const message* const data) {
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

	// create reply message
	gboolean ret = rand() > (RAND_MAX/2);

	message* msg = message_new();
	g_print("generating reply msg [%p]\n", msg);

	g_byte_array_append(msg, (guint8*) &data->data[0],		4); // sequence no of original
	g_byte_array_append(msg, (guint8*) "\x00",				1);
	g_byte_array_append(msg, (guint8*) (ret ? "1" : "0"),	1); // ack/nack
	g_byte_array_append(msg, (guint8*) "\x00",				1);

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

	// context switching, init condition & register it to core
	g_lck_switch = g_mutex_new();
	g_cnd_switch = g_cond_new();
	core_register_condition(g_cnd_switch, g_lck_switch);

	acks = NULL;

	return g_module_info;
}

gboolean UnloadModule() {
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
