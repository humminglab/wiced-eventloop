/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

#define MAX_SERVER_NAME		128
#define MAX_DEVICE_TOKEN	128

typedef struct
{
	char server[MAX_SERVER_NAME];
	char device_token[MAX_DEVICE_TOKEN];
} app_dct_t;
