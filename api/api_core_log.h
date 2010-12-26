#ifndef _API_CORE_LOG_H_
#define _API_CORE_LOG_H_

#include <glib.h>

#include <syslog.h>

#ifndef DLLIMPORT
#define DLLIMPORT extern
#endif

DLLIMPORT gboolean core_log(gchar* domain, guchar level, guint code, gchar* message);

#endif
