/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"

#include "eventloop.h"
#include "sys_uart.h"

wiced_result_t a_sys_uart_init(sys_uart_t* s, wiced_uart_t uart, int baud_rate)
{
	memset(s, 0, sizeof(*s));
	s->uart = uart;
	s->uart_config.baud_rate = baud_rate;
	s->uart_config.data_width = DATA_WIDTH_8BIT;
	s->uart_config.parity = NO_PARITY;
	s->uart_config.stop_bits = STOP_BITS_1;
	s->uart_config.flow_control = FLOW_CONTROL_DISABLED;

	ring_buffer_init(&s->rx_buffer, s->rx_data, UART_RX_BUFFER_SIZE);
	return wiced_uart_init(uart, &s->uart_config, &s->rx_buffer);
}

static wiced_result_t recv_worker(void *arg)
{
	sys_uart_t *s = arg;
	uint32_t size;
	wiced_result_t result;

	while (1) {
		size = 1;
		result = wiced_uart_receive_bytes(s->uart, &s->last_data, &size, WICED_WAIT_FOREVER);
		if (result == WICED_SUCCESS)
			break;
	}
	a_eventloop_set_flag(s->evt, s->flag);
	return WICED_SUCCESS;
}

static void recv_callback(void *arg)
{
	sys_uart_t *s = arg;
	char buf[UART_RX_BUFFER_SIZE];
	wiced_result_t result;
	int pos = 1;

	buf[0] = s->last_data;
	do {
		uint32_t size = 1;
		result = wiced_uart_receive_bytes(s->uart, &buf[pos++], &size, WICED_NO_WAIT);
	} while (result == WICED_SUCCESS);

	(*s->fn)(s->arg, buf, (uint32_t)(pos-1));
	wiced_rtos_send_asynchronous_event(&s->worker, recv_worker, s);
}

wiced_result_t a_sys_uart_register_event(sys_uart_t* s, eventloop_t *e, uint32_t event_flag,
					 sys_uart_callback_fn fn, void* arg)
{
	s->evt = e;
	s->fn = fn;
	s->arg = arg;
	s->flag = event_flag;

	a_eventloop_register_event(s->evt, &s->evt_node, recv_callback, s->flag, s);
	wiced_rtos_create_worker_thread(&s->worker, WICED_DEFAULT_WORKER_PRIORITY, 1024, 1);
	wiced_rtos_send_asynchronous_event(&s->worker, recv_worker, s);
	return WICED_SUCCESS;
}
