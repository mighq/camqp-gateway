#include "thread.h"
#include "global.h"
#include "queue.h"

#include <api_core.h>
#include <api_module_msg_input.h>
#include <api_module_msg_output.h>

gpointer thread_forward_pull(gpointer data) {
	g_print("=frwd_pull\n");
	g_print("#frwd_pull\n");

	return NULL;
}

/**
 * calling invoker in loop, until termination occurs
 */
gpointer thread_forward_push(gpointer data) {
	g_print("=frwd_push\n");

	// get input module invoker
	module_vtable_msg_input* tbl = (module_vtable_msg_input*) g_handlers_input->vtable;

	// do it until termination
	gboolean term = core_terminated();
//	while (term == FALSE) {
		// delegate logic to input module
		tbl->invoker_push_forward();

		// check if we have to stop
		term = core_terminated();
//	}

	g_print("#frwd_push\n");

	return NULL;
}

/**
 * receives data from forward queue
 *
 */
gpointer thread_forward_receive(gpointer data) {
	g_print("=frwd_recv\n");

	// get output module handler
	module_vtable_msg_output* tbl = (module_vtable_msg_output*) g_handlers_output->vtable;

	// do it until termination
	gboolean term = core_terminated();
	while (term == FALSE) {
		// wait till data arrive
		g_mutex_lock(g_lck_forward_receive);
		g_cond_wait(g_cnd_forward_receive, g_lck_forward_receive);
		g_mutex_unlock(g_lck_forward_receive);

		// foreach messages from queue
		gboolean delivered = FALSE;
		message* data = core_queue_pop(g_queue_forward);
		while (data != NULL) {
			// send message to output plugin
			delivered = tbl->handler_receive_forward(data);

			// TODO: if not delivered, drop to trash

			// receive next
			data = core_queue_pop(g_queue_forward);
		}
		g_print("nothing (more) to receive\n");

		// check if we have to stop
		term = core_terminated();
	}

	g_print("#frwd_recv\n");

	return NULL;
}


gpointer thread_feeback_push(gpointer data) {
//	g_print("=fbck_push\n");
//	g_print("#fbck_push\n");
	return NULL;
}

gpointer thread_feeback_pull(gpointer data) {
//	g_print("=fbck_pull\n");
//	g_print("#fbck_pull\n");
	return NULL;
}

gpointer thread_feeback_receive(gpointer data) {
//	g_print("=fbck_recv\n");
//	g_print("#fbck_recv\n");
	return NULL;
}


gpointer thread_trash_receive(gpointer data) {
//	g_print("=trsh_recv\n");
//	g_print("#trsh_recv\n");
	return NULL;
}
