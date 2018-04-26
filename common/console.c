/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "command_console.h"
#include "wwd_debug.h"
#include "wiced_log.h"

#include "console.h"
#include "app_dct.h"
#include "util.h"
#include "device.h"

int cmd_get_dct(int argc, char* argv[])
{
	app_dct_t* dct;
	platform_dct_wifi_config_t* dct_wifi_config;
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);

	wiced_dct_read_lock((void**)&dct, WICED_FALSE, DCT_APP_SECTION,
			     0, sizeof(app_dct_t));
	WPRINT_APP_INFO(("Server: %s\n", dct->server));
	WPRINT_APP_INFO(("Device Token: %s\n", dct->device_token));
	WPRINT_APP_INFO(("Device ID: %s\n", dct->device_id));
	wiced_dct_read_unlock(dct, WICED_FALSE);

	wiced_dct_read_lock((void**)&dct_wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION,
			    0, sizeof(platform_dct_wifi_config_t));
	WPRINT_APP_INFO(("\nSSID: %s\n", dct_wifi_config->stored_ap_list[0].details.SSID.value));
	WPRINT_APP_INFO(("Auth Type: %s\n", a_get_wlan_sec_name(
				 dct_wifi_config->stored_ap_list[0].details.security)));
	WPRINT_APP_INFO(("PSK Key: %s\n", dct_wifi_config->stored_ap_list[0].security_key));
	wiced_dct_read_unlock((void*)dct_wifi_config, WICED_FALSE);
	return ERR_CMD_OK;
}

int cmd_set_server(int argc, char* argv[])
{
	app_dct_t* dct;
	if (argc < 2)
		return ERR_INSUFFICENT_ARGS;

	wiced_dct_read_lock((void**) &dct, WICED_TRUE, DCT_APP_SECTION,
			     0, sizeof(app_dct_t));
	memset(dct->server, 0, sizeof(dct->server));
	strcpy(dct->server, argv[1]);
	wiced_dct_write((const void*)dct, DCT_APP_SECTION,
			0, sizeof(app_dct_t));
	wiced_dct_read_unlock(dct, WICED_TRUE);
	return ERR_CMD_OK;
}	

int cmd_set_device_token(int argc, char* argv[])
{
	app_dct_t* dct;
	if (argc < 2)
		return ERR_INSUFFICENT_ARGS;

	wiced_dct_read_lock((void**) &dct, WICED_TRUE, DCT_APP_SECTION,
			     0, sizeof(app_dct_t));
	memset(dct->device_token, 0, sizeof(dct->device_token));
	strcpy(dct->device_token, argv[1]);
	wiced_dct_write((const void*)dct, DCT_APP_SECTION,
			0, sizeof(app_dct_t));
	wiced_dct_read_unlock(dct, WICED_TRUE);
	return ERR_CMD_OK;
}	

int cmd_set_device_id(int argc, char* argv[])
{
	app_dct_t* dct;
	if (argc < 2)
		return ERR_INSUFFICENT_ARGS;

	wiced_dct_read_lock((void**) &dct, WICED_TRUE, DCT_APP_SECTION,
			     0, sizeof(app_dct_t));
	memset(dct->device_id, 0, sizeof(dct->device_token));
	strcpy(dct->device_id, argv[1]);
	wiced_dct_write((const void*)dct, DCT_APP_SECTION,
			0, sizeof(app_dct_t));
	wiced_dct_read_unlock(dct, WICED_TRUE);
	return ERR_CMD_OK;
}

int cmd_set_wlan(int argc, char* argv[])
{
	if (argc < 4)
		return ERR_INSUFFICENT_ARGS;
	
	a_set_wlan(argv[1], argv[2], argv[3]);
	return ERR_CMD_OK;
}

static inline unsigned int hextoint(char c)
{
	if (c >= '0' && c <= '9')
		return (unsigned int)c - '0';
	else
		return (unsigned int)c - 'a' + 10;
}

