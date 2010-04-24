#include <api_module_msg_input.h>
#include <api_core_messaging.h>
#include <api_core.h>

#include <stdlib.h>

static module_info*				g_module_info;
static module_vtable_msg_input*	g_vtable;

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

	gboolean ret = rand() > (RAND_MAX/2);
	if (ret) {
		// create message
		message* msg = message_new();
		g_byte_array_append(msg, (guint8*) "ABC\x00", 4);

		g_print("generating msg [%p]\n", msg);

		// add to batch
		message_batch* batch = NULL;
		batch = g_slist_append(batch, msg);

		// send batch
		core_handler_push_forward(batch);
	} else {
		g_print("no new messages\n");
	}

	// wait
	g_usleep(3000000);
}

gboolean msg_in_smpp_handler_receive_feedback(const message* const data) {
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

	return g_module_info;
}

gboolean UnloadModule() {
	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_msg_input* InputModule() {
	return g_vtable;
}
