#include <glib.h>			// GLib framework
#include <glib/gprintf.h>	// UTF-8 printf

#include "global.h"
#include "options.h"
#include "module.h"
#include "config.h"
#include "log.h"
#include "queue.h"
#include "messaging.h"

#include <signal.h>	// signals
#include <locale.h>	// locales

void signal_handler(int signum)
{
	core_log("core", LOG_NOTICE, 1111, "signal received");

	module_vtable_msg_input*	vt_i = core_messaging_handlers_input();
	module_vtable_msg_output*	vt_o = core_messaging_handlers_output();

	// === setup termination
	g_mutex_lock(g_lck_termination);
	g_termination = TRUE;
	g_mutex_unlock(g_lck_termination);

	// === signal wake-ups to all handlers

	// forward push
	if (vt_i->terminate_blocking != NULL)
		vt_i->terminate_blocking(THREAD_TYPE_FORWARD_PUSH);

	// forward pull
	if (g_lck_forward_pull != NULL) {
		g_mutex_lock(g_lck_forward_pull);
		g_cond_signal(g_cnd_forward_pull);
		g_mutex_unlock(g_lck_forward_pull);
	}
	if (vt_i->terminate_blocking != NULL)
		vt_i->terminate_blocking(THREAD_TYPE_FORWARD_PULL);

	// forward receiver
	g_mutex_lock(g_lck_forward_receive);
	g_cond_signal(g_cnd_forward_receive);
	g_mutex_unlock(g_lck_forward_receive);
	if (vt_o->terminate_blocking != NULL)
		vt_o->terminate_blocking(THREAD_TYPE_FORWARD_RECV);

	// feedback push
	if (vt_o->terminate_blocking != NULL)
		vt_o->terminate_blocking(THREAD_TYPE_FEEDBACK_PUSH);

	// feedback pull
	if (g_lck_feedback_pull != NULL) {
		g_mutex_lock(g_lck_feedback_pull);
		g_cond_signal(g_cnd_feedback_pull);
		g_mutex_unlock(g_lck_feedback_pull);
	}
	if (vt_o->terminate_blocking != NULL)
		vt_o->terminate_blocking(THREAD_TYPE_FEEDBACK_PULL);

	// feedback receiver
	g_mutex_lock(g_lck_feedback_receive);
	g_cond_signal(g_cnd_feedback_receive);
	g_mutex_unlock(g_lck_feedback_receive);
	if (vt_i->terminate_blocking != NULL)
		vt_i->terminate_blocking(THREAD_TYPE_FEEDBACK_RECV);

	// trash receiver
	g_mutex_lock(g_lck_trash_receive);
	g_cond_signal(g_cnd_trash_receive);
	g_mutex_unlock(g_lck_trash_receive);
}

void core_init() {
	// setup runtime
	setlocale(LC_ALL, "C.UTF-8");

	// do not terminate yet
	g_termination = FALSE;

	g_provider_config = NULL;
	g_provider_queue = NULL;
	g_provider_log = NULL;

	// setup signal handling
	struct sigaction signal;
	signal.sa_handler = signal_handler;
	sigemptyset(&signal.sa_mask);
	signal.sa_flags = 0;

	sigaction(SIGHUP,  &signal, NULL);
	sigaction(SIGINT,  &signal, NULL); // CTRL+C [stty intr "^C"]
	sigaction(SIGQUIT, &signal, NULL); // CTRL+\ [stty quit "^\\"]
	sigaction(SIGTERM, &signal, NULL);
	sigaction(SIGUSR1, &signal, NULL);
	sigaction(SIGUSR2, &signal, NULL);

	// init threads
	#ifndef G_THREADS_ENABLED
	#error "Threads not supported by GLib!"
	#endif
	#ifdef G_THREADS_IMPL_NONE
	#error "No threads implementation in GLib!"
	#endif
	if (!g_thread_get_initialized()) {
		g_thread_init(NULL);
		if (!g_thread_get_initialized()) {
			return;
		}
	}

	// allocate global variables
	g_options = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	g_modules = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) core_module_unload_ptr);
}

