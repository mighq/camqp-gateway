#ifndef _API_CORE_CONFIG_H_
#define _API_CORE_CONFIG_H_

#include <glib.h>

#ifndef DLLIMPORT
#define DLLIMPORT extern
#endif

DLLIMPORT gboolean		core_config_isset(gchar* group, gchar* option);

DLLIMPORT gboolean		core_config_get_bool(gchar* group, gchar* option);
DLLIMPORT gint32		core_config_get_int(gchar* group, gchar* option);
DLLIMPORT gfloat		core_config_get_real(gchar* group, gchar* option);
DLLIMPORT gchar*		core_config_get_text(gchar* group, gchar* option);
DLLIMPORT GByteArray*	core_config_get_bin(gchar* group, gchar* option);

#endif
