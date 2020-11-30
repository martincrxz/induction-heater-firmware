#include <rms.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_adc.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/udma.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/pin_map.h"

#include "usb.h"
#include "freq.h"

static uint32_t captureLastValue = 0;
static uint32_t count = 2500;
static bool shouldSendFreq = false;

void timer_capture_init(){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    GPIOPinConfigure(GPIO_PC6_WT1CCP0);

    GPIOPinTypeTimer(GPIO_PORTC_BASE, GPIO_PIN_6);

    TimerConfigure(WTIMER1_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME);

    TimerControlEvent(WTIMER1_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);

    IntEnable(INT_WTIMER1A);

    TimerIntEnable(WTIMER1_BASE, TIMER_CAPA_EVENT);

    TimerEnable(WTIMER1_BASE, TIMER_A);
}

void timer_capture_int_handler(){
    ROM_TimerIntClear(WTIMER1_BASE, TIMER_CAPA_EVENT);
    uint32_t actualValue = ROM_TimerValueGet(WTIMER1_BASE, TIMER_A);

    // TODO: See if it's necessary to clear the timer

    if(actualValue < captureLastValue)
        count = captureLastValue - actualValue;
    else
        count = captureLastValue - actualValue + 4294967295;

    captureLastValue = actualValue;
    shouldSendFreq = true;
}

void send_freq_timer_init() {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1))
    {
    }
    ROM_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, ROM_SysCtlClockGet() / 5);
    ROM_IntEnable(INT_TIMER1A);
    ROM_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(TIMER1_BASE, TIMER_A);
}

void send_freq_timer_int_handler() {
    ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    if (!shouldSendFreq)
        return;

    send_packet(CURRENT_FREQUENCY_READING, count>>24, count>>16, count>>8, count);
    shouldSendFreq = false;
}

uint32_t get_freq_count() {
    return count;
}
