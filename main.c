#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_uart.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_adc.h"
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
#include "driverlib/udma.h"

#include "macros.h"
#include "usb_structs.h"
#include "usb.h"
#include "spi_thermocouple.h"
#include "spi_pot.h"
#include "rms.h"
#include "freq.h"

extern buffer_status_t g_BufferStatus[2];
extern uint16_t g_ADC_Out1[ADC_SAMPLE_BUF_SIZE];
extern uint16_t g_ADC_Out2[ADC_SAMPLE_BUF_SIZE];
extern float g_voltageMidValue;

#ifdef ENABLE_TEST
void timer_0A_test_handler(void);
#endif

static void quadraticSum(uint16_t*);

int main(void)
{

#ifdef ENABLE_TEST
    uint32_t ui32Period;
#endif

    ROM_IntMasterDisable();

    // This enable floating point operations during interrupts, not sure if it's necessary
    ROM_FPULazyStackingEnable();

    // Set clock to 50 MHz, 16 MHz clock feeds PLL witch oscillates at 400 MHz
    // By default there's a division by 2 and we set another by 4, so it's 50 MHz
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

#ifdef ENABLE_TEST
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
    {
    }
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0))
    {
    }
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
    ui32Period = SysCtlClockGet() / 40000;
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);
#endif

#ifdef ENABLE_LED
    // Enable PORTF peripheral for led usage
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Enable green, blue and red LEDs (PORTF PIN3, PIN2 & PIN1)
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
#endif

    usb_init();

#ifdef ENABLE_THERMOCOUPLE
    spi_thermocouple_init();
#endif

#ifdef ENABLE_POWER_CONTROL
    spi_pot_init();
#endif

#ifdef ENABLE_CURRENT_MEASUREMENT
    adc_init();
    uDMA_init();
    adc_timer_init();
#endif

    timer_capture_init();
    send_freq_timer_init();

    ROM_IntMasterEnable();

#ifdef ENABLE_CURRENT_MEASUREMENT
    adc_timer_start();
#endif

    while(1)
    {
        if(g_BufferStatus[0] == FULL) {
            quadraticSum(g_ADC_Out1);
            g_BufferStatus[0] = EMPTY;
            ROM_uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                                       UDMA_MODE_PINGPONG,
                                       (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                                       &g_ADC_Out1,
                                       ADC_SAMPLE_BUF_SIZE);
            ROM_uDMAChannelEnable(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT);
        }

        if(g_BufferStatus[1] == FULL) {
            quadraticSum(g_ADC_Out2);
            g_BufferStatus[1] = EMPTY;
            ROM_uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                                       UDMA_MODE_PINGPONG,
                                       (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                                       &g_ADC_Out2,
                                       ADC_SAMPLE_BUF_SIZE);
            ROM_uDMAChannelEnable(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT);
        }
    }
}

void quadraticSum(uint16_t *samples) {
    if (samples == NULL)
        return;

    union {
        float flt;
        unsigned char bytes[4];
    } sum;

    uint32_t freqCount = get_freq_count();
    uint32_t samplesInPeriod = freqCount * 400E3 / 50E6;
    float voltageMidValue = 0;

    // Calculate average value
    int i;
    for(i = 0, sum.flt = 0; i < samplesInPeriod; i++) {
        float sample = ADC_VOLTAGE_STEP * samples[i];
        sum.flt += sample;
    }
    voltageMidValue = sum.flt / samplesInPeriod;

    // Calculate the RMS value, the square root is done in the PC not here
    for(i = 0, sum.flt = 0; i < samplesInPeriod; i++) {
        float sample = (ADC_VOLTAGE_STEP * samples[i]) - (voltageMidValue / 1.0);
        samples[i] = 0;
        sum.flt += sample * sample;
    }
    sum.flt /= samplesInPeriod;

    send_packet(CURRENT_RMS_READING, sum.bytes[3], sum.bytes[2], sum.bytes[1], sum.bytes[0]);
}

#ifdef ENABLE_TEST
void timer_0A_test_handler(void)
{
    // Clear the timer interrupt
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    int32_t readValue = ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3);
    if (((readValue & GPIO_PIN_3) & 0xff) == GPIO_PIN_3)
        ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);
    else
        ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
}
#endif
