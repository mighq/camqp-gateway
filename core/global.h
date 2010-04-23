#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <api_module_queue.h>
#include "module.h"

#ifndef GLOBALIMPORT
#define GLOBALIMPORT extern
#endif

/**
 * general
 *
 */
GLOBALIMPORT guint			g_instance;
GLOBALIMPORT GHashTable*	g_options;
GLOBALIMPORT GHashTable*	g_modules;

/**
 * provider modules
 *
 */
GLOBALIMPORT module_loaded*	g_provider_config;
GLOBALIMPORT module_loaded*	g_provider_queue;

/**
 * messaging modules
 *
 */
GLOBALIMPORT module_loaded*	g_handlers_input;
GLOBALIMPORT module_loaded*	g_handlers_output;
GLOBALIMPORT module_loaded*	g_handlers_trash;

/**
 * queues
 *
 */
GLOBALIMPORT queue*			g_queue_forward;
GLOBALIMPORT queue*			g_queue_feedback;
GLOBALIMPORT queue*			g_queue_trash;

/**
 * thread locks & conditions
 *
 */
GLOBALIMPORT gboolean		g_termination;
GLOBALIMPORT GMutex*		g_lck_termination;

GLOBALIMPORT GCond*			g_cnd_forward_receive;
GLOBALIMPORT GMutex*		g_lck_forward_receive;

#endif
