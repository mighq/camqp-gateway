#ifndef _API_MODULE_MSG_INPUT_H_
#define _API_MODULE_MSG_INPUT_H_

#include <api_module_interface.h>
#include <api_module_messaging.h>

// vtable for config module
typedef struct {
	module_producer_type	(*producer_type)			();

	void					(*invoker_push_forward)		();

	message_batch*			(*handler_pull_forward)		();
	gboolean				(*handler_receive_feedback)	(const message* const data);
} module_vtable_msg_input;

// function which returns vtable for that module
typedef module_vtable_msg_input* (*InputModuleFunc) (void); // InputModule

#endif