static void fill_mac(uint8_t* octet, char* str)
{
	octet[0] = (uint8_t)((hextoint(str[0]) << 4) + hextoint(str[1]));
	octet[1] = (uint8_t)((hextoint(str[3]) << 4) + hextoint(str[4]));
	octet[2] = (uint8_t)((hextoint(str[6]) << 4) + hextoint(str[7]));
	octet[3] = (uint8_t)((hextoint(str[9]) << 4) + hextoint(str[10]));
	octet[4] = (uint8_t)((hextoint(str[12]) << 4) + hextoint(str[13]));
	octet[5] = (uint8_t)((hextoint(str[15]) << 4) + hextoint(str[16]));
}

int cmd_set_mac(int argc, char* argv[])
{
	platform_dct_wifi_config_t* dct_wifi_config;
	if (argc < 2)
		return ERR_INSUFFICENT_ARGS;

	wiced_dct_read_lock((void**)&dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION,
			    0, sizeof(platform_dct_wifi_config_t));
	fill_mac(dct_wifi_config->mac_address.octet, argv[1]);
	wiced_dct_write((const void*)dct_wifi_config, DCT_WIFI_CONFIG_SECTION,
			0, sizeof(platform_dct_wifi_config_t));
	wiced_dct_read_unlock((void*)dct_wifi_config, WICED_TRUE);
	return ERR_CMD_OK;
}

int cmd_get_mac(int argc, char* argv[])
{
	platform_dct_wifi_config_t* dct_wifi_config;
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);
	wiced_dct_read_lock((void**)&dct_wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION,
			    0, sizeof(platform_dct_wifi_config_t));
	WPRINT_APP_INFO(("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       dct_wifi_config->mac_address.octet[0],
	       dct_wifi_config->mac_address.octet[1],
	       dct_wifi_config->mac_address.octet[2],
	       dct_wifi_config->mac_address.octet[3],
	       dct_wifi_config->mac_address.octet[4],
	       dct_wifi_config->mac_address.octet[5]
				));
	wiced_dct_read_unlock((void*)dct_wifi_config, WICED_FALSE);
	return ERR_CMD_OK;
}

int cmd_reboot(int argc, char* argv[])
{
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);
	wiced_framework_reboot();
	return ERR_CMD_OK;
}

int cmd_rssi(int argc, char* argv[])
{
	if (wiced_network_is_up(WICED_STA_INTERFACE)) {
                int32_t rssi;
                wwd_wifi_get_rssi(&rssi);
                printf("RSSI: %d\n", (int)rssi);
        } else {
                printf("RSSI: Disconnected\n");
        }
	return ERR_CMD_OK;
}

int cmd_pwm(int argc, char* argv[])
{
#if defined(TARGET_LED_GW)
	int freq = 3000;
	int index;
	int val;
	if (argc < 3)
		return ERR_INSUFFICENT_ARGS;

	index = atoi(argv[1]);
	if (index >= WICED_PWM_MAX)
		return ERR_TOO_LARGE_ARG;

	val = atoi(argv[2]);
	if (argc >= 4)
		freq = atoi(argv[3]);

	if (val < 0) val = 0;
	if (val >= 100) val = 100;
	wiced_pwm_init(WICED_PWM_1 + index, freq, val);
	wiced_pwm_start(WICED_PWM_1 + index );
	printf("Set PWM %d Level to %d, Freq = %d\n", index, val, freq);

	return ERR_CMD_OK;
#else
	printf("PWM not supported\n");
	return ERR_CMD_OK;
#endif
}

int cmd_adc(int argc, char* argv[])
{
	int i;
	uint16_t v;
	for (i = 0; i < WICED_ADC_MAX; i++) {
		wiced_adc_take_sample(WICED_PWM_1+i, &v);
		printf("ADC %d: %d\n", i, (int)v);
	}
	return ERR_CMD_OK;
}
