#ifndef _API_MODULE_LOG_H_
#define _API_MODULE_LOG_H_

#include <api_module_interface.h>

// vtable for log module
typedef struct {
	gboolean	(*log)		(gchar* domain, guchar level, guint code, gchar* message);
	gboolean	(*init)		(GError** error);
	gboolean	(*destroy)	();
} module_vtable_log;

// function which returns vtable for that module
typedef module_vtable_log* (*LogModuleFunc) (void); // LogModule

#endif
