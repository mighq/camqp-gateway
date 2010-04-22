#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <glib.h>

#include <api_module_queue.h>
#include "module.h"

extern guint			g_instance;
extern GHashTable*		g_options;
extern GHashTable*		g_modules;

extern module_loaded*	g_provider_config;
extern module_loaded*	g_provider_queue;

extern module_loaded*	g_handlers_input;
extern module_loaded*	g_handlers_output;
extern module_loaded*	g_handlers_trash;

extern queue*			g_queue_forward;
extern queue*			g_queue_feedback;
extern queue*			g_queue_trash;

#endif
