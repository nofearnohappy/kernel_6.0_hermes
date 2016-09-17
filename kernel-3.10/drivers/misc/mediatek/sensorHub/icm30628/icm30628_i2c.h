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

#ifndef ICM30628_I2C_H
#define ICM30628_I2C_H

#include <linux/i2c.h>
#include "icm30628.h"
#include "icm30628_debug.h"

struct i2c_client * invensense_i2c_initialize(void);
int invensense_i2c_terminate(void);
int invensense_i2c_read(struct i2c_client *i2c_client, unsigned char reg, u32 length, unsigned char * data);
int invensense_i2c_write(struct i2c_client *i2c_client, unsigned char reg, u32 length, unsigned char * data);

#endif /* ICM30628_I2C_H */
