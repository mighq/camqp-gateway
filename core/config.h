#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <glib.h>

// API functions
#include <api_core_config.h>
#include <api_module_config.h>

// private core functions
module_vtable_config*	core_config_init();
gboolean				core_config_destroy();

module_vtable_config*	core_config_provider();

void					core_config_preload();

#endif
