/*
 * Copyright (C) 2012 Invensense, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef ICM30628_SPI_H
#define ICM30628_SPI_H

#include <linux/spi/spi.h>
#include "icm30628.h"
#include "icm30628_debug.h"

struct spi_device * invensense_spi_initialize(void);
int invensense_spi_terminate(void);
int invensense_spi_read(struct spi_device *spi_device, unsigned char reg, u32 length, unsigned char * data);
int invensense_spi_write(struct spi_device *spi_device, unsigned char reg, u32 length, unsigned char * data);

#endif /* ICM30628_SPI_H */

