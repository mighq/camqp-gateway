#include <glib.h>

#include <api_module_queue.h>
#include "module.h"

guint			g_instance;

GHashTable*		g_options;
GHashTable*		g_modules;

module_loaded*	g_provider_config;
module_loaded*	g_provider_queue;

module_loaded*	g_handlers_input;
module_loaded*	g_handlers_output;
module_loaded*	g_handlers_trash;

queue*			g_queue_forward;
queue*			g_queue_feedback;
queue*			g_queue_trash;
