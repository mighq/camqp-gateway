#ifndef _API_CONFIG_H_
#define _API_CONFIG_H_

#include <glib.h>

gboolean	core_config_isset(gchar* group, gchar* option);

gboolean	core_config_get_bool(gchar* group, gchar* option);
gint32		core_config_get_int(gchar* group, gchar* option);
gfloat		core_config_get_real(gchar* group, gchar* option);
gchar*		core_config_get_text(gchar* group, gchar* option);
GByteArray*	core_config_get_bin(gchar* group, gchar* option);

#endif
