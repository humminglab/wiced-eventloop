/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"

#include "util.h"
#include "network.h"


#define NET_RETRY_INTERVAL	(2 * 1000)

struct network_t {
	wiced_timed_event_t tevent;
	
	wiced_bool_t assoc;
	wiced_bool_t up;
};

static struct network_t a_net;

struct callback_tbl_t {
	a_network_callback_t callback;
	void* arg;
};
	
static struct callback_tbl_t callback_tbl[5];

static void call_callback(wiced_bool_t up);

static wiced_result_t __network_change(void* arg)
{
	wiced_ip_address_t ip;

	UNUSED_PARAMETER(arg);
	if (wiced_network_is_up(WICED_STA_INTERFACE) &&
	    wiced_network_is_ip_up(WICED_STA_INTERFACE) && 
	    wiced_ip_get_ipv4_address(WICED_STA_INTERFACE, &ip) == WICED_SUCCESS &&
	    ip.ip.v4 != 0) {
		if (a_net.up != WICED_TRUE) {
			a_net.up = WICED_TRUE;
			call_callback(a_net.up);
		}

	}
	return WICED_SUCCESS;
}

static void _network_change(void* arg)
{
	UNUSED_PARAMETER(arg);

	wiced_rtos_send_asynchronous_event(WICED_NETWORKING_WORKER_THREAD,
					   __network_change, 0);
}

static void _link_up_callback(void)
{
	a_net.assoc = WICED_TRUE;
	/* do nothing */
}

static void _link_down_callback(void)
{
	a_net.assoc = WICED_FALSE;

	if (a_net.up == WICED_FALSE)
		return;

	a_net.up = WICED_FALSE;
	call_callback(a_net.up);
}

static wiced_result_t _network_init(void* arg)
{
	wiced_result_t result;
	UNUSED_PARAMETER(arg);

	if (a_net.tevent.function) {
		wiced_rtos_deregister_timed_event(&a_net.tevent);
	}

	result = wiced_network_up(WICED_STA_INTERFACE,
				  WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
	if (result == WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Network Start\n");
		a_net.assoc = WICED_TRUE;
		_network_change(0);
		return WICED_SUCCESS;
	}

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO,"Fail net binding... retry after %d  secs\n",
		      NET_RETRY_INTERVAL/1000);
	wiced_rtos_register_timed_event(&a_net.tevent, WICED_NETWORKING_WORKER_THREAD,
					_network_init, NET_RETRY_INTERVAL, 0);
	return WICED_SUCCESS;
}

static void call_callback(wiced_bool_t up)
{
	size_t i;
	for (i = 0; i < N_ELEMENT(callback_tbl); i++) {
		if (callback_tbl[i].callback)
			(*callback_tbl[i].callback)(up, callback_tbl[i].arg);
	}
}

wiced_result_t a_network_register_callback(a_network_callback_t callback, void *arg)
{
	size_t i;
	for (i = 0; i < N_ELEMENT(callback_tbl); i++) {
		if (!callback_tbl[i].callback) {
			callback_tbl[i].callback = callback;
			callback_tbl[i].arg = arg;
			break;
		}
	}
	return (i < N_ELEMENT(callback_tbl)) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_bool_t a_network_is_assoc(void)
{
	return a_net.assoc;
}

wiced_bool_t a_network_is_up(void)
{
	return a_net.up;
}

void a_network_start(void)
{
	wiced_rtos_send_asynchronous_event(WICED_NETWORKING_WORKER_THREAD,
					   _network_init, 0);
}

void a_network_stop(void)
{
	/* FIXME: can not do network down, if any socket is opned. */
	if (a_net.tevent.function)
		wiced_rtos_deregister_timed_event(&a_net.tevent);
	wiced_network_down(WICED_STA_INTERFACE);
}

void a_network_restart(void)
{
	a_network_stop();
	a_network_start();
}

void a_network_init(void)
{
	wiced_result_t result; 
	result = wiced_ip_register_address_change_callback(_network_change, 0);
	if (result != WICED_SUCCESS)
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR,
			      "Error!!! wiced_ip_register_address_change_callback()\n");
	result = wiced_network_register_link_callback(_link_up_callback, _link_down_callback,
						      WICED_STA_INTERFACE);
	if (result != WICED_SUCCESS)
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR,
			      "Error!!! wiced_network_register_link_callback()\n");

	wiced_rtos_send_asynchronous_event(WICED_NETWORKING_WORKER_THREAD,
					   _network_init, 0);
	
}
