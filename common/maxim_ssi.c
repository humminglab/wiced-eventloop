/**********************************************************************
 * File: maxim_ssi.c
 * Programmer: Justin Jordan
 * Started: 17JUN14
 * Updated: 14AUG14
 *
 * Description: SSI interface for 78M6610+PSU, 78M6110+LMU and 
 *              MAX78630+PPM Embedded Measurement Devices.
 *
 * The function defenitions for ssi_tx_byte and ssi_rx_byte need to be 
 * updated for your enviroment.
 *
 **********************************************************************/


#include "maxim_ssi.h"

#include "wiced.h"

/**********************************************************************
 * Function: ssi_select_device()
 * Parameters: ssid - address of device   
 *
 * ssid 24-pin pkg = ((DevAddr[5:0] << 2) | (ADDR1-pin << 1) | ADDR0-pin)
 *
 * ssid 16-pin pkg = ((DevAddr[6:0] << 1) | ADDR0-pin)
 *
 * Returns: response from EMD, or -1 for error
 *
 * Description: Selects slave EMD with the address 'ssid'
 *
 **********************************************************************/
int ssi_select_device(unsigned char ssid)
{
	unsigned char byte_cnt = 0;
	unsigned char payload[2];
	uint32_t size;
	wiced_result_t r;
  
	if(ssid > 0x0f)
	{
		payload[byte_cnt++] = SELECT_TRGT;
		payload[byte_cnt++] = ssid;
	}
	else
	{
		payload[byte_cnt++] = (ssid | 0xC0);
	}
  
	if(ssi_send_packet(byte_cnt, payload) != -1)
	{
		size = 1;
		r = wiced_uart_receive_bytes(SENSOR_UART, payload, &size, 3000);
		return((r == WICED_SUCCESS && payload[0] == ACK_NO_DATA) ? 0 : -1);
	}
	else
	{
		return(-1);
	}
}


/**********************************************************************
 * Function: ssi_deselect_device
 * Parameters: none
 * Returns: response from EMD, or -1 for error
 *
 * Description: Deselects slave EMD with the address 'ssid'

 **********************************************************************/
int ssi_deselect_device(void)
{
	unsigned char payload = DE_SELECT_TRGT;
	wiced_result_t r;
	uint32_t size;
  
	if(ssi_send_packet(1, &payload) != -1)
	{
		size = 1;
		r = wiced_uart_receive_bytes(SENSOR_UART, &payload, &size, 1000);
		return((r == WICED_SUCCESS && payload == ACK_NO_DATA) ? 0 : -1);
	}
	else
	{
		return(-1);
	}
}


/**********************************************************************
 * Function: ssi_set_rw_adrs()
 * Parameters: adrs - Word address of register, use defined register 
 *                    names.
 * Returns: response from EMD, or -1 for error
 *
 * Description: Sets the read write pointer of the EMD to the desired 
 *              register.  
 *
 **********************************************************************/
int ssi_set_rw_adrs(int local_adrs)
{
	unsigned char byte_cnt = 0;
	unsigned char payload[3];
	wiced_result_t r;
	uint32_t size;
  
	local_adrs *= 3;

	payload[byte_cnt++] = RW_ADRS;
	//LSB of address
	payload[byte_cnt++] = ((unsigned char) (0x00ff&local_adrs)); 
	//MSB of address
	payload[byte_cnt++] = ((unsigned char) (0x00ff&(local_adrs >> 8))); 
  
	if(ssi_send_packet(byte_cnt, payload) != -1)
	{
		size = 1;
		r = wiced_uart_receive_bytes(SENSOR_UART, payload, &size, 3000);
		return((r == WICED_SUCCESS && payload[0] == ACK_NO_DATA) ? 0 : -1);
	}
	else
	{
		return(-1);
	}
}


/**********************************************************************
 * Function: ssi_clear_adrs()
 * Parameters: none
 * Returns: response from EMD, or -1 for error
 *
 * Description: Sets the read/write pointer to address 0
 **********************************************************************/
int ssi_clear_adrs(void)
{
	unsigned char payload = CLEAR_ADRS;
	wiced_result_t r;
	uint32_t size;
  
	if(ssi_send_packet(1, &payload) != -1)
	{
		size = 1;
		r = wiced_uart_receive_bytes(SENSOR_UART, &payload, &size, 3000);
		return((r == WICED_SUCCESS && payload == ACK_NO_DATA) ? 0 : -1);
	}
	else
	{
		return(-1);
	}
}


/**********************************************************************
 * Function: ssi_read_3bytes()
 * Parameters: adrs - Word address of register, use defined register 
 *                    names.
 * Returns: pointer to response from EMD
 *
 * Description: Sets the read/write pointer of the EMD to the desired 
 *              register and then reads the data from that register.  
 **********************************************************************/
