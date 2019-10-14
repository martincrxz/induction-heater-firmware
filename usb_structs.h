/*
 * usb.h
 *
 *  Created on: Sep 28, 2019
 *      Author: martin
 */

#ifndef USB_STRUCTS_H_
#define USB_STRUCTS_H_

#define USB_BUFFER_SIZE 256

extern tUSBBuffer g_sTxBuffer;
extern tUSBBuffer g_sRxBuffer;
extern tUSBDCDCDevice g_sCDCDevice;
extern uint8_t g_pui8USBTxBuffer[];
extern uint8_t g_pui8USBRxBuffer[];

#endif /* USB_STRUCTS_H_ */
