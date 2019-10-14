/*
 * spi_pot.c
 *
 *  Created on: Oct 13, 2019
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

#include "spi_pot.h"

void spi_pot_init() {
    // Enable the peripherals used by SSI2
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    ROM_SysCtlDelay(3);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlDelay(3);

    // Enable the PINS used by SSI2
    // There's no need to configure RX since the potentiometer is not sending anything to the uC
    ROM_GPIOPinConfigure(GPIO_PB4_SSI2CLK);
    ROM_GPIOPinConfigure(GPIO_PB5_SSI2FSS);
    ROM_GPIOPinConfigure(GPIO_PB7_SSI2TX);
    ROM_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7 );

    // SSI Configuration
    ROM_SSIConfigSetExpClk(SSI2_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
            SSI_MODE_MASTER, SSI2_BIT_RATE, SSI2_DATA_SIZE);

    ROM_SSIEnable(SSI2_BASE);
    ROM_SysCtlDelay(3);

    ROM_SSIDataPut(SSI2_BASE, POWER_STANDBY);
    while(ROM_SSIBusy(SSI2_BASE))
    {
    }

    // A delay is maybe needed here

}

void set_power(uint8_t power) {

    ROM_SSIDataPut(SSI2_BASE, power);
    while(ROM_SSIBusy(SSI2_BASE))
    {
    }

}
