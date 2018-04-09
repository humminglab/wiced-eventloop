/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

wiced_result_t a_dev_temp_humid_get(float *temp, float *humid);

void a_dev_set_usb_tester(int index);

uint16_t a_dev_adc_get(int adc);
wiced_result_t a_dev_adc_init(void);

wiced_result_t a_dev_temp_humid_init(void);
