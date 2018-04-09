/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"

#include "device.h"
#include "app_dct.h"
#include "maxim_ssi.h"

#define UART_RX_BUFFER_SIZE 64

static const wiced_i2c_device_t temp_i2c_device =
{
	.port          = WICED_I2C_1,
	.address       = 0x45,
	.address_width = I2C_ADDRESS_WIDTH_7BIT,
	.speed_mode    = I2C_HIGH_SPEED_MODE,
};

static const wiced_uart_config_t uart_measure_config =
{
	.baud_rate    = 9600,
	.data_width   = DATA_WIDTH_8BIT,
	.parity       = NO_PARITY,
	.stop_bits    = STOP_BITS_1,
	.flow_control = FLOW_CONTROL_DISABLED
};

static wiced_result_t temp_humid_init(void)
{
	wiced_result_t status;
	static const uint8_t data_out_break[] = {0x30, 0x93}; /* break; */
	static const uint8_t data_out[] = {0x30, 0xa2}; /* soft reset */
	wiced_i2c_message_t message;
	wiced_i2c_init(&temp_i2c_device);
	wiced_i2c_init_tx_message(&message, data_out_break, sizeof(data_out_break), 3, 1);
	status = wiced_i2c_transfer(&temp_i2c_device, &message, 1);
	wiced_rtos_delay_milliseconds(1);
	wiced_i2c_init_tx_message(&message, data_out, sizeof(data_out), 3, 1);
	status = wiced_i2c_transfer(&temp_i2c_device, &message, 1);
	wiced_rtos_delay_milliseconds(1);
	if (status != WICED_SUCCESS)
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail with temp & humidity: %d\n", status);
	return status;
}

static wiced_result_t a_dev_temp_humid_measure_periodic_start(void)
{
	wiced_result_t status;
	static const uint8_t data_out[] = {0x20, 0x32}; /* high, clk stretch */
	wiced_i2c_message_t message;
	wiced_i2c_init_tx_message(&message, data_out, sizeof(data_out), 3, 1);
	status = wiced_i2c_transfer(&temp_i2c_device, &message, 1);
	if (status != WICED_SUCCESS)
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail with temp & humidity trigger: %d\n",
			      status);
	return status;
}

wiced_result_t a_dev_temp_humid_get(float *temp, float *humid)
{
	int v;
	uint8_t data_out[] = { 0xe0, 0x00 }; /* read */
	uint8_t data_in[6];
	wiced_result_t status;
	wiced_i2c_message_t message;
	static float last_temp, last_humid;

	wiced_i2c_init_tx_message(&message, data_out, sizeof(data_out), 3, 1);
	wiced_i2c_transfer(&temp_i2c_device, &message, 1);

	wiced_i2c_init_rx_message(&message, data_in, 6, 1, 1);
	status = wiced_i2c_transfer(&temp_i2c_device, &message, 1);

	if (status != WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail with temp & humidity reading: %d\n",
			      status);
		*temp = last_temp;
		*humid = last_humid;
		return status;
	}

	v = (data_in[0] << 8) + data_in[1];
	*temp = (float)(-45. + 175. * v / 65535.);
	v = (data_in[3] << 8) + data_in[4];
	*humid = (float)(100. * v / 65535.);

	last_temp = *temp;
	last_humid = *humid;
	return WICED_SUCCESS;
}

uint16_t a_dev_adc_get(int adc)
{
	uint16_t v;
	wiced_adc_take_sample(adc, &v);
	return v;
}

wiced_result_t a_dev_adc_init(void)
{
	int i, j;
	uint16_t dummy = 0;
	for (i = 0; i < WICED_ADC_MAX; i++) {
		wiced_adc_init(WICED_ADC_1 + i, 5);
	}
	for (i = 0; i < WICED_ADC_MAX; i++) {
		for (j = 0; j < 3; j++)
			wiced_adc_take_sample(WICED_ADC_1 + i, &dummy);
	}
	return WICED_SUCCESS;
}

wiced_result_t a_dev_temp_humid_init(void)
{
	wiced_result_t result;
	if ((result = temp_humid_init()) != WICED_SUCCESS)
		return result;

	result = a_dev_temp_humid_measure_periodic_start();
	return result;
}
