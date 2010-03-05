#include <glib.h>
#include <manager_interface.h>

manager_interface* g_module_info;

manager_interface* LoadManagerModule() {
	g_module_info = g_try_new0(manager_interface, 1);
	if (g_module_info == NULL)
		return NULL;

	// fill plugin parameters
	g_module_info->version.major = 0;
	g_module_info->version.minor = 1;
	g_module_info->direction = MANAGER_IFACE_DIRECTION_INPUT;
	g_module_info->transfer_content_type = 1;

	return g_module_info;
}

gboolean UnloadManagerModule() {
	g_free(g_module_info);

	return TRUE;
}
