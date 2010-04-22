#include <api_module_msg_trash.h>

static module_info*					g_module_info;
static module_vtable_msg_trash*		g_vtable;

// exported functions
void handler_receive_trash(message* data) {
}

// module interface
module_info* LoadModule() {
	g_module_info = g_try_new0(module_info, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->type = MODULE_TYPE_MESSAGE_TRASH;
	g_module_info->name = g_strdup("sqlite");

	// init vtable for config module
	g_vtable = g_try_new0(module_vtable_msg_trash, 1);
	if (g_vtable == NULL)
		return NULL;

	// fill vtable with implemented functions
	g_vtable->handler_receive_trash = handler_receive_trash;

	return g_module_info;
}

gboolean UnloadModule() {
	g_free(g_module_info->name);
	g_free(g_module_info);
	g_free(g_vtable);

	return TRUE;
}

module_vtable_msg_trash* TrashModule() {
	return g_vtable;
}
