#ifndef _LOG_H_
#define _LOG_H_

#include <glib.h>

// provider interface
#include <api_module_log.h>

// logging provider functions
gboolean				core_log_provider_init();
gboolean				core_log_provider_destroy();

module_vtable_log*		core_log_provider();

// public logging functions
#define DLLIMPORT
#include <api_core_log.h>

#endif
