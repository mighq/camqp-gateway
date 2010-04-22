#ifndef _MODULE_H_
#define _MODULE_H_

#include <gmodule.h>

#include <api_module_interface.h>

typedef struct {
	GModule*		module;
	module_info*	info;
	gpointer*		vtable;
} module_loaded;

// common
gchar*			core_module_file(module_type type, const gchar* name);

module_loaded*	core_module_load(module_type type, const gchar* name, GError** error);
gboolean		core_module_unload(module_type type, const gchar* name);

void			core_module_unload_ptr(module_loaded* data);

#endif
