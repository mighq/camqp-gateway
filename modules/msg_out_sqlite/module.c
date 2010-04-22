#include <api_module_msg_output.h>

static module_info*					g_module_info;
static module_vtable_msg_output*	g_vtable;

// exported functions
message_batch* handler_pull_feedback() {
	return NULL;
}

gboolean handler_receive_forward(message* data) {
	return TRUE;
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
	g_vtable->handler_pull_feedback = handler_pull_feedback;
	g_vtable->handler_receive_forward = handler_receive_forward;

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
