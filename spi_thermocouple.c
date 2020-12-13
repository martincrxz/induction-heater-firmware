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
    ROM_GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_LOW_LEVEL);
    ROM_SysCtlDelay(3);

    // Configure PA7 for conversion ready interrupt (connected to Adafruit DRDY)
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_7);
    ROM_GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE);
    ROM_SysCtlDelay(3);

    // Register the handler and enable the interrupt
    GPIOIntRegister(GPIO_PORTA_BASE, spi_thermocouple_int_handler);
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_INT_PIN_6 | GPIO_INT_PIN_7);

    //Set default configuration
    uint32_t faultMaskValue = 0xFC;
    uint32_t cr0Value = 0x91;
    uint32_t cr1Value = 0x03;

    write_register(MAX31856_MASK_REG, faultMaskValue);
    write_register(MAX31856_CR0_REG, cr0Value);
    write_register(MAX31856_CR1_REG, cr1Value);
    cr0Value = read_register(MAX31856_CR0_REG);
    cr1Value = read_register(MAX31856_CR1_REG);

    send_packet(THERMOCOUPLE_CONFIGURATION_ACKNOWLEDGE, cr0Value, cr1Value, 0x00, 0x00);
}

void spi_thermocouple_int_handler() {

    uint32_t ui32Status = GPIOIntStatus(GPIO_PORTA_BASE, true);
    GPIOIntClear(GPIO_PORTA_BASE, ui32Status);

    if(ui32Status & GPIO_INT_PIN_6){
        // Send fault status register via USB to the computer
        // See MAX31856 datasheet to parse in PC application
        send_packet(THERMOCOUPLE_FAULT, read_register(MAX31856_SR_REG), 0x00, 0x00, 0x00);
    }

    if(ui32Status & GPIO_INT_PIN_7){
        // Send cold junction reading via USB to the computer
        send_packet(COLD_JUNCTION_READING,
                    read_register(MAX31856_CJTH_REG),
                    read_register(MAX31856_CJTL_REG),
                    0x00,
                    0x00);

        // Send temperature conversion reading via USB to the computer
        send_packet(TEMPERATURE_READING,
                    read_register(MAX31856_LTCBH_REG),
                    read_register(MAX31856_LTCBM_REG),
                    read_register(MAX31856_LTCBL_REG),
                    0x00);
    }

}

void set_thermocouple_type(max31856_thermocoupletype_t type){
    write_register(MAX31856_CR1_REG, type);
}

void write_register(uint32_t address, uint32_t value) {
    uint32_t tmp;
    ROM_SSIDataPut(SSI0_BASE, ((MAX31856_REG_WRITE | address) << 8) | value);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }
    while(ROM_SSIDataGetNonBlocking(SSI0_BASE,  &tmp))
    {
    }
}

uint32_t read_register(uint32_t address) {
    uint32_t value = 0;
    ROM_SSIDataPut(SSI0_BASE, address << 8);
    while(ROM_SSIBusy(SSI0_BASE))
    {
    }
    while(ROM_SSIDataGetNonBlocking(SSI0_BASE,  &value))
    {
    }
    return value & 0xFF;
}