void core_destroy() {
	// destroy global variables
	g_hash_table_destroy(g_options);
	g_hash_table_destroy(g_modules);
}

int main(gint argc, gchar* argv[]) {
	GError* err = NULL;

	// initialize
	core_log("core", LOG_NOTICE, 1111, "initializing core");
	core_init();

	// determine essential things
	core_log("core", LOG_INFO, 1111, "checking environment");
	if (!core_options_environment(&err)) {
		core_log("core", LOG_ERR, 1111, err->message);
		g_error_free(err);
		core_destroy();
		return 2;
	}

	// treat cmd-line options
	core_log("core", LOG_INFO, 1111, "parsing command line");
	if (!core_options_treat(argc, argv, &err)) {
		core_log("core", LOG_ERR, 1111, err->message);
		g_error_free(err);

		core_destroy();

		return 3;
	}

	// init config provider
	core_log("core", LOG_INFO, 1111, "initializing configuration");
	if (!core_config_provider_init()) {
		core_log("core", LOG_CRIT, 1111, "cannot initialize config provider");

		core_destroy();

		return 4;
	}

	// do config preloading
	core_log("core", LOG_INFO, 1111, "loading configuration");
	core_config_preload();

	// init logger provider
	core_log("core", LOG_INFO, 1111, "initializing log provider");
	if (!core_log_provider_init()) {
		core_log("core", LOG_ERR, 1111, "cannot initialize log provider");

		core_config_provider_destroy();
		core_destroy();
		return 1111;
	}

	core_log("core", LOG_INFO, 1111, "logging system initialized");

	// init queue provider
	core_log("core", LOG_INFO, 1111, "initializing queue provider");
	if (!core_queue_provider_init(&err)) {
		core_log("core", LOG_CRIT, 1111, "cannot initialize queue provider");

		g_error_free(err);
		core_log_provider_destroy();
		core_config_provider_destroy();
		core_destroy();

		return 5;
	}

	// load message handling modules
	core_log("core", LOG_INFO, 1111, "initializing msg input module");
	if (!core_messaging_module_init(MODULE_TYPE_MESSAGE_INPUT, &err)) {
		core_log("core", LOG_ERR, 1111, err->message);

		g_error_free(err);
		core_queue_provider_destroy();
		core_log_provider_destroy();
		core_config_provider_destroy();
		core_destroy();

		return 6;
	}

	core_log("core", LOG_INFO, 1111, "initializing msg output module");
	if (!core_messaging_module_init(MODULE_TYPE_MESSAGE_OUTPUT, &err)) {
		core_log("core", LOG_ERR, 1111, err->message);

		g_error_free(err);
		core_queue_provider_destroy();
		core_log_provider_destroy();
		core_config_provider_destroy();
		core_destroy();

		return 7;
	}

	core_log("core", LOG_INFO, 1111, "initializing msg trash module");
	if (!core_messaging_module_init(MODULE_TYPE_MESSAGE_TRASH, &err)) {
		core_log("core", LOG_ERR, 1111, err->message);

		g_error_free(err);
		core_queue_provider_destroy();
		core_log_provider_destroy();
		core_config_provider_destroy();
		core_destroy();

		return 8;
	}

	// init messaging framework
	core_log("core", LOG_INFO, 1111, "initializing messaging");
	core_messaging_init();

	// start messaging framework
	core_log("core", LOG_NOTICE, 1111, "starting messaging");
	core_messaging_start();

	// destroy messaging framework
	core_log("core", LOG_INFO, 1111, "destroying messaging");
	core_messaging_destroy();

	// destroy queue provider
	core_log("core", LOG_INFO, 1111, "destroying queue provider");
	core_queue_provider_destroy();

	// destroy log provider
	core_log("core", LOG_INFO, 1111, "destroying logging provider");
	core_log_provider_destroy();

	// destroy config
	core_log("core", LOG_INFO, 1111, "destroying config provider");

	core_config_provider_destroy();

	// do cleanup (options & modules)
	core_log("core", LOG_INFO, 1111, "destroying core");
	core_destroy();

	return 0;
}
