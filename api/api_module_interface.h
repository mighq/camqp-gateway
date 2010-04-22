#ifndef _API_MODULE_INTERFACE_H_
#define _API_MODULE_INTERFACE_H_

#include <glib.h>

typedef enum {
	MODULE_TYPE_CONFIG = 0,
	MODULE_TYPE_QUEUE,
	MODULE_TYPE_MESSAGE_INPUT,
	MODULE_TYPE_MESSAGE_OUTPUT,
	MODULE_TYPE_MESSAGE_TRASH
} module_type;

typedef struct {
	module_type		type;
	gchar*			name;
} module_info;

typedef module_info*	(*LoadModuleFunc)	(void); // LoadModule
typedef gboolean		(*UnloadModuleFunc)	(void); // UnloadModule

typedef gpointer		(*ModuleFunc)		(void); // abstract module entry function

#endif
