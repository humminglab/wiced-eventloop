/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"

#include "app_dct.h"
#include "upgrade.h"
#include "network.h"

#include "http_stream.h"
#include "base64.h"
#include "wiced_security.h"
#include "wiced_tls.h"
#include "waf_platform.h"

#define MD5_LENGTH	16

#define NET_TIMEOUT	60000

#define MAX_FIRMWARE_IMAGE_SIZE 500 * 1024

static const char ref[] = "HTTP/1.";
static const char cl[] = "Content-Length:";

static int _read_line(wiced_tcp_stream_t *stream, char *buf, int max)
{
	int i;
	char c;

	for (i = 0; i < max - 1;) {
		if (wiced_tcp_stream_read(stream, &c, 1, NET_TIMEOUT) != WICED_SUCCESS)
			return -1;

		if (c == '\r') 
			/* do nothing */;
		else if (c == '\n')
			break;
		else
			buf[i++] = c;
	}

	while (c != '\n') {
		if (wiced_tcp_stream_read(stream, &c, 1, NET_TIMEOUT) != WICED_SUCCESS)
			return -1;
	};

	buf[i] = '\0';
	return i;
}

static void dump_bytes(const uint8_t* bptr, uint32_t len)
{
    uint32_t i = 0;
    UNUSED_PARAMETER(bptr);

    for (i = 0; i < len; )
    {
        if ((i & 0x0f) == 0)
        {
		printf("\n");
        }
        else if ((i & 0x07) == 0)
        {
		printf(" ");
        }
        printf("%02x ", bptr[i++]);
    }
    printf("\n");
}

static ota_result_t _get_write_fw(int index, int max, wiced_bool_t use_https, const char* host, uint16_t port,
				  const char* path, const uint8_t* md5)
{
	int i;
	int len;
	uint8_t calc_md5[MD5_LENGTH+1];
	uint8_t verify_md5[MD5_LENGTH+1];
	md5_context md5_ctx;

	uint32_t count  = 0;
	wiced_ip_address_t host_ip;
	wiced_app_t app;
	char *buf = NULL;

	wiced_tcp_socket_t socket;
	wiced_tls_context_t context;
	wiced_tcp_stream_t stream;

	ota_result_t ota_result = OTA_FAILURE;

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "OTA server : %s \n", host);

	while (wiced_hostname_lookup(host, &host_ip, NET_TIMEOUT,
				     WICED_STA_INTERFACE) != WICED_SUCCESS) {
		if (++count >= 2) {
			wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to find a host %s\n", host);
			ota_result = OTA_FAIL_TO_LOOKUP_HOSTNAME;
			goto return_error;
		}
	}
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Upgrade Server %s -> %08x\n", host, (unsigned int)host_ip.ip.v4);
	
	if (wiced_tcp_create_socket(&socket, WICED_STA_INTERFACE) != WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Create Socket Error\n");
		ota_result = OTA_FAIL_TO_CREATE_SOCKET;
		goto return_error;
	}

	if (use_https) {
		wiced_tls_init_context(&context, NULL, "*.humminglab.is");
		wiced_tcp_enable_tls(&socket, &context);
	}

	if (wiced_tcp_connect(&socket, &host_ip, port, NET_TIMEOUT) != WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to connect to server\n");
		ota_result = OTA_FAIL_TO_CONNECT_TO_SERVER;
		goto return_error_with_socket;
	}

	if (wiced_tcp_stream_init(&stream, &socket) != WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to create tcp stream\n");
		ota_result = OTA_FAIL_TO_CREATE_TCP_STREAM;
		goto return_error_with_socket;
	}

	/* send req */
	{
		static const char s[] = "GET ";
		wiced_tcp_stream_write(&stream, s, sizeof(s) - 1);
	}

	if (path) {
		wiced_tcp_stream_write(&stream, path, strlen(path));
	} else {
		ota_result = OTA_FAILURE;
		goto return_error_with_socket; 
	}

	{
		static const char s[] = " HTTP/1.1\r\nHost: ";
		wiced_tcp_stream_write(&stream, s, sizeof(s) - 1);
	}
	wiced_tcp_stream_write(&stream, host, strlen(host));
	{
		static const char s[] = "\r\nConnection: close\r\n\r\n";
		wiced_tcp_stream_write(&stream, s, sizeof(s) - 1);
	}
	if (wiced_tcp_stream_flush(&stream) != WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to send HTTP request\n");
		ota_result = OTA_FAIL_TO_SEND_HTTP_REQUEST;
		goto return_error_with_stream;
	}

	/* read */
