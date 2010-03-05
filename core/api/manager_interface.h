#ifndef _MANAGER_INTERFACE_H_
#define _MANAGER_INTERFACE_H_

#include <glib.h>

/* Plugin interface*/
typedef enum {
	MANAGER_IFACE_DIRECTION_INPUT = 0,
	MANAGER_IFACE_DIRECTION_OUTPUT
} manager_iface_direction;

typedef struct {
	guint major;
	guint minor;
} manager_iface_version;

typedef struct {
	manager_iface_version	version;
	manager_iface_direction	direction;
	guint16					transfer_content_type;
} manager_interface;

/* Plugin loading functions */
typedef manager_interface*	(*LoadManagerModuleFunc)	(void);
typedef gboolean			(*UnloadManagerModuleFunc)	(void);

#endif
