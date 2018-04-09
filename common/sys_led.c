/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"

#include "eventloop.h"
#include "sys_led.h"

static void set_led(int gpio, wiced_bool_t on)
{
	if (on)
		wiced_gpio_output_high(gpio);
	else
		wiced_gpio_output_low(gpio);
}

static void timer_callback(void *arg)
{
	int i;
	sys_led_t *s = arg;

	s->blink_cur = !s->blink_cur;
	for (i = 0; i < s->max_leds; i++) {
		if (s->modes[i] == LED_BLINK) {
			set_led(s->gpios[i], s->blink_cur);
		}
	}
}

wiced_result_t a_sys_led_init(sys_led_t* s, eventloop_t *e, int blink_interval, const int *gpios, int max_leds)
{
	if (max_leds > MAX_LED)
		return WICED_ERROR;
	
	memset(s, 0, sizeof(*s));
	s->evt = e;
	s->blink_interval = blink_interval;
	s->gpios = gpios;
	s->max_leds = max_leds;
	return WICED_SUCCESS;
}

wiced_result_t a_sys_led_set(sys_led_t* s, int index, led_mode_t mode)
{
	if (s->modes[index] == LED_BLINK && mode != LED_BLINK) {
		if (--s->num_blink_led == 0) {
			a_eventloop_deregister_timer(s->evt, &s->timer_node);
		}
	} else if (s->modes[index] != LED_BLINK && mode == LED_BLINK) {
		if (s->num_blink_led++ == 0) {
 			a_eventloop_register_timer(s->evt, &s->timer_node,
				   timer_callback, s->blink_interval, s);
			s->blink_cur = WICED_TRUE;
		}
	}

	s->modes[index] = mode;
	set_led(s->gpios[index], mode == LED_ON || (mode == LED_BLINK && s->blink_cur));
	return WICED_SUCCESS;
}
