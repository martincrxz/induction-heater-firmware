/*
 * spi_thermocouple.h
 *
 *  Created on: Oct 12, 2019
 *      Author: martin
 */

#ifndef SPI_THERMOCOUPLE_H_
#define SPI_THERMOCOUPLE_H_

#define SSI0_BIT_RATE  1000000
#define SSI0_DATA_SIZE 16

#include "Adafruit_MAX31856.h"

#define DELAY_MS(DELAY) ROM_SysCtlDelay((DELAY) * (ROM_SysCtlClockGet() / 3 / 1000))
#define SPI_DATA_DELAY 5

void spi_thermocouple_init();
void write_register(uint32_t address, uint32_t value);
uint32_t read_register(uint32_t address);
void spi_thermocouple_int_handler(); // interrupt handler
void set_thermocouple_type(max31856_thermocoupletype_t);
void read_fault_timer_handler();

#endif /* SPI_THERMOCOUPLE_H_ */
