/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

#define UART_RX_BUFFER_SIZE 64

typedef void (*sys_uart_callback_fn)(void *arg, const char* buf, uint32_t size);

typedef struct {
	wiced_uart_t uart;
	uint32_t flag;
	wiced_ring_buffer_t rx_buffer;
	uint8_t rx_data[UART_RX_BUFFER_SIZE];
	wiced_uart_config_t uart_config;
	char last_data;

	wiced_worker_thread_t worker;
	eventloop_t *evt;
	eventloop_event_node_t evt_node;

	sys_uart_callback_fn fn;
	void *arg;
} sys_uart_t;


wiced_result_t a_sys_uart_init(sys_uart_t* s, wiced_uart_t uart, int baud_rate);
wiced_result_t a_sys_uart_register_event(sys_uart_t* s, eventloop_t *e, uint32_t event_flag,
					 sys_uart_callback_fn fn, void* arg);
