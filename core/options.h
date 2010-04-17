#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <glib.h>

// API functions
#define DLLIMPORT
#include <api_core_options.h>

// private functions
gboolean core_options_environment(GError** error);
gboolean core_options_treat(gint argc, gchar** argv, GError** error);

#endif
