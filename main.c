

/**
 * main.c
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
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/usb.h"
#include "driverlib/rom.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "utils/ustdlib.h"

#include "usb_structs.h"
#include "usb.h"
#include "spi_thermocouple.h"
#include "spi_pot.h"

//void Timer0IntHandler(void);

//static uint8_t count = 0x00;

int main(void)
{

    //uint32_t ui32Period;

    // This enable floating point operations during interrupts, not sure if it's necessary
    ROM_FPULazyStackingEnable();

    // Set clock to 50 MHz, 16 MHz clock feeds PLL witch oscillates at 400 MHz
    // By default there's a division by 2 and we set another by 4, so it's 50 MHz
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);


    /*
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ui32Period = SysCtlClockGet() / 5;
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);
    */

    // Enable PORTF peripheral for led usage
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Enable green, blue and red LEDs (PORTF PIN3, PIN2 & PIN1)
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);

    usb_init();
    spi_thermocouple_init();
    spi_pot_init();

    ROM_IntMasterEnable();

    while(1)
    {

    }
}

/*
void Timer0IntHandler(void)
{
    // Clear the timer interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    if(count == 0xff)
        count = 0x00;

    send_packet(TEMPERATURE_READING, 0x80, count, 0x00, 0x00);

    count++;
}
*/
