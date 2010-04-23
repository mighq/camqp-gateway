#ifndef _MESSAGING_H_
#define _MESSAGING_H_

#include <api_module_interface.h>
#include <api_module_msg_input.h>
#include <api_module_msg_output.h>
#include <api_module_msg_trash.h>

#include "module.h"

module_vtable_msg_input*	core_messaging_handlers_input();
module_vtable_msg_output*	core_messaging_handlers_output();
module_vtable_msg_trash*	core_messaging_handlers_trash();

gboolean core_messaging_module_init(module_type type, GError** error);
gboolean core_messaging_module_check(module_loaded* module, GError** error);

void core_messaging_init();
void core_messaging_start();
void core_messaging_destroy();

#endif
