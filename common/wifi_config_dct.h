/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

/*
 * AP settings in this file are stored in the DCT. These
 * settings may be overwritten at manufacture when the
 * DCT is written with the final production configuration
 */

/* This is the soft AP used for device configuration */
#define CONFIG_AP_SSID       "NOT USED FOR THIS APP"
#define CONFIG_AP_PASSPHRASE "NOT USED FOR THIS APP"
#define CONFIG_AP_SECURITY   WICED_SECURITY_OPEN
#define CONFIG_AP_CHANNEL    1
#define CONFIG_AP_VALID      WICED_FALSE

/* This is the soft AP available for normal operation */
#define SOFT_AP_SSID         "WICED Soft AP"
#define SOFT_AP_PASSPHRASE   "abcd1234"
#define SOFT_AP_SECURITY     WICED_SECURITY_WPA2_AES_PSK
#define SOFT_AP_CHANNEL      1

/* This is the default AP the device will connect to (as a client)*/
#define CLIENT_AP_SSID       "test-ssid"
#define CLIENT_AP_PASSPHRASE "test-1234"
#define CLIENT_AP_BSS_TYPE   WICED_BSS_TYPE_INFRASTRUCTURE
#define CLIENT_AP_SECURITY   WICED_SECURITY_WPA2_MIXED_PSK
#define CLIENT_AP_CHANNEL    1
#define CLIENT_AP_BAND       WICED_802_11_BAND_2_4GHZ

/* Override default country code */
#define WICED_COUNTRY_CODE   WICED_COUNTRY_CHINA

#ifdef __cplusplus
} /*extern "C" */
#endif
