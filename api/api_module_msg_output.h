#ifndef _API_MODULE_MSG_OUTPUT_H_
#define _API_MODULE_MSG_OUTPUT_H_

#include <api_module_interface.h>
#include <api_module_messaging.h>

// vtable for messaging output module
typedef struct {
	module_producer_type	(*producer_type)			();

	void					(*invoker_push_feedback)	();

	message_batch*			(*handler_pull_feedback)	();
	gboolean				(*handler_receive_forward)	(message* data);
} module_vtable_msg_output;

// function which returns vtable for that module
typedef module_vtable_msg_output* (*OutputModuleFunc) (void); // OutputModule

#endif