#define _BSIZE (1024)
	buf = malloc(_BSIZE);
	i = _read_line(&stream, buf, _BSIZE);
	if (i < 0 || strncmp(buf, ref, sizeof(ref) - 1) != 0) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bad HTTP response\n");
		ota_result = OTA_FAIL_BY_BAD_HTTP_RESPONSE;
		goto return_error_with_stream;
	}

	i = atoi(buf + sizeof(ref) - 1 + 2);
	if (i != 200) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "No Upgrade: %d\n", i);
		if(i == 204) {
			ota_result = OTA_NO_UPGRADE_204;
		} else if (i == 500) {
			ota_result = OTA_NO_UPGRADE_500;
		} else if (i >= 400 && i < 499) {
			ota_result = OTA_NO_UPGRADE_4XX;
		} else {
			ota_result = OTA_NO_UPGRADE;
		}
		
		goto return_with_no_upgrade;
	}

	len = -1;
	while ((i = _read_line(&stream, buf, _BSIZE)) > 0) {
		if (strncasecmp(buf, cl, sizeof(cl) - 1) == 0) {
			len = atoi(buf + (sizeof(cl) - 1));
			wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Content-Length: %d\n", len);
		}
	}
	if (len <= 0 || len > max) {
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Content-Length is over max_size: %d\n", max);
		ota_result = OTA_FAIL_BY_CONTENT_LENGTH_OVER_MAXSIZE;
		goto return_error_with_stream;
	}

	/* skip until empty line */
	while (i > 0)
		i = _read_line(&stream, buf, _BSIZE);
	if (i < 0) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to find empty line\n");
		ota_result = OTA_FAIL_TO_FIND_EMPTY_LINE;
		goto return_error_with_stream;
	}

	/* read file & write flash */
	wiced_framework_app_open((uint8_t)index, &app);
	wiced_framework_app_get_size(&app, &count);
	if ((int)count < len) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Content-Length is over flash size: %d\n", (int)count);
		ota_result = OTA_FAIL_BY_BINARY_OVER_FLASHSIZE;
		goto return_error_with_sflash;
	}

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Erasing SFlash\n");
	wiced_framework_app_erase(&app);

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Start Download Body\n");
	md5_starts(&md5_ctx);

	for (i = 0; i < len / _BSIZE; i ++) {
		wiced_result_t r = wiced_tcp_stream_read(&stream, buf, _BSIZE, NET_TIMEOUT);
		if (r != WICED_SUCCESS) {
			wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to read http at %d (%d)\n", i * _BSIZE, r);
			ota_result = OTA_FAIL_TO_RECV_BINARY;
			goto return_error_with_sflash;
		}
		md5_update(&md5_ctx, (unsigned char*)buf, _BSIZE);
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Read %d\n", i * _BSIZE);
		wiced_framework_app_write_chunk(&app, (uint8_t*)buf, _BSIZE);
	}
	
	i = len % _BSIZE;
	if (i) {
		if (wiced_tcp_stream_read(&stream, buf,
					  (uint16_t)i, NET_TIMEOUT) != WICED_SUCCESS) {
			wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to read http at last block\n");
			goto return_error_with_sflash;
		}
		md5_update(&md5_ctx, (unsigned char*)buf, i);
		wiced_framework_app_write_chunk(&app, (uint8_t*)buf, (uint32_t)i);
	}		
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Download Completed %d Bytes\n", len);
	wiced_framework_app_close(&app);
	wiced_tcp_stream_deinit(&stream);
	wiced_tcp_delete_socket(&socket);
	if (use_https) {
		wiced_tls_deinit_context(&context);
	}

	md5_finish(&md5_ctx, calc_md5);
	if (md5) {
		if (memcmp(md5, calc_md5, MD5_LENGTH) != 0) {
			wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bad hash value: \n");
			dump_bytes(calc_md5, MD5_LENGTH);
			ota_result = OTA_FAIL_MD5_VALIDATION;
			goto return_error;
		}
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "MD5 hash is verified\n");
	}

	/* Write verified */
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Verifying SFlash writing... \n");
	md5_starts(&md5_ctx);
	wiced_framework_app_open((uint8_t)index, &app);
	for (i = 0; i < len / _BSIZE; i ++) {
		wiced_framework_app_read_chunk(&app, (uint32_t)i*_BSIZE, (uint8_t*)buf, _BSIZE);
		md5_update(&md5_ctx, (unsigned char*)buf, _BSIZE);
	}
	i = len % _BSIZE;
	wiced_framework_app_read_chunk(&app, (uint32_t)(len/_BSIZE)*_BSIZE, (uint8_t*)buf, (uint32_t)i);
	md5_update(&md5_ctx, (unsigned char*)buf, i);
	md5_finish(&md5_ctx, verify_md5);
	if (memcmp(verify_md5, calc_md5, MD5_LENGTH) != 0) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bad hash value: \n");
		dump_bytes(calc_md5, MD5_LENGTH);
		ota_result = OTA_FAIL_MD5_VALIDATION_WRITING;
		goto return_error;
	}
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "MD5 hash is verified\n");
	wiced_framework_app_close(&app);

	free(buf);
	return OTA_SUCCESS;
       
