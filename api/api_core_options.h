#ifndef _API_CORE_OPTIONS_H_
#define _API_CORE_OPTIONS_H_

#include <glib.h>

#ifndef DLLIMPORT
#define DLLIMPORT extern
#endif

/**
 * "program"	=> program executable directory (from environment)
 *
 * "config"		=> configuration module (from command line)
 * "queue"		=> queue module (from command line)
 */
DLLIMPORT GHashTable*	core_options_get();

#endif
