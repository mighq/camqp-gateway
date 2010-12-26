#include "thread.h"
#include "global.h"
#include "queue.h"
#include "messaging.h"
#include "log.h"

#include <api_core.h>
#include <api_core_config.h>
#include <api_core_messaging.h>
#include <api_module_msg_input.h>
#include <api_module_msg_output.h>

gpointer thread_forward_pull(gpointer data) {
	core_log("msg", LOG_INFO, 1111, "starting thread: FORWARD PULL");

	GTimeVal wakeup;
	gfloat timeout;

	// get retry timeout from config (miliseconds)
	if (!core_config_isset("core", "forward_pull_timeout")) {
		core_log("msg", LOG_WARNING, 1111, "Timeout for forward pull not specified in configuration! Using default 30s.");
		timeout = 30000;
	} else {
		timeout = core_config_get_real("core", "forward_pull_timeout");
	}

	// get input module handler
	module_vtable_msg_input* tbl = (module_vtable_msg_input*) g_handlers_input->vtable;

	// do it until termination
	gboolean term = core_terminated();
	while (term == FALSE) {
		// get batch from input module
		core_log("msg", LOG_INFO, 1111, "calling FORWARD PULL handler");
		message_batch* todo = tbl->handler_pull_forward();

		// push batch to queue
		core_handler_push_forward(todo);

		// determine next wakeup time
		g_get_current_time(&wakeup);
		g_time_val_add(&wakeup, timeout*1000);

		// wait for timeout & or be interrupted
		g_mutex_lock(g_lck_forward_pull);
		g_cond_timed_wait(g_cnd_forward_pull, g_lck_forward_pull, &wakeup);
		g_mutex_unlock(g_lck_forward_pull);

		// check if we have to stop
		term = core_terminated();
	}

	core_log("msg", LOG_INFO, 1111, "ending thread: FORWARD PULL");

	return NULL;
}

/**
 * calling invoker in loop, until termination occurs
 */
gpointer thread_forward_push(gpointer data) {
	core_log("msg", LOG_INFO, 1111, "starting thread: FORWARD PUSH");

	// get input module invoker
	module_vtable_msg_input* tbl = core_messaging_handlers_input();

	// do it until termination
	gboolean term = core_terminated();
	while (term == FALSE) {
		// delegate logic to input module
		core_log("msg", LOG_INFO, 1111, "calling FORWARD PUSH invoker");
		tbl->invoker_push_forward();

		// check if we have to stop
		term = core_terminated();
	}

	core_log("msg", LOG_INFO, 1111, "ending thread: FORWARD PUSH");

	return NULL;
}

/**
 * receives data from forward queue
 *
 */
gpointer thread_forward_receive(gpointer data) {
	core_log("msg", LOG_INFO, 1111, "starting thread: FORWARD RECV");

	// get output module handler
	module_vtable_msg_output* tbl = core_messaging_handlers_output();

	// do it until termination
	gboolean term = core_terminated();
	while (term == FALSE) {
		// get message from queue
		message* msg = core_queue_pop(g_queue_forward);

		// while there's something to do
		while (msg != NULL) {
			// send message to output plugin
			core_log("msg", LOG_INFO, 1111, "calling FORWARD RECV handler");
			gboolean delivered = tbl->handler_receive_forward(msg);

			if (delivered) {
				// delivered successfully

				// free message data
				message_free(msg);
			} else {
				// error during delivery

				// add message to trash
				core_handler_push_trash(msg);
			}

			// get next message from queue
			msg = core_queue_pop(g_queue_forward);
		}

		// wait till new data arrive to queue
		g_mutex_lock(g_lck_forward_receive);
		g_cond_wait(g_cnd_forward_receive, g_lck_forward_receive);
		g_mutex_unlock(g_lck_forward_receive);

		// check if we have to stop
		term = core_terminated();
	}

	core_log("msg", LOG_INFO, 1111, "ending thread: FORWARD RECV");

	return NULL;
}


gpointer thread_feedback_push(gpointer data) {
	core_log("msg", LOG_INFO, 1111, "starting thread: FEEDBACK PUSH");

	// get input module invoker
	module_vtable_msg_output* tbl = core_messaging_handlers_output();

	// do it until termination
	gboolean term = core_terminated();
	while (term == FALSE) {
		// delegate logic to input module
		core_log("msg", LOG_INFO, 1111, "calling FEEDBACK PUSH invoker");
		tbl->invoker_push_feedback();

		// check if we have to stop
		term = core_terminated();
	}

	core_log("messaging", LOG_INFO, 1111, "ending thread: FEEDBACK PUSH");

	return NULL;
}

