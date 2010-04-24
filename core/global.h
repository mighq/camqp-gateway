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

// used to indicate program termination
GLOBALIMPORT gboolean		g_termination;
GLOBALIMPORT GMutex*		g_lck_termination;

// used to indicate that new messages are on trash queue and trash receiver thread should be woken-up
GLOBALIMPORT GCond*			g_cnd_trash_receive;
GLOBALIMPORT GMutex*		g_lck_trash_receive;

// used to indicate that new messages are on forward queue and forward receiver thread should be woken-up
GLOBALIMPORT GCond*			g_cnd_forward_receive;
GLOBALIMPORT GMutex*		g_lck_forward_receive;
/*
// used to hold forward push thread until receiver is ready
GLOBALIMPORT GCond*			g_cnd_forward_push;
GLOBALIMPORT GMutex*		g_lck_forward_push;
*/
// used to breakable-timeout on forward pull thread
GLOBALIMPORT GCond*			g_cnd_forward_pull;
GLOBALIMPORT GMutex*		g_lck_forward_pull;

#endif
