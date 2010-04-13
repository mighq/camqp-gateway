#include <glib.h> // GLib framework
#include <glib/gprintf.h> // UTF-8 printf

#include <signal.h> // signals
#include <stdlib.h> // exit
#include <locale.h> // locales

GMutex* g_lck_termination;
gboolean g_termination = FALSE;

GCond* g_cnd_t1 = NULL;
GMutex* g_lck_t1 = NULL;

GCond* g_cnd_t2 = NULL;
GMutex* g_lck_t2 = NULL;

void signal_handler(int signum)
{
	g_printf("Handling signal %d\n", signum);

	// check for termination
	g_mutex_lock(g_lck_termination);
	g_termination = TRUE;
	g_mutex_unlock(g_lck_termination);

	// send break of delay to each thread
	g_mutex_lock(g_lck_t1);
	g_cond_signal(g_cnd_t1);
	g_mutex_unlock(g_lck_t1);

	g_mutex_lock(g_lck_t2);
	g_cond_signal(g_cnd_t2);
	g_mutex_unlock(g_lck_t2);
}

gpointer Func1(gpointer data) {
	while (TRUE) {
		// do what is needed
		g_printf("F1\n");

		// sleep
		GTimeVal delay;
		g_get_current_time(&delay);
		g_time_val_add(&delay, 5000000);
		g_mutex_lock(g_lck_t1);
		g_cond_timed_wait(g_cnd_t1, g_lck_t1, &delay);
		g_mutex_unlock(g_lck_t1);

		// check for termination
		g_mutex_lock(g_lck_termination);
		if (g_termination) {
			g_mutex_unlock(g_lck_termination);
			break;
		}
		g_mutex_unlock(g_lck_termination);
	}

	g_printf("EO T1\n");

	return NULL;
}

gpointer Func2(gpointer data) {
	while (TRUE) {
		// do what is needed
		g_printf("F2\n");

		// sleep
		GTimeVal delay;
		g_get_current_time(&delay);
		g_time_val_add(&delay, 2000000);
		g_mutex_lock(g_lck_t2);
		g_cond_timed_wait(g_cnd_t2, g_lck_t2, &delay);
		g_mutex_unlock(g_lck_t2);

		// check for termination
		g_mutex_lock(g_lck_termination);
		if (g_termination) {
			g_mutex_unlock(g_lck_termination);
			break;
		}
		g_mutex_unlock(g_lck_termination);
	}

	g_printf("EO T2\n");

	return NULL;
}

int main(int argc, const char* argv[]) {
	setlocale(LC_ALL, "C.UTF-8");

	// setup signal handling
	struct sigaction signal;
	signal.sa_handler = signal_handler;
	sigemptyset(&signal.sa_mask);
	signal.sa_flags = 0;

	sigaction(SIGHUP,  &signal, NULL);
	sigaction(SIGINT,  &signal, NULL);
	sigaction(SIGQUIT, &signal, NULL);
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
			g_printerr("GLib Threading cannot be initialized!");
			exit(1);
		}
	}

	// init conditions & mutexes
	g_lck_termination = g_mutex_new();
	g_lck_t1 = g_mutex_new();
	g_cnd_t1 = g_cond_new();
	g_lck_t2 = g_mutex_new();
	g_cnd_t2 = g_cond_new();

	// start & wait
	GThread* t1 = g_thread_create(Func1, NULL, TRUE, NULL);
	GThread* t2 = g_thread_create(Func2, NULL, TRUE, NULL);

	g_printf("F0\n");

	g_thread_join(t1);
	g_thread_join(t2);

	// cleanup conditions & mutexes
	g_cond_free(g_cnd_t1);
	g_mutex_free(g_lck_t1);
	g_cond_free(g_cnd_t2);
	g_mutex_free(g_lck_t2);
	g_mutex_free(g_lck_termination);

	g_printf("ENDING\n");

	return 0;
}
