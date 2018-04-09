/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

typedef void (*a_network_callback_t)(wiced_bool_t, void*);

wiced_result_t a_network_register_callback(a_network_callback_t callback, void *arg);
extern wiced_bool_t a_network_is_assoc(void);
extern wiced_bool_t a_network_is_up(void);
extern void a_network_start(void);
extern void a_network_stop(void);
extern void a_network_restart(void);
extern void a_network_init(void);
