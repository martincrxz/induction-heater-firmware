/*
 * adc.c
 *
 *  Created on: Feb 24, 2020
 *      Author: martin
 */

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
#include "rms.h"

buffer_status_t g_BufferStatus[2];
uint16_t g_ADC_Out1[ADC_SAMPLE_BUF_SIZE];
uint16_t g_ADC_Out2[ADC_SAMPLE_BUF_SIZE];

#pragma DATA_ALIGN(ucControlTable, 1024)
static uint8_t ucControlTable[1024];

static uint32_t ui32DMAErrCount = 0u;

void adc_init(){

    int i;
    for(i = 0; i < ADC_SAMPLE_BUF_SIZE; i++) {
        g_ADC_Out1[i] = 0;
        g_ADC_Out2[i] = 0;
    }

    // ADC peripheral is enabled
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0))
    {
    }

    // Port E pin 3 for ADC ???
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE))
    {
    }
    ROM_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
    ROM_SysCtlDelay(80u);

    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_HALF, 1);
    ROM_SysCtlDelay(10);

    // All disabled just in case
    ROM_IntDisable(INT_ADC0SS0);
    ADCIntDisable(ADC0_BASE, 0u);
    ADCSequenceDisable(ADC0_BASE,0u);

    ROM_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0); // Sequencer 0 is configured
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0|ADC_CTL_IE|ADC_CTL_END); // Sequence steps are configured
    ROM_ADCSequenceEnable(ADC0_BASE, 0); // Sequencer 0 is enabled
    ROM_ADCIntClear(ADC0_BASE, 0); // Interrupt flag is cleared just in case
    ADCSequenceDMAEnable(ADC0_BASE, 0);
    ROM_IntEnable(INT_ADC0SS0); // ADC interrupt is enabled
}

void adc_timer_init(){
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER2))
    {
    }
    ROM_TimerConfigure(TIMER2_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
    ROM_TimerLoadSet(TIMER2_BASE, TIMER_A, ROM_SysCtlClockGet() / ADC_TIMER_FREQ - 1);
    ROM_TimerControlTrigger(TIMER2_BASE, TIMER_A, true);
    ROM_TimerControlStall(TIMER2_BASE, TIMER_A, true);
}

void adc_timer_start() {
    ROM_TimerEnable(TIMER2_BASE, TIMER_A);
}

void sample_seq_int_handler(){
    // Clear interrupt flag
    ROM_ADCIntClear(ADC0_BASE, 0);

    if ((uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT) == UDMA_MODE_STOP)
            && (g_BufferStatus[0] == FILLING))
    {
        g_BufferStatus[0] = FULL;
        g_BufferStatus[1] = FILLING;
    }
    else if ((uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT) == UDMA_MODE_STOP)
            && (g_BufferStatus[1] == FILLING))

    {
        g_BufferStatus[0] = FILLING;
        g_BufferStatus[1] = FULL;
    }
}

void uDMA_init(){
    g_BufferStatus[0] = FILLING;
    g_BufferStatus[1] = EMPTY;

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA))
    {
    }

    uDMAEnable(); // Enables uDMA
    uDMAControlBaseSet(ucControlTable);

    uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC0,
                                UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

    // Only allow burst transfers
    uDMAChannelAttributeEnable(UDMA_CHANNEL_ADC0, UDMA_ATTR_USEBURST);

    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                          UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_1);
    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                          UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_1);

    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                           UDMA_MODE_PINGPONG,
                           (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                           &g_ADC_Out1,
                           ADC_SAMPLE_BUF_SIZE);
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                           UDMA_MODE_PINGPONG,
                           (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                           &g_ADC_Out2,
                           ADC_SAMPLE_BUF_SIZE);

    // Enable uDMA channel 0
    uDMAChannelEnable(UDMA_CHANNEL_ADC0);
}

void uDMA_error_handler(){
    uint32_t ui32Status;
    ui32Status = uDMAErrorStatusGet();
    if(ui32Status)
    {
        uDMAErrorStatusClear();
        ui32DMAErrCount++;
    }
}
