/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"
#include "platform_button.h"

#include "eventloop.h"
#include "sys_button.h"

static sys_button_t* button_used[PLATFORM_BUTTON_MAX];

static void button_callback(platform_button_t id, wiced_bool_t new_state)
{
	sys_button_t *s = button_used[id];
	if (!s)
		return;
	a_eventloop_set_flag(s->evt, s->flag);
}

static void recv_callback(void *arg)
{
	sys_button_t *s = arg;
	wiced_bool_t state = platform_button_get_value(s->button);
	(*s->fn)(s->arg, state);
}

wiced_result_t a_sys_button_init(sys_button_t* s, platform_button_t button_id, eventloop_t* e,
				 uint32_t event_flag, sys_button_callback_fn fn, void* arg)
{
	static wiced_bool_t inited;

	if (!inited) {
		platform_button_register_state_change_callback(button_callback);
		inited = WICED_TRUE;
	}
	memset(s, 0, sizeof(*s));
	s->button = button_id;
	s->evt = e;
	s->fn = fn;
	s->arg = arg;
	s->flag = event_flag;
	a_eventloop_register_event(s->evt, &s->evt_node, recv_callback, s->flag, s);

	button_used[s->button] = s;
	platform_button_enable(s->button);
	return WICED_SUCCESS;
}
