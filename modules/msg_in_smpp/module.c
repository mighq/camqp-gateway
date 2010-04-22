#include <api_module_msg_input.h>

static module_info*				g_module_info;
static module_vtable_msg_input*	g_vtable;

// exported function
message_batch* handler_pull_forward() {
	return NULL;
}

gboolean handler_receive_feedback(message* data) {
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
	g_vtable->handler_pull_forward = handler_pull_forward;
	g_vtable->handler_receive_feedback = handler_receive_feedback;

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
