#include "messaging.h"

#include "global.h"
#include "module.h"
#include "queue.h"
#include "thread.h"

#include <api_core.h>
#include <api_core_config.h>
//#include <api_core_messaging.h>

module_vtable_msg_input* core_messaging_handlers_input() {
	return (module_vtable_msg_input*) g_handlers_input->vtable;
}

module_vtable_msg_output* core_messaging_handlers_output() {
	return (module_vtable_msg_output*) g_handlers_output->vtable;
}

module_vtable_msg_trash* core_messaging_handlers_trash() {
	return (module_vtable_msg_trash*) g_handlers_trash->vtable;
}

gboolean core_messaging_module_init(module_type type, GError** error) {
	*error = NULL;

	// prepare variant options
	gchar*		conf = NULL;
	ModuleFunc	entry = NULL;
	gchar*		func = NULL;
	switch (type) {
		case MODULE_TYPE_MESSAGE_INPUT:
			conf = g_strdup("input_module");
			func = g_strdup("InputModule");
			break;
		case MODULE_TYPE_MESSAGE_OUTPUT:
			conf = g_strdup("output_module");
			func = g_strdup("OutputModule");
			break;
		case MODULE_TYPE_MESSAGE_TRASH:
			conf = g_strdup("trash_module");
			func = g_strdup("TrashModule");
			break;
		default:
			g_set_error(error, 0, 0, "Invalid messaging module type %d!", type);
			return FALSE;
	}

	// determine input module from settings
	if (!core_config_isset("core", conf)) {
		g_set_error(error, 0, 0, "Setting 'core.%s' was not specified for this instance (%d)", conf, core_instance());

		g_free(conf);
		g_free(func);

		return FALSE;
	}

	// load module
	gchar* module_name = core_config_get_text("core", conf);
	module_loaded* module = core_module_load(type, module_name, error);
	g_free(module_name);
	g_free(conf);

	if (module == NULL) {
		g_free(func);
		return FALSE;
	}

	// find xxxModule function
	g_module_symbol(module->module, func, (gpointer*) &entry);
	g_free(func);

	if (entry == NULL)
		return FALSE;

	// get vtable from module
	module_vtable* ret = entry();
	if (ret == NULL)
		return FALSE;

	// assign vtable to global pointer
	switch (type) {
		case MODULE_TYPE_MESSAGE_INPUT:
			module->vtable = ret;
			g_handlers_input = module;
			break;
		case MODULE_TYPE_MESSAGE_OUTPUT:
			module->vtable = ret;
			g_handlers_output = module;
			break;
		case MODULE_TYPE_MESSAGE_TRASH:
			module->vtable = ret;
			g_handlers_trash = module;
			break;
		default:
			return FALSE;
	}

	// do checking of assigned handlers
	return core_messaging_module_check(module, error);
}

gboolean core_messaging_module_check(module_loaded* module, GError** error) {
	module_vtable_msg_input*	tbl_i = NULL;
	module_vtable_msg_output*	tbl_o = NULL;
	module_vtable_msg_trash*	tbl_t = NULL;

	// check consumer handlers
	switch (module->info->type) {
		case MODULE_TYPE_MESSAGE_INPUT:
			tbl_i = (module_vtable_msg_input*) module->vtable;
			if (tbl_i->handler_receive_feedback == NULL) {
				g_set_error(error, 0, 0, "Handler 'receive_feedback' was not set!");
				return FALSE;
			}
			break;
		case MODULE_TYPE_MESSAGE_OUTPUT:
			tbl_o = (module_vtable_msg_output*) module->vtable;
			if (tbl_o->handler_receive_forward == NULL) {
				g_set_error(error, 0, 0, "Handler 'receive_forward' was not set!");
				return FALSE;
			}
			break;
		case MODULE_TYPE_MESSAGE_TRASH:
			tbl_t = (module_vtable_msg_trash*) module->vtable;
			if (tbl_t->handler_receive_trash == NULL) {
				g_set_error(error, 0, 0, "Handler 'receive_trash' was not set!");
				return FALSE;
			}
			break;
		default:
			return FALSE;
	}

	// nothing more to check for TRASH module
	if (
		module->info->type != MODULE_TYPE_MESSAGE_INPUT
			&&
		module->info->type != MODULE_TYPE_MESSAGE_OUTPUT
	)
		return TRUE;

	// check producer handlers
	module_vtable_msg* tbl = (module_vtable_msg*) module->vtable;

	// check for function in vtable
	if (tbl->producer_type == NULL) {
		g_set_error(error, 0, 0, "Function 'producer_type' not set in module '%d:%s'!", module->info->type, module->info->name);
		return FALSE;
	}

	// get producer type
	module_producer_type mode = tbl->producer_type();
	// validate
	if (
		(mode & MODULE_PRODUCER_TYPE_PUSH) == 0
			&&
		(mode & MODULE_PRODUCER_TYPE_PULL) == 0
	) {
		g_set_error(error, 0, 0, "At least one of PUSH/PULL producer types has to be set for module '%d:%s'", module->info->type, module->info->name);
		return FALSE;
	}

	// check for PUSH invokers, if module is claiming, that he are going to use it
	if ((mode & MODULE_PRODUCER_TYPE_PUSH) != 0) {
		// get target symbol
		gpointer target = NULL;
		switch (module->info->type) {
			case MODULE_TYPE_MESSAGE_INPUT:
				target = tbl_i->invoker_push_forward;
				break;
			case MODULE_TYPE_MESSAGE_OUTPUT:
				target = tbl_o->invoker_push_feedback;
				break;
			default:
				target = NULL;
		}
		// check
		if (target == NULL) {
			g_set_error(error, 0, 0, "PUSH invoker not set for module '%d:%s'", module->info->type, module->info->name);
			return FALSE;
		}
	}

	// check for PULL handlers, if module is claiming, that he are going to use it
	if ((mode & MODULE_PRODUCER_TYPE_PULL) != 0) {
		// get target symbol
		gpointer target = NULL;
		switch (module->info->type) {
			case MODULE_TYPE_MESSAGE_INPUT:
				target = tbl_i->handler_pull_forward;
				break;
			case MODULE_TYPE_MESSAGE_OUTPUT:
				target = tbl_o->handler_pull_feedback;
				break;
			default:
				target = NULL;
		}
		// check
		if (target == NULL) {
			g_set_error(error, 0, 0, "PULL handler not set for module '%d:%s'", module->info->type, module->info->name);
			return FALSE;
		}
	}

	return TRUE;
}

