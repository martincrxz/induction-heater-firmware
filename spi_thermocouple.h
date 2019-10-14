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

void spi_thermocouple_init();
void spi_thermocouple_clear();
void spi_thermocouple_int_handler(); // interrupt handler
void spi_thermocouple_read_flt(); // read fault status register
void spi_thermocouple_read_cj(); // read cold junction measurement
void spi_thermocouple_read_tc(); // read temperature conversion measurement

#endif /* SPI_THERMOCOUPLE_H_ */
