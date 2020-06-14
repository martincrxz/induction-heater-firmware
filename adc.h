/*
 * adc.h
 *
 *  Created on: Feb 24, 2020
 *      Author: martin
 */

#ifndef ADC_H_
#define ADC_H_

void adc_init();
void init_uDMA();
void sample_seq_int_handler();
void uDMAErrorHandler();
void udma_transfer_int_handler();

#endif /* ADC_H_ */
