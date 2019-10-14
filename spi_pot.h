/*
 * spi_pot.h
 *
 *  Created on: Oct 13, 2019
 *      Author: martin
 */

#ifndef SPI_POT_H_
#define SPI_POT_H_

#define SSI2_BIT_RATE 1000000
#define SSI2_DATA_SIZE 8
#define POWER_STANDBY 114

void spi_pot_init();
void set_power(uint8_t power);

#endif /* SPI_POT_H_ */
