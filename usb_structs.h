/*
 * usb.h
 *
 *  Created on: Sep 28, 2019
 *      Author: martin
 */

#ifndef USB_STRUCTS_H_
#define USB_STRUCTS_H_

#define USB_BUFFER_SIZE 256

tUSBBuffer* get_tx_buffer();
tUSBBuffer* get_rx_buffer();
tUSBDCDCDevice* get_cdc_device();

#endif /* USB_STRUCTS_H_ */