unsigned char * ssi_read_3bytes(int local_adrs)
{
	static unsigned char emd_response[6];
	unsigned char byte_cnt = 0;
	unsigned char payload[4];
	wiced_result_t r;
	uint32_t size;
  
	//flush response buffer from last time
	for(byte_cnt = 0; byte_cnt < 6; byte_cnt++)
	{
		emd_response[byte_cnt] = 0;
	}
  
	//reset byte_cnt
	byte_cnt = 0;
	local_adrs *= 3;
  
	payload[byte_cnt++] = RW_ADRS;
	//LSB of address
	payload[byte_cnt++] = ((unsigned char) (0x00ff&local_adrs)); 
	//MSB of address
	payload[byte_cnt++] = ((unsigned char) (0x00ff&(local_adrs >> 8)));
  
	payload[byte_cnt++] = (READ_BYTES | 0x03);
  
	ssi_send_packet(byte_cnt, payload);
 
	size = 6;
	r = wiced_uart_receive_bytes(SENSOR_UART, emd_response, &size, 3000);
	if (r == WICED_SUCCESS
	    && emd_response[0] == ACK_DATA)
	{
		return &emd_response[2];
	}

	return NULL;
}


/**********************************************************************
 * Function: ssi_write_3bytes
 * Parameters: adrs - Word address of register, use defined register 
 *                    names.
 *             data - pointer to data to write.  index 0 of data must 
 *                    be bits 7:0 of the register, index 1 bits 15:8
 *                    and index 2 bits 23:16
 * Returns: response from EMD, or -1 for error
 *
 * Description: Sets the read/write pointer of the EMD to the desired 
 *              register and then writes the data to that register.  
 **********************************************************************/
int ssi_write_3bytes(int local_adrs, unsigned char * data)
{
	unsigned char byte_cnt = 0;
	unsigned char payload[7];
	wiced_result_t r;
	uint32_t size;
  
	local_adrs *= 3;

	payload[byte_cnt++] = RW_ADRS;
	//LSB of address
	payload[byte_cnt++] = ((unsigned char) (0x00ff&local_adrs)); 
	//MSB of address
	payload[byte_cnt++] = ((unsigned char) (0x00ff&(local_adrs >> 8)));
  
	payload[byte_cnt++] = (WRITE_BYTES | 0x03);
	payload[byte_cnt++] = data[0]; //7:0
	payload[byte_cnt++] = data[1]; //15:8
	payload[byte_cnt++] = data[2]; //23:16
  
	if(ssi_send_packet(byte_cnt, payload) != -1)
	{
		size = 1;
		r = wiced_uart_receive_bytes(SENSOR_UART, payload, &size, 3000);
		return((r == WICED_SUCCESS && payload[0] == ACK_NO_DATA) ? 0 : -1);
	}
	else
	{
		return(-1);
	}
}

/**********************************************************************
 * Function: ssi_send_packet()
 * Parameters: byte_cnt - number of bytes in payload
 *             payload - pointer to payload data
 *
 * Returns: 1  - success
 *          -1 - failure, payload to big.
 *
 * Description: Builds complete packet from header, byte count, payload
 *              and checksum.  Then sends it.  Does not receive response
 *              or any data.
 *
 **********************************************************************/
int ssi_send_packet(unsigned char byte_cnt, unsigned char *payload)
{
	unsigned char idx;
	//+3 due to header, byte_cnt, and checksum
	unsigned char packet_length = (unsigned char)(byte_cnt + 3);
	unsigned char packet[MAX_PACKET_LEN];
  
	packet[0] = HEADER;
	packet[1] = packet_length; 
  
	//stuff payload into packet
	for(idx = 0; idx < byte_cnt; idx++)
	{
		packet[idx + 2] = payload[idx];
	}
  
	//get checksum
	packet[idx+2] = ssi_get_checksum(packet_length, packet);
  
	//send it down the pipe
	wiced_uart_transmit_bytes(SENSOR_UART, packet, packet_length);
  
	return 1;
}


/**********************************************************************
 * Function: ssi_get_checksum()
 * Parameters: cnt - number of bytes in packet counting the checksum
 *             data - pointer to an array containing data
 *
 * Returns: result of calculation
 *
 * Description: calculates the 2's comp of the modulo-256 sum of data
 *
 **********************************************************************/
unsigned char ssi_get_checksum(unsigned char cnt, unsigned char *data)
{
	int chksum = 0;
	unsigned char idx;
  
	for(idx = 0; idx < (cnt - 1); idx++)
	{
		chksum += data[idx];
	}
  
	chksum = (~(chksum%0x100) + 1);
  
	return((unsigned char) (0x00FF & chksum));
}


/**********************************************************************
 * Function: 
 * Parameters: none
 * Returns: none
 *
 * Description: 

 **********************************************************************/


/**********************************************************************
 * Copyright (C) 2013 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 **********************************************************************/


