/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"

#include "eventloop.h"
#include "sys_worker.h"

static wiced_result_t sensor_process(void *arg)
{
	sys_worker_t *s = arg;
	(*s->worker_fn)(s->arg);
	a_eventloop_set_flag(s->evt, s->event_flag);
	return WICED_SUCCESS;
}

static void timer_callback(void *arg)
{
	sys_worker_t *s = arg;
	a_eventloop_deregister_timer(s->evt, &s->timer_node);
	wiced_rtos_send_asynchronous_event(&s->worker_thread, sensor_process, s);
}

static void event_callback(void *arg)
{
	sys_worker_t *s = arg;
	(s->finish_fn)(s->arg);
	a_eventloop_register_timer(s->evt, &s->timer_node, timer_callback, s->interval_ms, s);
}

wiced_result_t a_sys_worker_trigger(sys_worker_t *s)
{
	if (a_eventloop_get_timer_fn(s->evt, &s->timer_node)) {
		timer_callback(s);
	}
	return WICED_SUCCESS;
}

wiced_result_t a_sys_worker_init(sys_worker_t *s, eventloop_t *e, uint32_t event_flag, int interval_ms,
				 sys_worker_fn worker_fn, sys_worker_fn finish_fn, void *arg)
{
	memset(s, 0, sizeof(*s));
	s->evt = e;
	s->event_flag = event_flag;
	s->interval_ms = interval_ms;
	s->worker_fn = worker_fn;
	s->finish_fn = finish_fn;
	s->arg = arg;

	a_eventloop_register_timer(s->evt, &s->timer_node, timer_callback, s->interval_ms, s);
	a_eventloop_register_event(s->evt, &s->event_node, event_callback, s->event_flag, s);
	wiced_rtos_create_worker_thread(&s->worker_thread, WICED_DEFAULT_WORKER_PRIORITY, 1024, 2);
	
	return WICED_SUCCESS;
}

