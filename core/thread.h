#ifndef _THREAD_H_
#define _THREAD_H_

#include <glib.h>

gpointer thread_forward_pull(gpointer data);
gpointer thread_forward_push(gpointer data);
gpointer thread_forward_receive(gpointer data);

gpointer thread_feeback_push(gpointer data);
gpointer thread_feeback_pull(gpointer data);
gpointer thread_feeback_receive(gpointer data);

gpointer thread_trash_receive(gpointer data);

#endif
