#ifndef _API_MODULE_INTERFACE_H_
#define _API_MODULE_INTERFACE_H_

#include <glib.h>

typedef enum {
	MODULE_TYPE_CONFIG = 0,
	MODULE_TYPE_QUEUE,
	MODULE_TYPE_INPUT,
	MODULE_TYPE_OUTPUT
} module_type;

typedef struct {
	module_type		type;
	gchar*			name;
} module_info;

typedef module_info*	(*LoadModuleFunc)	(void); // LoadModule
typedef gboolean		(*UnloadModuleFunc)	(void); // UnloadModule

#endif
