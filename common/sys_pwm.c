/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"
#include <math.h>

#include "eventloop.h"
#include "sys_pwm.h"

static void set_pwm(wiced_pwm_t pwm_id, float val)
{
	wiced_pwm_init(pwm_id, 3000, val);
	wiced_pwm_start(pwm_id);
}

static int square_val(int start, int end, int total_step, int step)
{
	int diff = end - start;
	int max;
	int cur;
	int v;
	if (diff < 0) {
		step  = total_step - step;
	}

	cur = step * step;
	max = total_step * total_step;
	v = (int)round((double)diff * (double)cur / (double)max);

	if (diff >= 0){
		v += start;
	} else {
		v = end - v;
	}
	return v;
}

static void timer_callback(void *arg)
{
	sys_pwm_t *s = arg;

	a_eventloop_deregister_timer(s->evt, &s->timer_node);
	if (++s->step >= s->total_step || s->cur_val == s->target_val) {
		set_pwm(s->pwm_id, s->target_val);
		s->cur_val = s->target_val;
		return;
	}

	s->cur_val = square_val(s->start_val, s->target_val, s->total_step, s->step);
	set_pwm(s->pwm_id, s->cur_val);

	a_eventloop_register_timer(s->evt, &s->timer_node,
				   timer_callback, s->interval_ms, s);
}

wiced_result_t a_sys_pwm_init(sys_pwm_t* s, eventloop_t *e, wiced_pwm_t pwm_id, int interval_ms, int total_step)
{
	memset(s, 0, sizeof(*s));
	s->evt = e;
	s->pwm_id = pwm_id;
	s->interval_ms = interval_ms;
	s->total_step = total_step;

	set_pwm(s->pwm_id, 0);
	return WICED_SUCCESS;
}

wiced_result_t a_sys_pwm_level(sys_pwm_t* s, int level)
{
	s->start_val = s->cur_val;
	s->target_val = level;
	s->step = 1;

	timer_callback(s);
	return WICED_SUCCESS;
}
