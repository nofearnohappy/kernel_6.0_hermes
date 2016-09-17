/*
 * Copyright (C) 2014 Invensense, Inc.
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

#ifndef ICM30628_COMMON_H
#define ICM30628_COMMON_H

#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include "icm30628.h"

struct icm30628_state_t;

#define I2C_TRY 10

int invensense_read(struct icm30628_state_t * st, unsigned char reg, u32 length, unsigned char * data);
int invensense_write(struct icm30628_state_t * st, unsigned char reg, u32 length, unsigned char * data);
int invensense_bank_read(struct icm30628_state_t * st, unsigned char bank, unsigned char register_addr, u32 length, unsigned char *data);
int invensense_bank_write(struct icm30628_state_t * st, unsigned char bank, unsigned char register_addr, u32 length, unsigned char *data);
int invensense_mem_read(struct icm30628_state_t * st, u8 mem_addr_reg, u8 mem_read_write_reg, u32 mem_addr, u32 length, u8 *data);
int invensense_mem_write(struct icm30628_state_t * st, u8 mem_addr_reg, u8 mem_read_write_reg, u32 mem_addr, u32 length, u8 *data);
int invensense_bank_mem_read(struct icm30628_state_t * st, unsigned int mem_addr, u32 length, unsigned char *data);
int invensense_bank_mem_write(struct icm30628_state_t * st, unsigned int mem_addr, u32 length, unsigned char *data);
int invensense_reg_read(struct icm30628_state_t * st, u16 reg, u32 length, u8 *data);
int invensense_reg_write(struct icm30628_state_t * st, u16 reg, u32 length, u8 *data);
int invensense_mems_reg_read(struct icm30628_state_t * st, u8 reg, u32 length, u8 *data);
int invensense_mems_reg_write(struct icm30628_state_t * st, u8 reg, u32 length, u8 *data);
int invensense_memory_write(struct icm30628_state_t * st, u32 mem_addr, const u8 *data_to_write, u32 total_size);

#endif //ICM30628_COMMON_H
