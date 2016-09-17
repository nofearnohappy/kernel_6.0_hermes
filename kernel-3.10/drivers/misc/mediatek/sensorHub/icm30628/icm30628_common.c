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
#include "icm30628_common.h"
#include "icm30628_i2c.h"
#include "icm30628_spi.h"

static unsigned char preBankSel = -1;	
int invensense_read(struct icm30628_state_t * st, unsigned char reg, u32 length, unsigned char * data)
{
	int ret = 0;

	INV_DBG_FUNC_NAME_DETAIL;
	
	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(length == 0){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef I2C_INTERFACE
	if(UNLIKELY(!st->icm30628_i2c_client)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}	
	ret = invensense_i2c_read(st->icm30628_i2c_client, reg, length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
#endif
#ifdef SPI_INTERFACE	
	if(UNLIKELY(!st->icm30628_spi_device)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	ret = invensense_spi_read(st->icm30628_spi_device, reg, length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
#endif

	return ret;
}

int invensense_write(struct icm30628_state_t * st, unsigned char reg, u32 length, unsigned char * data)
{
	int ret = 0;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(length == 0){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef I2C_INTERFACE
	if(UNLIKELY(!st->icm30628_i2c_client)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}	
	ret = invensense_i2c_write(st->icm30628_i2c_client, reg, length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
#endif
#ifdef SPI_INTERFACE
	if(UNLIKELY(!st->icm30628_spi_device)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}	
	ret = invensense_spi_write(st->icm30628_spi_device, reg, length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
#endif

	return ret;
}

int invensense_bank_read(struct icm30628_state_t * st, unsigned char bank, unsigned char register_addr, u32 length, unsigned char *data)
{
	int ret = 0;
	unsigned char regBankSel[2] ={0};	
	u8 new_data[256] = {0};
	int i=0;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	regBankSel[0] = (register_addr & 0x80) ? ((bank & 1) | GARNET_SPI_ADDR_MSB_BIT): (bank & 1);
	if(preBankSel != regBankSel[0]){
		ret = invensense_write(st, GARNET_REG_BANK_SEL, 1, regBankSel);
		if (ret < 0) {
			INV_ERR;
			return ret;
		}
		preBankSel = regBankSel[0];
	}
	
	ret = invensense_read(st, (register_addr&~0x80), length, new_data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	for(i=0; i < length; i++){
		//data[i] = new_data[i+1];
		data[i] = new_data[i];
	}

	return ret;	
}

int invensense_bank_write(struct icm30628_state_t * st, unsigned char bank, unsigned char register_addr, u32 length, unsigned char *data)
{
	int ret = 0;
	unsigned char regBankSel[2] ={0};	

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	regBankSel[0] = (register_addr & 0x80) ? ((bank & 1) | GARNET_SPI_ADDR_MSB_BIT): (bank & 1);
	if(preBankSel != regBankSel[0]){
		ret = invensense_write(st, GARNET_REG_BANK_SEL, 1, regBankSel);
		if (ret < 0) {
			INV_ERR;
			return ret;
		}
		preBankSel = regBankSel[0];
	}
	
	ret |= invensense_write(st, (register_addr&~0x80), length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;	
}

int invensense_mem_read(struct icm30628_state_t * st, u8 mem_addr_reg, u8 mem_read_write_reg, u32 mem_addr, u32 length, u8 *data)
{
	int ret = 0;
	u8 memAddr[5];
	u8 dataRead[2] = {0};
	int i=0;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_LP_enable(false);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
	
	memAddr[0] = (mem_addr >> 24) & 0xff;
	memAddr[1] = (mem_addr >> 16) & 0xff;
	memAddr[2] = (mem_addr >> 8) & 0xff;
	memAddr[3] = (mem_addr) & 0xff;

	ret = invensense_write(st, mem_addr_reg, 4, memAddr);
	if (ret < 0) {
		INV_ERR;
		ret = icm30628_set_LP_enable(true);
		if (UNLIKELY(ret < 0)) {
			INV_ERR;
			return ret;
		}
		return ret;
	}

	for(i = 0; i<length; i++)
	{
		ret = invensense_read(st, mem_read_write_reg,1,dataRead);	
		//data[i] =  dataRead[1];
		data[i] =  dataRead[0];
	}

	ret = icm30628_set_LP_enable(true);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_mem_write(struct icm30628_state_t * st, u8 mem_addr_reg, u8 mem_read_write_reg, u32 mem_addr, u32 length, u8 *data)
{
	int ret = 0;
	u8 mem_addr_array[5];
	int i = 0;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_LP_enable(false);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	mem_addr_array[0] = (mem_addr >> 24) & 0xff;
	mem_addr_array[1] = (mem_addr >> 16) & 0xff;
	mem_addr_array[2] = (mem_addr >> 8) & 0xff;
	mem_addr_array[3] = (mem_addr) & 0xff;	

	ret = invensense_write(st, mem_addr_reg, 4, mem_addr_array);
	if (ret < 0) {
		INV_ERR;
		ret = icm30628_set_LP_enable(true);
		if (UNLIKELY(ret < 0)) {
			INV_ERR;
			return ret;
		}
		return ret;
	}

	for (i = 0 ; i < length; i++){
		ret = invensense_write(st, mem_read_write_reg, 1, &data[i]);		
		if (ret < 0) {
			INV_ERR;
			ret = icm30628_set_LP_enable(true);
			if (UNLIKELY(ret < 0)) {
				INV_ERR;
				return ret;
			}
			return ret;
		}
	}

	ret = icm30628_set_LP_enable(true);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_bank_mem_read(struct icm30628_state_t * st, unsigned int mem_addr, u32 length, unsigned char *data)
{
	int ret = 0;
	unsigned char reg_bank_sel[2] = {0};

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(preBankSel != reg_bank_sel[0]){
		ret = invensense_write(st, GARNET_REG_BANK_SEL, 1, reg_bank_sel);
		if (ret < 0) {
			INV_ERR;
			return ret;
		}
		preBankSel = reg_bank_sel[0];
	}
	ret = invensense_mem_read(st, GARNET_MEM_ADDR_SEL_0_B0, GARNET_MEM_R_W_B0, mem_addr, length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_bank_mem_write(struct icm30628_state_t * st, unsigned int mem_addr, u32 length, unsigned char *data)
{
	int ret = 0;
	unsigned char reg_bank_sel[2] = {0};

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(preBankSel != reg_bank_sel[0]){
		ret = invensense_write(st, GARNET_REG_BANK_SEL, 1, reg_bank_sel);
		if (ret < 0) {
			INV_ERR;
			return ret;
		}
		preBankSel = reg_bank_sel[0];
	}
	ret = invensense_mem_write(st, GARNET_MEM_ADDR_SEL_0_B0, GARNET_MEM_R_W_B0, mem_addr, length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_reg_read(struct icm30628_state_t * st, u16 reg, u32 length, u8 *data)
{
	int ret = 0;
	u8 bank, reg_addr;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	bank = (reg >> 8) & 0xFF;
	reg_addr = reg & 0xFF;
	ret = invensense_bank_read(st, bank, reg_addr, length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_reg_write(struct icm30628_state_t * st, u16 reg, u32 length, u8 *data)
{
	int ret = 0;
	u8 bank, reg_addr;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	bank = (reg >> 8) & 0xFF;
	reg_addr = reg & 0xFF;
	ret = invensense_bank_write(st, bank, reg_addr, length, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_mems_reg_read(struct icm30628_state_t * st, u8 reg, u32 length, u8 *data)
{
	int ret = 0;
	u8 spi_program[33] = {0};
	u8 new_data[17] = {0};
	u8 regData[2] = {0};
	unsigned char temp[4] = {0};
	int i = 0;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	spi_program[i++] = SPI_CLK_FREQ_PCLK_DIV_8 | SPI_SLAVE_0_ID;
	spi_program[i++] = SPI_MASTER_READ_COMMAND | (length-1);		
	spi_program[i++] = reg;
	spi_program[i++] = SPI_MASTER_READ_UPDATE_COMMAND;
	spi_program[i++] = SPI_MASTER_STOP_COMMAND;

	ret = invensense_bank_mem_write(st, GARNET_SPI_SCRIPT_START_ADDR,32,spi_program);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	temp[0] = (GARNET_SPI_SCRIPT_START_ADDR >> 24) & 0xF0;
	temp[0] |= (GARNET_SPI_SCRIPT_START_ADDR >> 16) & 0x0F;	
	temp[1] = (GARNET_SPI_SCRIPT_START_ADDR >> 8) & 0xFF;		
	temp[2] = (GARNET_SPI_SCRIPT_START_ADDR & 0xFF);
	ret  = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_SEC_INTF_PRGM_START_ADDR_0_B0, 3, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	regData[0] = SEC_INTF_CH0_RUN_BIT;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_MOD_RUN_ONCE_0_B0, 1, regData);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_EXT_SLV_SENS_DATA_00_B0, length, new_data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	
	for(i=0; i < length; i++){
		data[i] = new_data[i];
	}

	return ret;
}

int invensense_mems_reg_write(struct icm30628_state_t * st, u8 reg, u32 length, u8 *data)
{
	int ret = 0;
	u8 spi_program[32] = {0};
	u8 regData[2] = {0};
	unsigned char temp[4] = {0};
	int i = 0, j = 0;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	spi_program[i++] = SPI_CLK_FREQ_PCLK_DIV_8 | SPI_SLAVE_0_ID;
	spi_program[i++] = SPI_MASTER_WRITE_COMMAND | (length - 1);	
	spi_program[i++] = reg;	
	for(j = 0; j < length; j++){
		spi_program[j+i] = data[j];
	}
	spi_program[j+i] = SPI_MASTER_STOP_COMMAND;

	ret = invensense_bank_mem_write(st, GARNET_SPI_SCRIPT_START_ADDR,32,spi_program);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	temp[0] = (GARNET_SPI_SCRIPT_START_ADDR >> 24) & 0xF0;
	temp[0] |= (GARNET_SPI_SCRIPT_START_ADDR >> 16) & 0x0F;	
	temp[1] = (GARNET_SPI_SCRIPT_START_ADDR >> 8) & 0xFF;		
	temp[2] = (GARNET_SPI_SCRIPT_START_ADDR & 0xFF);
	ret  = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_SEC_INTF_PRGM_START_ADDR_0_B0, 3, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	regData[0] = SEC_INTF_CH0_RUN_BIT;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0,GARNET_MOD_RUN_ONCE_0_B0,1,regData);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_memory_write(struct icm30628_state_t * st, u32 mem_addr, const u8 *data_to_write, u32 total_size)
{
	int ret = 0;
	int block, write_size;
	u32 curr_size, addr;
	u8 *data;
	u8 rb[MAX_SPI_TRANSACTION_SIZE];
	u32 cnt = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_wake_up_m0();
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	data = (unsigned char *)data_to_write;		
	curr_size = total_size;
	addr = mem_addr;

	for(block = 0; curr_size > 0; block++, curr_size -= write_size){
		if (curr_size > MAX_SPI_TRANSACTION_SIZE)
			write_size = MAX_SPI_TRANSACTION_SIZE;
		else
			write_size = curr_size;

		_retry:

		ret = invensense_bank_mem_write(st, addr, write_size, data);
		if (ret < 0) {
			INV_ERR;
			return ret;
		}		

		ret = invensense_bank_mem_read(st, addr, write_size, rb);
		if (ret < 0) {
			INV_ERR;
			return ret;
		}

		if (memcmp(rb, data, write_size)) {
			u8 temp;
			ret = invensense_reg_read(st, GARNET_PWR_MGMT_1_B0, 1, &temp); 
			if (UNLIKELY(ret < 0)) {
				INV_ERR;
				return ret;
			}
			cnt++;
			if(cnt < 5){
				goto _retry;
			}else {
				INV_PRINT_ERR("invensense_memory_write(), sram error=%x\n", addr);
				return -1;
			}
		} else {
			cnt = 0;
		}

		data += write_size;
		addr += write_size;
	}

	return ret;
}


