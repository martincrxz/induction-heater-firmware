/*
 * adc.h
 *
 *  Created on: Feb 24, 2020
 *      Author: martin
 */

#ifndef RMS_H_
#define RMS_H_

#define ADC_SAMPLE_BUF_SIZE 512
#define ADC_VOLTAGE_STEP 0.000813187

typedef enum{
    EMPTY,
    FILLING,
    FULL
} buffer_status_t;

void adc_init();
void adc_timer_init();
void adc_timer_start();
void sample_seq_int_handler();
void uDMA_init();
void uDMA_error_handler();

#endif /* RMS_H_ */
