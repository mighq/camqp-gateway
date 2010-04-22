#ifndef _API_MODULE_MSG_TRASH_H_
#define _API_MODULE_MSG_TRASH_H_

#include <api_module_interface.h>
#include <api_module_messaging.h>

// vtable for messaging trash module
typedef struct {
	void	(*handler_receive_trash)	(message* data);
} module_vtable_msg_trash;

// function which returns vtable for that module
typedef module_vtable_msg_trash* (*TrashModuleFunc) (void); // TrashModule

#endif
