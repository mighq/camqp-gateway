#include <glib.h>			// GLib framework
#include <glib/gprintf.h>	// UTF-8 printf

#include "global.h"
#include "options.h"
#include "module.h"
#include "config.h"
#include "queue.h"
#include "messaging.h"

#include <signal.h>	// signals
#include <locale.h>	// locales

void signal_handler(int signum)
{
	g_print("Handling signal %d\n", signum);

	// === setup termination
	g_mutex_lock(g_lck_termination);
	g_termination = TRUE;
	g_mutex_unlock(g_lck_termination);

	// === signal wake-ups to all handlers

	// forward pull
	if (g_lck_forward_pull != NULL) {
		g_mutex_lock(g_lck_forward_pull);
		g_cond_signal(g_cnd_forward_pull);
		g_mutex_unlock(g_lck_forward_pull);
	}

	// forward receiver
	g_mutex_lock(g_lck_forward_receive);
	g_cond_signal(g_cnd_forward_receive);
	g_mutex_unlock(g_lck_forward_receive);

	// feedback pull
	if (g_lck_feedback_pull != NULL) {
		g_mutex_lock(g_lck_feedback_pull);
		g_cond_signal(g_cnd_feedback_pull);
		g_mutex_unlock(g_lck_feedback_pull);
	}

	// feedback receiver
	g_mutex_lock(g_lck_feedback_receive);
	g_cond_signal(g_cnd_feedback_receive);
	g_mutex_unlock(g_lck_feedback_receive);

	// trash receiver
	g_mutex_lock(g_lck_trash_receive);
	g_cond_signal(g_cnd_trash_receive);
	g_mutex_unlock(g_lck_trash_receive);

	// signalize all external conditions
	GSList* list = g_conditions;
	while (list != NULL) {
		condition_info* ci = (condition_info*) list->data;
		g_mutex_lock(ci->mutex);
		g_cond_broadcast(ci->condition);
		g_mutex_unlock(ci->mutex);

		list = list->next;
	}
}

void core_init() {
	// setup runtime
	setlocale(LC_ALL, "C.UTF-8");

	// do not terminate yet
	g_termination = FALSE;
	g_sequence_started = FALSE;

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

	g_conditions = NULL;
}

void core_destroy() {
	// destroy global variables
	g_hash_table_destroy(g_options);
	g_hash_table_destroy(g_modules);

	// delete all external condition entries
	GSList* list = g_conditions;
	while (list != NULL) {
		condition_info* ci = (condition_info*) list->data;
		g_free(ci);

		list = list->next;
	}
	g_slist_free(g_conditions);
}

void core_register_condition(GCond* condition, GMutex* mutex) {
	condition_info* ci = g_try_new0(condition_info, 1);

	ci->condition = condition;
	ci->mutex = mutex;

	g_conditions = g_slist_append(g_conditions, ci);
}

int main(gint argc, gchar* argv[]) {
	GError* err = NULL;

	// initialize
	core_init();

	// determine essential things
	if (!core_options_environment(&err)) {
		g_print("%s\n", err->message);
		g_error_free(err);
		core_destroy();
		return 2;
	}

	// treat cmd-line options
	if (!core_options_treat(argc, argv, &err)) {
		g_print("%s\n", err->message);
		g_error_free(err);

		core_destroy();

		return 3;
	}

	// init config provider
	if (!core_config_provider_init()) {
		g_print("Cannot initialize config provider!\n");

		core_destroy();

		return 4;
	}

	// do config preloading
	core_config_preload();

	// init queue provider
	if (!core_queue_provider_init(&err)) {
		g_print("Cannot initialize queue provider: %s!\n", err->message);
	
		g_error_free(err);
		core_config_provider_destroy();
		core_destroy();

		return 5;
	}

	// load message handling modules
	if (!core_messaging_module_init(MODULE_TYPE_MESSAGE_INPUT, &err)) {
		g_print("%s\n", err->message);

		g_error_free(err);
		core_queue_provider_destroy();
		core_config_provider_destroy();
		core_destroy();

		return 6;
	}

	if (!core_messaging_module_init(MODULE_TYPE_MESSAGE_OUTPUT, &err)) {
		g_print("%s\n", err->message);

		g_error_free(err);
		core_queue_provider_destroy();
		core_config_provider_destroy();
		core_destroy();

		return 7;
	}

	if (!core_messaging_module_init(MODULE_TYPE_MESSAGE_TRASH, &err)) {
		g_print("%s\n", err->message);

		g_error_free(err);
		core_queue_provider_destroy();
		core_config_provider_destroy();
		core_destroy();

		return 8;
	}

	// init messaging framework
	core_messaging_init();


	// start messaging framework
	core_messaging_start();


	// destroy messaging framework
	core_messaging_destroy();

	// destroy queue provider
	core_queue_provider_destroy();

	// destroy config
	core_config_provider_destroy();

	// do cleanup (options & modules)
	core_destroy();

	return 0;
}
