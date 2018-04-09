/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

/* support pwm1 only */

typedef struct {
	eventloop_t *evt;
	eventloop_timer_node_t timer_node;
	wiced_pwm_t pwm_id;

	int target_val;
	int cur_val;
	int start_val;
	
	int interval_ms;
	int total_step;
	
	int step;
} sys_pwm_t;

wiced_result_t a_sys_pwm_init(sys_pwm_t* s, eventloop_t *e, wiced_pwm_t pwm_id, int interval_ms, int total_step);
wiced_result_t a_sys_pwm_level(sys_pwm_t* s, int level);