gpointer thread_feedback_pull(gpointer data) {
	core_log("msg", LOG_INFO, 1111, "starting thread: FEEDBACK PULL");

	GTimeVal wakeup;
	gfloat timeout;

	// get retry timeout from config (miliseconds)
	if (!core_config_isset("core", "feedback_pull_timeout")) {
		core_log("msg", LOG_WARNING, 1111, "Timeout for feedback pull not specified in configuration! Using default 30s.");
		timeout = 30000;
	} else {
		timeout = core_config_get_real("core", "feedback_pull_timeout");
	}

	// get output module handler
	module_vtable_msg_output* tbl = (module_vtable_msg_output*) g_handlers_output->vtable;

	// do it until termination
	gboolean term = core_terminated();
	while (term == FALSE) {
		// get batch from input module
		core_log("msg", LOG_INFO, 1111, "calling FEEDBACK PULL handler");
		message_batch* todo = tbl->handler_pull_feedback();

		// push batch to queue
		core_handler_push_feedback(todo);

		// determine next wakeup time
		g_get_current_time(&wakeup);
		g_time_val_add(&wakeup, timeout*1000);

		// wait for timeout & or be interrupted
		g_mutex_lock(g_lck_feedback_pull);
		g_cond_timed_wait(g_cnd_feedback_pull, g_lck_feedback_pull, &wakeup);
		g_mutex_unlock(g_lck_feedback_pull);

		// check if we have to stop
		term = core_terminated();
	}

	core_log("msg", LOG_INFO, 1111, "ending thread: FEEDBACK PULL");

	return NULL;
}

gpointer thread_feedback_receive(gpointer data) {
	core_log("msg", LOG_INFO, 1111, "starting thread: FEEDBACK RECV");

	// get output module handler
	module_vtable_msg_input* tbl = core_messaging_handlers_input();

	// do it until termination
	gboolean term = core_terminated();
	while (term == FALSE) {
		// get message from queue
		message* msg = core_queue_pop(g_queue_feedback);

		// while there's something to do
		while (msg != NULL) {
			// send message to output plugin
			core_log("msg", LOG_INFO, 1111, "calling FEEDBACK RECV handler");
			gboolean delivered = tbl->handler_receive_feedback(msg);

			if (delivered) {
				// delivered successfully

				// free message data
				message_free(msg);
			} else {
				// error during delivery

				// add message to trash
				core_handler_push_trash(msg);
			}

			// get next message from queue
			msg = core_queue_pop(g_queue_feedback);
		}

		// wait till new data arrive to queue
		g_mutex_lock(g_lck_feedback_receive);
		g_cond_wait(g_cnd_feedback_receive, g_lck_feedback_receive);
		g_mutex_unlock(g_lck_feedback_receive);

		// check if we have to stop
		term = core_terminated();
	}

	core_log("msg", LOG_INFO, 1111, "ending thread: FEEDBACK RECV");

	return NULL;
}

gpointer thread_trash_receive(gpointer data) {
	core_log("msg", LOG_INFO, 1111, "starting thread: TRASH RECV");

	// get output module handler
	module_vtable_msg_trash* tbl = core_messaging_handlers_trash();

	// do it until termination
	gboolean term = core_terminated();
	while (term == FALSE) {
		// get message from queue
		message* msg = core_queue_pop(g_queue_trash);

		// while there's something to do
		while (msg != NULL) {
			// send message to output plugin
			core_log("msg", LOG_INFO, 1111, "calling TRASH RECV handler");
			tbl->handler_receive_trash(msg);

			// free message data
			message_free(msg);

			// get next message from queue
			msg = core_queue_pop(g_queue_forward);
		}

		// wait till new data arrive to queue
		g_mutex_lock(g_lck_trash_receive);
		g_cond_wait(g_cnd_trash_receive, g_lck_trash_receive);
		g_mutex_unlock(g_lck_trash_receive);

		// check if we have to stop
		term = core_terminated();
	}

	core_log("msg", LOG_INFO, 1111, "ending thread: TRASH RECV");

	return NULL;
}