gboolean core_terminated() {
	g_mutex_lock(g_lck_termination);
	if (g_termination) {
		g_mutex_unlock(g_lck_termination);
		return TRUE;
	}
	g_mutex_unlock(g_lck_termination);

	return FALSE;
}

void core_messaging_init() {
	// === init queues
	core_queues_init();

	// === init thread locks

	// termination
	g_lck_termination = g_mutex_new();

	// forward_receive
	g_lck_forward_receive = g_mutex_new();
	g_cnd_forward_receive = g_cond_new();
}

void core_messaging_destroy() {
	// === destroy queues
	core_queues_destroy();

	// === destroy thread locks

	// termination
	g_mutex_free(g_lck_termination);

	// forward_receive
	g_mutex_free(g_lck_forward_receive);
	g_cond_free(g_cnd_forward_receive);
}

void core_messaging_start() {
	// first init reception of trash messages
	GThread* t_trash_receive = NULL;
	t_trash_receive = g_thread_create(thread_trash_receive, NULL, TRUE, NULL);

	// reception of feedback
	GThread* t_feeback_receive = NULL;
	t_feeback_receive = g_thread_create(thread_feeback_receive, NULL, TRUE, NULL);

	// get output module producer type
	module_producer_type mode_out = ((module_vtable_msg*) g_handlers_output->vtable)->producer_type();

	// sending of feedback (pull)
	GThread* t_feeback_pull = NULL;
	if ((mode_out & MODULE_PRODUCER_TYPE_PULL) != 0)
		t_feeback_pull = g_thread_create(thread_feeback_pull, NULL, TRUE, NULL);

	// sending of feedback (push)
	GThread* t_feeback_push = NULL;
	if ((mode_out & MODULE_PRODUCER_TYPE_PUSH) != 0)
		t_feeback_push = g_thread_create(thread_feeback_push, NULL, TRUE, NULL);

	// reception of forward
	GThread* t_forward_receive = NULL;
	t_forward_receive = g_thread_create(thread_forward_receive, NULL, TRUE, NULL);

	// get input module producer type
	module_producer_type mode_in = ((module_vtable_msg*) g_handlers_input->vtable)->producer_type();

	// sending of forward (pull)
	GThread* t_forward_pull = NULL;
	if ((mode_in & MODULE_PRODUCER_TYPE_PULL) != 0)
		t_forward_pull = g_thread_create(thread_forward_pull, NULL, TRUE, NULL);

	// sending of forward (push)
	GThread* t_forward_push = NULL;
	if ((mode_in & MODULE_PRODUCER_TYPE_PUSH) != 0)
		t_forward_push = g_thread_create(thread_forward_push, NULL, TRUE, NULL);

	// join all threads with main
	g_thread_join(t_trash_receive);
	g_thread_join(t_feeback_receive);
	if (t_feeback_pull != NULL)
		g_thread_join(t_feeback_pull);
	if (t_feeback_push != NULL)
		g_thread_join(t_feeback_push);
	g_thread_join(t_forward_receive);
	if (t_forward_pull != NULL)
		g_thread_join(t_forward_pull);
	if (t_forward_push != NULL)
		g_thread_join(t_forward_push);
}

// core message handlers

void core_handler_push_forward(message_batch* data) {
	gint qlen_before = core_queue_length(g_queue_forward);

	gint pushed = 0;
	message_batch* iter = data;
	while (iter != NULL) {
		// get current message
		message* msg = iter->data;
		if (msg == NULL) {
			// skipping non-set message (shouldn't occur)
			iter = g_slist_next(iter);
			continue;
		}

		// add message to forward queue
		core_queue_push(g_queue_forward, msg);
		pushed++;

		// next
		iter = g_slist_next(iter);
	}

	g_slist_free(data);

	gint qlen_after = core_queue_length(g_queue_forward);

	// if nothing was in queue and we pushed everyrything what's there now (means, nobody other has pushed in meantime)
	if (qlen_before == 0 && qlen_after > 0 && qlen_after == pushed) {
		// waking up receiver
		g_mutex_lock(g_lck_forward_receive);
		g_cond_signal(g_cnd_forward_receive);
		g_mutex_unlock(g_lck_forward_receive);
	}
}
