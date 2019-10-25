/*
 * spi_thermocouple.c
 *
 *  Created on: Oct 12, 2019
 *      Author: martin
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/ssi.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "Adafruit_MAX31856.h"

#include "spi_thermocouple.h"
#include "usb.h"

// We use 32 bits variables because the API's functions are set up for that size
static uint32_t ssi0_tx_data[3];
static uint32_t ssi0_rx_data[3];
static uint32_t ssi0_trash;

void spi_thermocouple_init() {

    // Enable peripherals
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlDelay(3);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    ROM_SysCtlDelay(3);

    // Configure pins
    ROM_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    ROM_GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    ROM_GPIOPinConfigure(GPIO_PA4_SSI0RX);
    ROM_GPIOPinConfigure(GPIO_PA5_SSI0TX);
    ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2);

    // Configure clock, mode, bit rate and data size
    ROM_SSIConfigSetExpClk(SSI0_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_1,
            SSI_MODE_MASTER, SSI0_BIT_RATE, SSI0_DATA_SIZE);

    // Enable SSI0
    ROM_SSIEnable(SSI0_BASE);
    ROM_SysCtlDelay(3);

    // Configure PA6 for fault interrupt (connected to Adafruit FLT)
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_6);
    ROM_GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_FALLING_EDGE);

    // Configure PA7 for conversion ready interrupt (connected to Adafruit DRDY)
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_7);
    ROM_GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE);

    // Clear the variables just in case
    spi_thermocouple_clear();

    // Register the handler and enable the interrupt
    GPIOIntRegister(GPIO_PORTA_BASE, spi_thermocouple_int_handler);
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_INT_PIN_6 | GPIO_INT_PIN_7);

}

void spi_thermocouple_clear() {

    // I'm not sure if this function is even necessary

    // Clear any remaining data
    while(ROM_SSIDataGetNonBlocking(SSI0_BASE,  &ssi0_trash))
    {
    }

    ssi0_tx_data[0] = 0;
    ssi0_tx_data[1] = 0;
    ssi0_tx_data[2] = 0;
    ssi0_rx_data[0] = 0;
    ssi0_rx_data[1] = 0;
    ssi0_rx_data[2] = 0;

}

void spi_thermocouple_int_handler() {

    uint32_t ui32Status = GPIOIntStatus(GPIO_PORTA_BASE, true);
    GPIOIntClear(GPIO_PORTA_BASE, ui32Status);

    if(ui32Status & GPIO_INT_PIN_6){
        // Handle fault
        void spi_thermocouple_read_flt();
        // Send fault status register via USB to the computer
        // See MAX31856 datasheet to parse in PC application
        send_packet(THERMOCOUPLE_FAULT, ssi0_rx_data[0], 0x00, 0x00, 0x00);
    }

    if(ui32Status & GPIO_INT_PIN_7){
        // Handle conversion ready
        void spi_thermocouple_read_cj();
        // Send cold junction reading via USB to the computer
        send_packet(COLD_JUNCTION_READING, ssi0_rx_data[0], ssi0_rx_data[1], 0x00, 0x00);
        void spi_thermocouple_read_tc();
        // Send temperature conversion reading via USB to the computer
        send_packet(TEMPERATURE_READING, ssi0_rx_data[0], ssi0_rx_data[1], ssi0_rx_data[2], 0x00);
    }

}

// Fault reading routine
void spi_thermocouple_read_flt() {

    // This will set up the register address in place to send to the MAX31856
    // Notice that the first 32-N bits are ignored by hardware (in this case N=16)
    ssi0_tx_data[0] = MAX31856_SR_REG << 8;

    // Ask for SR register (Fault status register)
    ROM_SSIDataPut(SSI0_BASE, ssi0_tx_data[0]);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }

    // A delay is maybe needed here

    // Receive SR register, only the first 8 bits will have valid data
    ROM_SSIDataGet(SSI0_BASE, &ssi0_rx_data[0]);

    ssi0_rx_data[0] &= 0xFF;
    ssi0_rx_data[1] &= 0x00;

}

// Cold junction reading routine
void spi_thermocouple_read_cj() {

    // This will set up the registers addresses in place to send to the MAX31856
    // Notice that the first 32-N bits are ignored by hardware (in this case N=16)
    ssi0_tx_data[0] = MAX31856_CJTH_REG << 8;
    ssi0_tx_data[1] = MAX31856_CJTL_REG << 8;

    // Ask for CJTH register
    ROM_SSIDataPut(SSI0_BASE, ssi0_tx_data[0]);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }

    // A delay is maybe needed here

    // Receive CJTH register, only the first 8 bits will have valid data
    ROM_SSIDataGet(SSI0_BASE, &ssi0_rx_data[0]);

    // Ask for CJTL register
    ROM_SSIDataPut(SSI0_BASE, ssi0_tx_data[1]);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }

    // A delay is maybe needed here

    // Receive CJTL register, only the first 8 bits will have valid data
    ROM_SSIDataGet(SSI0_BASE, &ssi0_rx_data[1]);

    // Clear everything but the first 8 bits, just in case
    ssi0_rx_data[0] &= 0xFF;
    ssi0_rx_data[1] &= 0xFF;

}

// Temperature conversion reading routine
void spi_thermocouple_read_tc() {

    // This will set up the registers addresses in place to send to the MAX31856
    // Notice that the first 32-N bits are ignored by hardware (in this case N=16)
    ssi0_tx_data[0] = MAX31856_LTCBH_REG << 8;
    ssi0_tx_data[1] = MAX31856_LTCBM_REG << 8;
    ssi0_tx_data[2] = MAX31856_LTCBL_REG << 8;

    // Ask for LTCBH register
    ROM_SSIDataPut(SSI0_BASE, ssi0_tx_data[0]);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }

    // A delay is maybe needed here

    // Receive LTCBH register, only the first 8 bits will have valid data
    ROM_SSIDataGet(SSI0_BASE, &ssi0_rx_data[0]);

    // Ask for LTCBM register
    ROM_SSIDataPut(SSI0_BASE, ssi0_tx_data[1]);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }

    // A delay is maybe needed here

    // Receive LTCBM register, only the first 8 bits will have valid data
    ROM_SSIDataGet(SSI0_BASE, &ssi0_rx_data[1]);

    // Ask for LTCBL register
    ROM_SSIDataPut(SSI0_BASE, ssi0_tx_data[2]);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }

    // A delay is maybe needed here

    // Receive LTCBL register, only the first 8 bits will have valid data
    ROM_SSIDataGet(SSI0_BASE, &ssi0_rx_data[2]);

    // Clear everything but the first 8 bits, just in case
    ssi0_rx_data[0] &= 0xFF;
    ssi0_rx_data[1] &= 0xFF;
    ssi0_rx_data[2] &= 0xFF;

}

void set_thermocouple_type(max31856_thermocoupletype_t type){

    static uint32_t cr1_aux = MAX31856_CR1_REG << 8;

    // Ask for CR1 register
    ROM_SSIDataPut(SSI0_BASE, cr1_aux);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }

    ROM_SSIDataGet(SSI0_BASE, &cr1_aux);

    // Clear type bits, set corresponding bits and set write bit
    cr1_aux = (cr1_aux & 0b0000) | type | (MAX31856_REG_WRITE << 8);

    ROM_SSIDataPut(SSI0_BASE, cr1_aux);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }

}
