#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <glib.h>

#include <api_module_config.h>
#include <api_module_queue.h>

extern GHashTable*				g_options;
extern GHashTable*				g_modules;

extern module_vtable_config*	g_provider_config;
extern module_vtable_queue*		g_provider_queue;

#endif
