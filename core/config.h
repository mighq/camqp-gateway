#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <glib.h>

// provider interface
#include <api_module_config.h>

// config provider functions
module_vtable_config*	core_config_provider_init();
gboolean				core_config_provider_destroy();
module_vtable_config*	core_config_provider();

// public config functions
#define DLLIMPORT
#include <api_core_config.h>

// private config functions
void					core_config_preload();

#endif
