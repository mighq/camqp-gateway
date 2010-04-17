#include <glib.h>

#include <api_module_config.h>
#include <api_module_queue.h>

guint					g_instance;

GHashTable*				g_options;
GHashTable*				g_modules;

module_vtable_config*	g_provider_config;
module_vtable_queue*	g_provider_queue;
