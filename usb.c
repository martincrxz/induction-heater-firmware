/*
 * usb.c
 *
 *  Created on: Sep 28, 2019
 *      Author: martin
 */

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_uart.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/usb.h"
#include "driverlib/rom.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "utils/ustdlib.h"

#include "macros.h"
#include "usb_structs.h"
#include "usb.h"
#include "spi_pot.h"
#include "spi_thermocouple.h"

static uint8_t packet_count = 0;
static reading_status_t reading_status = WAITING;
static uint8_t read_data[PACKET_SIZE];

void usb_init() {
    // USB Pins
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_5 | GPIO_PIN_4);

    // USB Buffers
    USBBufferInit(get_tx_buffer());
    USBBufferInit(get_rx_buffer());

    // Set USB in device mode
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    // Init USB using CDC protocol
    USBDCDCInit(0, get_cdc_device());
}

uint32_t control_handler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue, void *pvMsgData) {

    switch(ui32Event)
    {

        case USB_EVENT_CONNECTED:

            USBBufferFlush(get_tx_buffer());
            USBBufferFlush(get_rx_buffer());

            break;

        case USB_EVENT_DISCONNECTED:
            break;

        case USBD_CDC_EVENT_GET_LINE_CODING:
            // Not sure if it needs configuration since we are not really using UART
            break;

        case USBD_CDC_EVENT_SET_LINE_CODING:
            // Not sure if it needs configuration since we are not really using UART
            break;

        case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
            break;

        case USBD_CDC_EVENT_SEND_BREAK:
            break;

        case USBD_CDC_EVENT_CLEAR_BREAK:
            break;

        case USB_EVENT_SUSPEND:
            break;

        case USB_EVENT_RESUME:
            break;

        default:
            break;

    }

    return(0);
}

uint32_t tx_handler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue, void *pvMsgData)
{

    switch(ui32Event)
    {
        case USB_EVENT_TX_COMPLETE:
            break;

        default:
            break;

    }
    return(0);
}

uint32_t rx_handler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue, void *pvMsgData) {

    tUSBBuffer* rxBuffer = get_rx_buffer();
    uint8_t byteRead;
    uint8_t i;

    switch(ui32Event)
    {
        case USB_EVENT_RX_AVAILABLE:
        {
            uint8_t bytes_available = USBBufferDataAvailable(rxBuffer);
            for(i = 0;i < bytes_available ; i++){
                USBBufferRead(rxBuffer,&byteRead,1);
                switch(reading_status){
                case WAITING:
                    if(byteRead == PACKETS_SEPARATOR){
                        read_data[packet_count] = byteRead;
                        packet_count++;
                        reading_status = READING;
                    }
                    break;
                case READING:
                    read_data[packet_count] = byteRead;
                    packet_count++;
                    if(packet_count == PACKET_SIZE){
                        reading_status = WAITING;
                        packet_count = 0;
                        process_packet(read_data);
                    }
                    break;
                }
            }
            break;
        }

        case USB_EVENT_DATA_REMAINING:
            break;

        case USB_EVENT_REQUEST_BUFFER:
            break;

        default:
            break;
    }

    return(0);
}

void process_packet(uint8_t packet[PACKET_SIZE]) {
    switch(packet[1]){ // packet[1] is message type
#ifdef ENABLE_THERMOCOUPLE
        case THERMOCOUPLE_CONFIGURATION:
            set_thermocouple_type((max31856_thermocoupletype_t)packet[CR1_INDEX]);
            // TODO: Ask for configuration registers to MAX31856 and then send that to the computer
            // I'm sending the data received from the computer back because I've not the MAX31856 to test
            send_packet(THERMOCOUPLE_CONFIGURATION_ACKNOWLEDGE, 0x00, packet[CR1_INDEX], 0x00, 0x00);
            break;
#endif
#ifdef ENABLE_POWER_CONTROL
        case SET_POWER:
            set_power(packet[2]);
            send_packet(POWER_SET_ACKNOWLEDGE, packet[2], 0x00, 0x00, 0x00);
            break;
#endif
        case SET_AUTOMATIC_CONTROL:
            // TODO: Set the automatic control ON (Through relay control)
            send_packet(AUTOMATIC_CONTROL_ACKNOWLEDGE, 0x00, 0x00, 0x00, 0x00);
            break;
        case SET_MANUAL_CONTROL:
            // TODO: Set the automatic control OFF (Through relay control)
            send_packet(MANUAL_CONTROL_ACKNOWLEDGE, 0x00, 0x00, 0x00, 0x00);
            break;
#ifdef ENABLE_POWER_CONTROL
        case SHUTDOWN_MESSAGE:
            // TODO: Send the stop instruction to the heater
            set_power(114);
            send_packet(SHUTDOWN_ACKNOWLEDGE, 0x00, 0x00, 0x00, 0x00);
            break;
#endif
        default:
            break;
    }
    return;
}

void send_packet(msg_type_t type, uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3){
    uint8_t packet[8] = {0x7E, type, data0, data1, data2, data3, 0x00};
    uint8_t crc = crc_checksum(packet, PACKET_SIZE-1);
    tUSBBuffer* txBuffer = get_tx_buffer();
    USBBufferWrite(txBuffer,packet,1);
    USBBufferWrite(txBuffer,packet+1,1);
    USBBufferWrite(txBuffer,packet+2,1);
    USBBufferWrite(txBuffer,packet+3,1);
    USBBufferWrite(txBuffer,packet+4,1);
    USBBufferWrite(txBuffer,packet+5,1);
    USBBufferWrite(txBuffer,packet+6,1);
    USBBufferWrite(txBuffer,&crc,1);
}

uint8_t crc_checksum(uint8_t *data, uint8_t size) {
    int16_t i;
    uint8_t chk = 0xFF;

    for (i=0; i < size; i++)
        chk -= data[i];

    return chk;
}

