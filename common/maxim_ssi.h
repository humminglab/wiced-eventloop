/**********************************************************************
 * File: maxim_ssi.h
 * Programmer: Justin Jordan
 * Started: 17JUN14
 * Updated: DDMMMYY
 *
 * Description: Header file for maxim_ssi.c.  maxim_ssi.c is an 
 *              interface for the 78M6610+PSU, 78M6110+LMU and 
 *              MAX78630+PPM Embedded Measurement Devices.
 *
 **********************************************************************/


#ifndef __MAXIM_SSI_H
#define __MAXIM_SSI_H

/******************** Register Map ************************************/
#define EM_COMMAND	0x00
#define EM_CONTROL	0x02
#define EM_AVGPOWER	0x17

/******************** 78M6610_PSU ************************************/
#define MAX_PACKET_LEN 255 
#define MAX_REG_ADRS 0x7F  //max word address

/******************** Master SSI commands ****************************/
#define HEADER          0xAA
#define CLEAR_ADRS      0xA0
#define RW_ADRS_LO_BYTE 0xA1
#define RW_ADRS_HI_BYTE 0xA2
#define RW_ADRS         0xA3
#define DE_SELECT_TRGT  0xC0
#define SELECT_TRGT     0xCF
#define WRITE_BYTES     0xD0
#define READ_BYTES      0xE0

/******************** Slave SSI replies ******************************/
#define ACK_DATA    0xAA
#define AUTO_REPORT 0xAE
#define ACK_NO_DATA 0xAD
#define NACK        0xB0
#define BAD_CMD     0xBC
#define CHK_SUM_BAD 0xBD
#define BUFF_OVRFLW 0xBF


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
int ssi_select_device(unsigned char ssid);


/**********************************************************************
 * Function: ssi_deselect_device
 * Parameters: none
 * Returns: response from EMD, or -1 for error
 *
 * Description: Deselects slave EMD with the address 'ssid'

 **********************************************************************/
int ssi_deselect_device(void);


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
int ssi_set_rw_adrs(int adrs);


/**********************************************************************
 * Function: ssi_clear_adrs()
 * Parameters: none
 * Returns: response from EMD, or -1 for error
 *
 * Description: Sets the read/write pointer to address 0
 **********************************************************************/
int ssi_clear_adrs(void);


/**********************************************************************
 * Function: ssi_read_3bytes()
 * Parameters: adrs - Word address of register, use defined register 
 *                    names.
 * Returns: pointer to response from EMD
 *
 * Description: Sets the read/write pointer of the EMD to the desired 
 *              register and then reads the data from that register.  
 **********************************************************************/
unsigned char * ssi_read_3bytes(int adrs);


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
int ssi_write_3bytes(int adrs, unsigned char * data);


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
int ssi_send_packet(unsigned char byte_cnt, unsigned char *payload);


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
unsigned char ssi_get_checksum(unsigned char cnt, unsigned char *data);


/**********************************************************************
 * Function: 
 * Parameters: none
 * Returns: none
 *
 * Description: 

 **********************************************************************/


#endif /*__MAXIM_SSI_H*/


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

