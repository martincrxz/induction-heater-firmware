/*
 * usb.h
 *
 *  Created on: Sep 28, 2019
 *      Author: martin
 */

#ifndef USB_H_
#define USB_H_

#define PACKETS_SEPARATOR 0x7E
#define PACKET_SIZE 0x08

typedef enum {WAITING, READING} reading_status_t;
typedef enum {
    TEMPERATURE_READING = 0x10, // to computer
    COLD_JUNCTION_READING = 0x11, // to computer
    THERMOCOUPLE_FAULT = 0x12, // to computer
    THERMOCOUPLE_CONFIGURATION = 0x13, // to uC
    THERMOCOUPLE_CONFIGURATION_ACKNOWLEDGE = 0x14, // to computer
    SET_POWER = 0x20, // to uC
    POWER_SET_ACKNOWLEDGE = 0x21, // to computer
    SET_AUTOMATIC_CONTROL = 0x30, // to uC
    AUTOMATIC_CONTROL_ACKNOWLEDGE = 0x31, // to computer
    SET_MANUAL_CONTROL = 0x32, // to uC
    MANUAL_CONTROL_ACKNOWLEDGE = 0x33, // to computer
    SHUTDOWN_MESSAGE = 0x50, // to uC
    SHUTDOWN_ACKNOWLEDGE = 0x51 // to computer
} msg_type_t;

void usb_init();
void process_packet(uint8_t packet[PACKET_SIZE]);
void send_packet(msg_type_t type, uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3);
uint32_t rx_handler(void *pvCBData, uint32_t ui32Event,uint32_t ui32MsgValue, void *pvMsgData);
uint32_t tx_handler(void *pvi32CBData, uint32_t ui32Event,uint32_t ui32MsgValue, void *pvMsgData);
uint32_t control_handler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue, void *pvMsgData);
uint8_t crc_checksum(uint8_t *, uint8_t);

#endif /* USB_H_ */
