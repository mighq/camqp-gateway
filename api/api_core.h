#ifndef _API_CORE_H_
#define _API_CORE_H_

#ifndef DLLIMPORT
#define DLLIMPORT extern
#endif

/**
 * returns instance identifier
 */
DLLIMPORT guint32		core_instance();

/**
 * returns if core received termination signal
 */
DLLIMPORT gboolean		core_terminated();

DLLIMPORT void			core_register_condition(GCond* condition, GMutex* mutex);

#endif
