/*
 * adc.c
 *
 *  Created on: Feb 24, 2020
 *      Author: martin
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_adc.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/udma.h"

#include "adc.h"

#define ADC_SAMPLE_BUF_SIZE 512

enum BUFFERSTATUS{
    EMPTY,
    FILLING,
    FULL
};
static enum BUFFERSTATUS BufferStatus[2];

static uint16_t ADC_Out1[ADC_SAMPLE_BUF_SIZE];
static uint16_t ADC_Out2[ADC_SAMPLE_BUF_SIZE];

#pragma DATA_ALIGN(ucControlTable, 1024)
uint8_t ucControlTable[1024];

static uint32_t g_ui32DMAErrCount = 0u;

void adc_init(){

    // ADC peripheral is enabled
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Waiting for initialization
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0))
    {
    }

    // Interrupt handler is registered
    ADCIntRegister(ADC0_BASE, 0, sample_seq_int_handler);

    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_HALF, 1);

    SysCtlDelay(10);

    // Sequencer 0 is configured
    ROM_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0);

    // Sequence steps are configured
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH0|ADC_CTL_IE|ADC_CTL_END);

    // Interrupt flag is cleared just in case
    ROM_ADCIntClear(ADC0_BASE, 0);

    // ADC interrupt is enabled
    ROM_ADCIntEnable(ADC0_BASE, 0);

    // Sequencer 0 is enabled
    ROM_ADCSequenceEnable(ADC0_BASE, 0);
}

void sample_seq_int_handler(){

    // Clear interrupt flag
    ROM_ADCIntClear(ADC0_BASE, 0);

    if ((uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT) == UDMA_MODE_STOP)
            && (BufferStatus[0] == FILLING))
    {
        BufferStatus[0] = FULL;
        BufferStatus[1] = FILLING;
    }
    else if ((uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT) == UDMA_MODE_STOP)
            && (BufferStatus[1] == FILLING))

    {
        BufferStatus[0] = FILLING;
        BufferStatus[1] = FULL;
    }

    return;
}

void init_uDMA(){
    uDMAEnable(); // Enables uDMA
    uDMAControlBaseSet(ucControlTable);

    uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC0, UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

    uDMAChannelAttributeEnable(UDMA_CHANNEL_ADC0, UDMA_ATTR_USEBURST);
    // Only allow burst transfers

    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_1);
    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_1);

    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), &ADC_Out1, ADC_SAMPLE_BUF_SIZE);
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), &ADC_Out2, ADC_SAMPLE_BUF_SIZE);

    uDMAIntRegister(UDMA_CHANNEL_ADC0, udma_transfer_int_handler);

    uDMAChannelEnable(UDMA_CHANNEL_ADC0); // Enables DMA channel so it can perform transfers
}

void uDMAErrorHandler(){
    uint32_t ui32Status;
    ui32Status = uDMAErrorStatusGet();
    if(ui32Status)
    {
        uDMAErrorStatusClear();
        g_ui32DMAErrCount++;
    }
}

void udma_transfer_int_handler(){

    return;
}
