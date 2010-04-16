#ifndef _API_MODULE_CONFIG_H_
#define _API_MODULE_CONFIG_H_

#include <api_module_interface.h>

// vtable for config module
typedef struct {
	void		(*preload)	();

	gboolean	(*isset)	(gchar* group, gchar* option);

	gboolean	(*get_bool)	(gchar* group, gchar* option);
	gint32		(*get_int)	(gchar* group, gchar* option);
	gfloat		(*get_real)	(gchar* group, gchar* option);
	gchar*		(*get_text)	(gchar* group, gchar* option);
	GByteArray*	(*get_bin)	(gchar* group, gchar* option);
} module_vtable_config;

// function which returns vtable for that module
typedef module_vtable_config* (*ConfigModuleFunc) (void); // ConfigModule

#endif
