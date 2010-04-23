#include <api_module_msg_output.h>

static module_info*					g_module_info;
static module_vtable_msg_output*	g_vtable;

// exported functions
module_producer_type msg_out_sqlite_producer_type() {
	return MODULE_PRODUCER_TYPE_PUSH;
}

gboolean msg_out_sqlite_handler_receive_forward(message* data) {
	g_print("received out:%p:%d\n", data, data->len);

	g_byte_array_free(data, TRUE);

	return TRUE;
}

void msg_out_sqlite_invoker_push_feedback() {
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

	return g_module_info;
}

gboolean UnloadModule() {
	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_msg_output* OutputModule() {
	return g_vtable;
}