return_error_with_sflash:
	wiced_framework_app_close(&app);

return_with_no_upgrade:
return_error_with_stream:
	wiced_tcp_stream_deinit(&stream);

return_error_with_socket:
	wiced_tcp_delete_socket(&socket);
	if (use_https) {
		wiced_tls_deinit_context(&context);
	}
		
return_error:
	if (buf)
		free(buf);
	return ota_result;
}

static int _atoh(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return -1;
}

static wiced_result_t md5_hex_to_bin(const char* hex, uint8_t *bin)
{
	int i;
	i = strlen(hex);

	memset(bin, 0, MD5_LENGTH);
	if (i != MD5_LENGTH*2)
		return WICED_ERROR;

	for (i = 0; i < MD5_LENGTH; i++) {
		int d1, d2;
		d1 = _atoh(hex[i * 2]);
		d2 = _atoh(hex[i * 2 + 1]);
		if (d1 < 0 || d2 < 0)
			return WICED_ERROR;

		bin[i] = (uint8_t)((d1 << 4) + d2);
	}
	return WICED_SUCCESS;
}

ota_result_t a_upgrade_try(wiced_bool_t use_tls, const char *host, uint16_t port,
			   const char *path, const char *md5_hex, wiced_bool_t no_reboot)
{
	ota_result_t ota_result;
	uint8_t md5_bin[MD5_LENGTH];

	if (!a_network_is_up())
		return WICED_FALSE;

	if (md5_hex) {
		if (md5_hex_to_bin(md5_hex, md5_bin) < 0) {
			wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bad MD5 hexdigit format");
			return WICED_FALSE;
		}
	}
		
	ota_result = _get_write_fw(DCT_FR_APP_INDEX, MAX_FIRMWARE_IMAGE_SIZE, use_tls,
				   host, port, path, md5_hex ? md5_bin : NULL);
	if (ota_result == OTA_SUCCESS) {
		uint32_t ms = 3000;
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Upgrade completed");
		wiced_framework_set_boot(DCT_FR_APP_INDEX, PLATFORM_DEFAULT_LOAD);
		if (no_reboot)
			return OTA_SUCCESS;
		
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Rebooting in %d secs...\n", (int)ms / 1000);
		wiced_rtos_delay_milliseconds(ms);
		wiced_framework_reboot();
		while (1)
			wiced_rtos_delay_milliseconds(1000);
	}

	return ota_result;
}
