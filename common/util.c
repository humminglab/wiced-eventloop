/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_crypto.h"

#include "app_dct.h"
#include "util.h"

#define MAX_SSID_LEN 32
#define MAX_PASSPHRASE_LEN 64

const char* a_get_wlan_sec_name(wiced_security_t security)
{
	const char* s;
	switch(security) {
	case WICED_SECURITY_OPEN:		s = "None"; break;
	case WICED_SECURITY_WEP_PSK:		s = "WEP PSK"; break;
	case WICED_SECURITY_WEP_SHARED:		s = "WEP SHARED"; break;
	case WICED_SECURITY_WPA_TKIP_PSK:	s = "WPA TKIP PSK"; break;
	case WICED_SECURITY_WPA_AES_PSK:	s = "WPA AES PSK"; break;
	case WICED_SECURITY_WPA_MIXED_PSK:	s = "WPA MIXED PSK"; break;
	case WICED_SECURITY_WPA2_AES_PSK:	s = "WPA2 AES PSK"; break;
	case WICED_SECURITY_WPA2_TKIP_PSK:	s = "WPA2 TKIP PSK"; break;
	case WICED_SECURITY_WPA2_MIXED_PSK:	s = "WPA2 MIXED PSK"; break;
	case WICED_SECURITY_WPA_TKIP_ENT:
	case WICED_SECURITY_WPA_AES_ENT:
	case WICED_SECURITY_WPA_MIXED_ENT:
	case WICED_SECURITY_WPA2_TKIP_ENT:
	case WICED_SECURITY_WPA2_AES_ENT:
	case WICED_SECURITY_WPA2_MIXED_ENT:
	case WICED_SECURITY_IBSS_OPEN:
	case WICED_SECURITY_WPS_OPEN:
	case WICED_SECURITY_WPS_SECURE:
	case WICED_SECURITY_UNKNOWN:
	case WICED_SECURITY_FORCE_32_BIT:
	default:				s = "Unknown"; break;
	}
	return s;
}

wiced_result_t a_set_wlan(char *ssid, char *security, char *key)
{
	platform_dct_wifi_config_t* dct_wifi_config;
	wiced_security_t auth_type = WICED_SECURITY_UNKNOWN;

	if (strcmp(security, "None") == 0)
		auth_type = WICED_SECURITY_OPEN;
	else if (strcmp(security, "WEP-PSK") == 0 || strcmp(security, "WEP PSK") == 0 )
		auth_type = WICED_SECURITY_WEP_PSK;
	else if (strcmp(security, "WEP-SHARED") == 0 || strcmp(security, "WEP SHARED") == 0)
		auth_type = WICED_SECURITY_WEP_SHARED;
	else if (strcmp(security, "WPA-TKIP-PSK") == 0 || strcmp(security, "WPA TKIP PSK") == 0)
		auth_type = WICED_SECURITY_WPA_TKIP_PSK;
	else if (strcmp(security, "WPA-AES-PSK") == 0 || strcmp(security, "WPA AES PSK") == 0)
		auth_type = WICED_SECURITY_WPA_AES_PSK;
	else if (strcmp(security, "WPA-MIXED-PSK") == 0 || strcmp(security, "WPA MIXED PSK") == 0)
		auth_type = WICED_SECURITY_WPA_MIXED_PSK;
	else if (strcmp(security, "WPA2-AES-PSK") == 0 || strcmp(security, "WPA2 AES PSK") == 0)
		auth_type = WICED_SECURITY_WPA2_AES_PSK;
	else if (strcmp(security, "WPA2-TKIP-PSK") == 0 || strcmp(security, "WPA2 TKIP PSK") == 0)
		auth_type = WICED_SECURITY_WPA2_TKIP_PSK;
	else if (strcmp(security, "WPA2-MIXED-PSK") == 0 || strcmp(security, "WPA2 MIXED PSK") == 0)
		auth_type = WICED_SECURITY_WPA2_MIXED_PSK;

	if (auth_type == WICED_SECURITY_UNKNOWN)
		return WICED_ERROR;

	wiced_dct_read_lock((void**)&dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION,
			    0, sizeof(platform_dct_wifi_config_t));

	dct_wifi_config->stored_ap_list[0].details.SSID.length = (uint8_t)strlen(ssid);
	strncpy((char*)dct_wifi_config->stored_ap_list[0].details.SSID.value,
		ssid, MAX_SSID_LEN);
	dct_wifi_config->stored_ap_list[0].details.security = auth_type;
	if (auth_type == WICED_SECURITY_WEP_PSK || auth_type == WICED_SECURITY_WEP_SHARED) {
		uint8_t key_length = (uint8_t)strlen(key);
		format_wep_keys((char*)dct_wifi_config->stored_ap_list[0].security_key,
				key, &key_length, WEP_KEY_HEX_FORMAT);
		dct_wifi_config->stored_ap_list[0].security_key_length = key_length;
	} else {
		strncpy((char*)dct_wifi_config->stored_ap_list[0].security_key,
			key, MAX_PASSPHRASE_LEN);
		dct_wifi_config->stored_ap_list[0].security_key_length = (uint8_t)strlen(key);
	}
	wiced_dct_write((const void*)dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 
			0, sizeof(platform_dct_wifi_config_t));
	wiced_dct_read_unlock((void*)dct_wifi_config, WICED_TRUE);

	return WICED_SUCCESS;
}	

void a_stack_check(void)
{
	TX_THREAD *pt;
	const char *name;
	ULONG used;

	pt = tx_thread_identify();
	tx_thread_info_get(pt, &name, TX_NULL, TX_NULL, TX_NULL, TX_NULL, TX_NULL,
			   TX_NULL, TX_NULL);
	
	printf("Check Stack:  %s\n", name);
	printf("  Stack: %p~%p(0x%0x): %p\n",
	       pt->tx_thread_stack_start, pt->tx_thread_stack_end,
	       (unsigned int)pt->tx_thread_stack_size,
	       pt->tx_thread_stack_ptr);

	used = (ULONG)pt->tx_thread_stack_end - 1 - (ULONG)pt->tx_thread_stack_ptr;
	printf("  Used: %u/%u (%f%%)\n", (unsigned int)used,
	       (unsigned int)pt->tx_thread_stack_size,
	       (double)used / pt->tx_thread_stack_size * 100.);
}

void a_init_srand(void)
{
	unsigned int seed;
	wiced_crypto_get_random(&seed, sizeof(seed));
	srand(seed);
}

uint32_t a_random_time_window(uint32_t max_ms)
{
	int x = rand();

	if (max_ms == 0)
		return 0;

	x %= (max_ms - 1);
	return x + 1;
}
