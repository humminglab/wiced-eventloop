/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

typedef void (*sys_button_callback_fn)(void *arg, wiced_bool_t on);

typedef struct {
	platform_button_t button;
	uint32_t flag;

	eventloop_t *evt;
	eventloop_event_node_t evt_node;

	sys_button_callback_fn fn;
	void *arg;
} sys_button_t;

wiced_result_t a_sys_button_init(sys_button_t* s, platform_button_t button_id, eventloop_t* e,
				 uint32_t event_flag, sys_button_callback_fn fn, void* arg);
