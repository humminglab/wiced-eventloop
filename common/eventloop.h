/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

#include "linked_list.h"

typedef void (*eventloop_timer_fn)(void *);
typedef void (*eventloop_event_fn)(void *);

/* continuous timer node */
typedef struct {
	linked_list_node_t node;
	eventloop_timer_fn fn;	/* callback function */
	uint32_t interval;	/* interval */
	wiced_time_t next_timeout; /* next timeout */
} eventloop_timer_node_t;

typedef struct {
	linked_list_node_t node;
	wiced_bool_t enabled;
	uint32_t mask; /* bitmap mask */
	eventloop_event_fn fn; 	/* callback function */
} eventloop_event_node_t;

typedef struct {
	wiced_event_flags_t events; /* event flag */
	/* const eventloop_event_fn *event_fns; /\* event callback functions *\/ */
	/* int max_events;			/\* number of event_fns *\/ */
	linked_list_t timer_list;	/* timer list */
	linked_list_t event_list;	/* event list */

	wiced_bool_t loop_stop;	/* flag for loop stop */
	uint32_t pending_events;	/* current events */
} eventloop_t ;

wiced_result_t a_eventloop_register_timer(eventloop_t* el,
					  eventloop_timer_node_t* node_insert,
					  eventloop_timer_fn fn,
					  uint32_t interval,
					  void* arg);
wiced_result_t a_eventloop_deregister_timer(eventloop_t* el, eventloop_timer_node_t* node_remove);
eventloop_timer_fn a_eventloop_get_timer_fn(eventloop_t* el, eventloop_timer_node_t* node);
wiced_result_t a_eventloop_set_flag(eventloop_t* el, uint32_t event);
wiced_result_t a_eventloop_register_event(eventloop_t* el,
					  eventloop_event_node_t* node_insert,
					  eventloop_event_fn fn,
					  uint32_t mask,
					  void* arg);
wiced_result_t a_eventloop_deregister_event(eventloop_t* el, eventloop_event_node_t* node_remove);
wiced_result_t a_eventloop(eventloop_t* el, uint32_t timeout_ms);
wiced_result_t a_eventloop_init(eventloop_t *el);
static inline void a_eventloop_break(eventloop_t* el)
{
	el->loop_stop = WICED_TRUE;
}
static inline void a_eventloop_disable_event(eventloop_t* el, eventloop_event_node_t* node)
{
	node->enabled = WICED_TRUE;
}

static inline void a_eventloop_enable_event(eventloop_t* el, eventloop_event_node_t* node)
{
	node->enabled = WICED_FALSE;
}
