/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"

#include "eventloop.h"

#define ALL_EVENTS	(~0UL)

static wiced_bool_t compare_node(linked_list_node_t* node_to_compare,
				 void* user_data)
{
	linked_list_node_t *base = (linked_list_node_t*)user_data;
	return node_to_compare == base;
}

static wiced_bool_t is_node_in(linked_list_t* list, linked_list_node_t* node)
{
	linked_list_node_t *tmp;
	return (linked_list_find_node(list, compare_node, (void*)node, &tmp)
		== WICED_SUCCESS) ? WICED_TRUE : WICED_FALSE;
}

static wiced_result_t insert_node_safe(linked_list_t* list, linked_list_node_t* node)
{	/* insert only if node is not in linked list */
	return is_node_in(list, node) ? WICED_SUCCESS :
		linked_list_insert_node_at_front(list, node);
}

static wiced_result_t remove_node_safe(linked_list_t* list, linked_list_node_t* node)
{
	return is_node_in(list, node) ? linked_list_remove_node(list, node) :
		WICED_ERROR;
}

static void process_timer(eventloop_t *el)
{
	wiced_time_t now;
	eventloop_timer_node_t* node;
	wiced_time_get_time(&now);
_recheck:
	linked_list_get_front_node(&el->timer_list, (linked_list_node_t**)&node);
	while (node) {
		if ((int)(node->next_timeout - now) <= 0) {
			node->next_timeout = now + node->interval;
			(*node->fn)(node->node.data);
			goto _recheck;
		}
		node = (eventloop_timer_node_t*)node->node.next;
	}			
}

static uint32_t get_first_timeout(eventloop_t *el)
{
	wiced_time_t now;
	eventloop_timer_node_t* node;
	uint32_t timeout = WICED_WAIT_FOREVER;
	wiced_time_get_time(&now);

	linked_list_get_front_node(&el->timer_list, (linked_list_node_t**)&node);
	while (node) {
		int diff = (int)(node->next_timeout - now);
		if (diff <= 0)
			return WICED_NO_WAIT;
		if (timeout > (unsigned int)diff)
			timeout = (unsigned int)diff;
		node = (eventloop_timer_node_t*)node->node.next;
	}
	return timeout;
}

wiced_result_t a_eventloop_register_timer(eventloop_t* el,
					 eventloop_timer_node_t* node_insert,
					 eventloop_timer_fn fn,
					 uint32_t interval,
					 void* arg)
{
	wiced_time_t now;
	if (interval == WICED_WAIT_FOREVER)
		return a_eventloop_deregister_timer(el, node_insert);

	node_insert->fn = fn;
	node_insert->interval = interval;
	node_insert->node.data = arg;
	wiced_time_get_time(&now);
	node_insert->next_timeout = now + interval;
	return insert_node_safe(&el->timer_list, &node_insert->node);
}

wiced_result_t a_eventloop_deregister_timer(eventloop_t* el,
					    eventloop_timer_node_t* node_remove)
{
	return remove_node_safe(&el->timer_list, &node_remove->node);
}

eventloop_timer_fn a_eventloop_get_timer_fn(eventloop_t* el,
					 eventloop_timer_node_t* node)
{
	wiced_result_t res;
	linked_list_node_t *node_found;
	res = linked_list_find_node(&el->timer_list, compare_node,
				    (void*)node, &node_found);
	if (res == WICED_SUCCESS) {
		return node->fn;
	}
	return NULL;
}

wiced_result_t a_eventloop_register_event(eventloop_t* el,
					  eventloop_event_node_t* node_insert,
					  eventloop_event_fn fn,
					  uint32_t mask,
					  void* arg)
{
	node_insert->fn = fn;
	node_insert->mask = mask;
	node_insert->enabled = WICED_TRUE;
	node_insert->node.data = arg;
	return insert_node_safe(&el->event_list, (linked_list_node_t*)node_insert);
}

wiced_result_t a_eventloop_deregister_event(eventloop_t* el,
					    eventloop_event_node_t* node_remove)
{
	return remove_node_safe(&el->timer_list, &node_remove->node);
}

wiced_result_t a_eventloop_set_flag(eventloop_t* el, uint32_t event)
{
	return wiced_rtos_set_event_flags(&el->events, event);
}

wiced_result_t a_eventloop(eventloop_t* el, uint32_t timeout_ms)
{
	wiced_result_t result;
	uint32_t events;
	uint32_t timeout;
	wiced_time_t start_time, now;

	wiced_time_get_time(&start_time);

	while (1) {
		if (el->pending_events) {
			timeout = WICED_NO_WAIT;
		} else {
			timeout = get_first_timeout(el);

			wiced_time_get_time(&now);
			if (timeout_ms != WICED_WAIT_FOREVER) {
				uint32_t diff = now - start_time;
				if (timeout > diff)
					timeout = diff;
			}
		}
		
		events = 0;
		result = wiced_rtos_wait_for_event_flags(&el->events, ALL_EVENTS,
			     &events, WICED_TRUE, WAIT_FOR_ANY_EVENT, timeout);
		if (result == WICED_SUCCESS) {
			el->pending_events |= events;
		}
		if (el->pending_events) {
			eventloop_event_node_t* node;
			linked_list_get_front_node(&el->event_list, (linked_list_node_t**)&node);
			while (node) {
				if (node->enabled && (node->mask & el->pending_events)) {
					el->pending_events &= ~node->mask;
					(*node->fn)(node->node.data);
				}
				node = (eventloop_event_node_t*)node->node.next;
			}
		}
		if (el->loop_stop) {
			el->loop_stop = WICED_FALSE;
			return WICED_SUCCESS;
		}

		process_timer(el);
		wiced_time_get_time(&now);
		if (timeout_ms != WICED_WAIT_FOREVER && now - start_time >= timeout_ms) {
			el->loop_stop = WICED_FALSE;
			return WICED_ERROR;
		}
	}
}

wiced_result_t a_eventloop_init(eventloop_t *el)
{
	memset(el, 0, sizeof(eventloop_t));
	wiced_rtos_init_event_flags(&el->events);
	linked_list_init(&el->timer_list);
	linked_list_init(&el->event_list);
	return WICED_TRUE;
}
