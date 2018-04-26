/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

int cmd_get_dct(int argc, char* argv[]);
int cmd_set_server(int argc, char* argv[]);
int cmd_set_device_token(int argc, char* argv[]);
int cmd_set_device_id(int argc, char* argv[]);
int cmd_set_wlan(int argc, char* argv[]);
int cmd_set_mac(int argc, char* argv[]);
int cmd_get_mac(int argc, char* argv[]);
int cmd_reboot(int argc, char* argv[]);
int cmd_rssi(int argc, char* argv[]);
int cmd_pwm(int argc, char* argv[]);
int cmd_adc(int argc, char* argv[]);

#define EVENTLOOP_COMMANDS \
 { "get_dct", cmd_get_dct, 0, NULL, NULL, NULL, "Get DCT information" }, \
 { "set_server", cmd_set_server, 1, NULL, NULL, "server_address", "Set Server Address" }, \
 { "set_device_token", cmd_set_device_token, 1, NULL, NULL, "device_token", "Set Device Token" }, \
 { "set_device_id", cmd_set_device_id, 1, NULL, NULL, "device_id", "Set Device ID" }, \
 { "set_wlan", cmd_set_wlan, 3, NULL, NULL, "ssid security, key", "Set AP info" }, \
 { "set_mac", cmd_set_mac, 1, NULL, NULL, "mac", "Set MAC address" }, \
 { "get_mac", cmd_get_mac, 0, NULL, NULL, NULL, "Get MAC address" }, \
 { "reboot", cmd_reboot, 0, NULL, NULL, NULL, "Reboot" }, \
 { "rssi", cmd_rssi, 0, NULL, NULL, NULL, "Get RSSI" }, \
 { "pwm", cmd_pwm, 2, NULL, NULL, "index level (freq)", "Set PWM" }, \
 { "adc", cmd_adc, 0, NULL, NULL, NULL, "Read all ADC" },
