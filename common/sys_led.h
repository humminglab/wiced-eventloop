/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

#define MAX_LED		12

typedef enum {
	LED_OFF,
	LED_ON,
	LED_BLINK
} led_mode_t;

typedef struct {
	eventloop_t *evt;
	eventloop_timer_node_t timer_node;
	int blink_interval;
	const int *gpios;
	int max_leds;

	wiced_bool_t num_blink_led;
	wiced_bool_t blink_cur;

	uint8_t modes[MAX_LED];
} sys_led_t;

wiced_result_t a_sys_led_init(sys_led_t* s, eventloop_t *e, int blink_interval, const int *gpios, int max_leds);
wiced_result_t a_sys_led_set(sys_led_t* s, int index, led_mode_t mode);
