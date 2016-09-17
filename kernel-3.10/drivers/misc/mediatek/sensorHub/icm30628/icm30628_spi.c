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

#include "icm30628_spi.h"

static struct spi_device *g_spi_device = NULL;
static spinlock_t g_spi_lock;

#ifdef MTK_PLATFORM
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <mach/mt_pm_ldo.h>
#include <linux/spi/spi.h>
#include <mach/mt_spi.h>
#include <mach/mt_sleep.h>

static struct mt_chip_conf spi_conf;

void mtk_spi_io_enable(int enable)
{
	if (enable){        
		mt_set_gpio_mode(GPIO_SPI_CS_PIN, GPIO_SPI_CS_PIN_M_SPI_CS);
		mt_set_gpio_mode(GPIO_SPI_SCK_PIN, GPIO_SPI_SCK_PIN_M_SPI_SCK);
		mt_set_gpio_mode(GPIO_SPI_MISO_PIN, GPIO_SPI_MISO_PIN_M_SPI_MISO);    
		mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_SPI_MOSI_PIN_M_SPI_MOSI);

		mt_set_gpio_mode(GPIO_SENSORHUB_HOST_RESET, GPIO_SENSORHUB_HOST_RESET_M_GPIO);
		mt_set_gpio_dir(GPIO_SENSORHUB_HOST_RESET, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_SENSORHUB_HOST_RESET, GPIO_OUT_ONE);

		mt_set_gpio_mode(GPIO_HEART_RATE_RESET_PIN, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_HEART_RATE_RESET_PIN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_HEART_RATE_RESET_PIN, GPIO_OUT_ONE);
	}else{
	        //set dir pull to save power
	        mt_set_gpio_mode(GPIO_SPI_CS_PIN, GPIO_SPI_CS_PIN_M_GPIO);
	        mt_set_gpio_dir(GPIO_SPI_CS_PIN, GPIO_DIR_IN);
	        mt_set_gpio_pull_enable(GPIO_SPI_CS_PIN, GPIO_PULL_DISABLE);
	            
	        mt_set_gpio_mode(GPIO_SPI_SCK_PIN, GPIO_SPI_SCK_PIN_M_GPIO);
	        mt_set_gpio_dir(GPIO_SPI_SCK_PIN, GPIO_DIR_IN);
	        mt_set_gpio_pull_enable(GPIO_SPI_SCK_PIN, GPIO_PULL_DISABLE);
	        
	        mt_set_gpio_mode(GPIO_SPI_MISO_PIN, GPIO_SPI_MISO_PIN_M_GPIO);
	        mt_set_gpio_dir(GPIO_SPI_MISO_PIN, GPIO_DIR_IN);
	        mt_set_gpio_pull_enable(GPIO_SPI_MISO_PIN, GPIO_PULL_DISABLE);
	        
	        mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_SPI_MOSI_PIN_M_GPIO);
	        mt_set_gpio_dir(GPIO_SPI_MOSI_PIN, GPIO_DIR_IN);
	        mt_set_gpio_pull_enable(GPIO_SPI_MOSI_PIN, GPIO_PULL_DISABLE);
	}
}
 
int mtk_spi_init(struct spi_device *spi)
{
	int ret = 0;

	mtk_spi_io_enable(1);

	spi->controller_data =(void*)&spi_conf;

	spi_conf.setuptime = 15;
	spi_conf.holdtime = 15;
	spi_conf.high_time = 6;  //Design 10--6m   15--4m   20--3m  30--2m  [ 60--1m 120--0.5m  300--0.2m]     
	spi_conf.low_time = 6;   //Actually in 2601, 6--5.53MHz, ICM30628 support 6MHz
	spi_conf.cs_idletime = 20;

	spi_conf.rx_mlsb = 1; 
	spi_conf.tx_mlsb = 1;		 
	spi_conf.tx_endian = 0;
	spi_conf.rx_endian = 0;

	spi_conf.cpol = SPI_CPOL_1;// 0;
	spi_conf.cpha = SPI_CPHA_1; //0
	spi_conf.com_mod = DMA_TRANSFER;

	spi_conf.pause = 0;
	spi_conf.finish_intr = 1;
	spi_conf.deassert = 0;

	ret = spi_setup(spi);
	if (ret < 0) {
		INV_ERR;
	}

	return ret;    
}
#endif

static int invensense_spi_probe(struct spi_device *spi)
{
	int ret = 0;
#ifdef MTK_PLATFORM
	extern struct icm30628_state_t * icm30628_state;
	u8 data[2] = {0};
#endif

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!spi)){
		INV_ERR;
		ret = -EINVAL;
	}

	g_spi_device =  spi;

	printk("SPI MODE = %x\n",spi->mode );
	printk("SPI IRQ = %x\n",spi->irq );
	printk("SPI SPEED = %x\n",spi->max_speed_hz );

#ifdef MTK_PLATFORM
	icm30628_state->icm30628_spi_device = g_spi_device;

	ret = mtk_spi_init(g_spi_device);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return NULL;
	}

	ret = invensense_bank_read(icm30628_state, GARNET_REG_BANK_0, GARNET_WHOAMI_B0, 1, data);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}		
	printk("WHOAMI = %x\n",data[0]);

	ret = invensense_post_initialize(icm30628_state);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}
#endif

	return ret;
}

static int invensense_spi_remove(struct spi_device *spi)
{
	INV_DBG_FUNC_NAME;

	return 0;
}

static int invensense_spi_suspend(struct spi_device *spi, pm_message_t mesg)
{
	INV_DBG_FUNC_NAME;

	return 0;
}

static int invensense_spi_resume(struct spi_device *spi)
{
	INV_DBG_FUNC_NAME;

	return 0;
}

#define INVSENS_SPI_NAME "ICM30628SPI"

#ifdef MTK_PLATFORM
static struct spi_board_info invsens_spi_boardinfo __initdata = {
    .modalias = INVSENS_SPI_NAME,
    .bus_num = 0,
    .chip_select=0,
    .mode = SPI_MODE_3,
};
#endif

const struct spi_device_id invsens_spi_ids[] = {
	{INVSENS_SPI_NAME, 0},
	{}
};

struct spi_driver invsens_spi_driver = {
	.driver = {
		.owner = THIS_MODULE,
#ifdef MTK_PLATFORM
		.name = INVSENS_SPI_NAME
#else
		.name = "inv_sensors_spi"
#endif
	},
#ifdef MTK_PLATFORM
#else
	.id_table =	invsens_spi_ids,
#endif
	.probe =		invensense_spi_probe,
	.remove =	invensense_spi_remove,
	.suspend =	invensense_spi_suspend,
	.resume =	invensense_spi_resume,
};

struct spi_device * invensense_spi_initialize(void)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

#ifdef MTK_PLATFORM
	ret = spi_register_board_info(&invsens_spi_boardinfo, 1);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return NULL;
	}
#endif

	ret = spi_register_driver(&invsens_spi_driver);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return NULL;
	}

#ifndef MTK_PLATFORM
	if(g_spi_device == NULL){
		ret = -1;
		INV_ERR;
		return NULL;
	}
#endif

	spin_lock_init(&g_spi_lock);

	return g_spi_device;
}

int invensense_spi_terminate(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

static void invensense_spi_complete(void *arg)
{
	complete(arg);
}

static int invensense_spi_bulk_read(struct spi_device *spi_device, u32 length, unsigned char * regs, unsigned char * data)
{
	int ret = 0;	
	DECLARE_COMPLETION_ONSTACK(done);
	struct spi_transfer	t = {
			.tx_buf		= regs,
			.rx_buf		= data,
			.len			= length,
		};
	struct spi_message	msg;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!spi_device || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	spi_message_init(&msg);
	spi_message_add_tail(&t, &msg);

	msg.complete = invensense_spi_complete;
	msg.context = &done;

	spin_lock_irq(&g_spi_lock);
	ret = spi_async(spi_device, &msg);
	if(UNLIKELY(ret)){
		spin_unlock_irq(&g_spi_lock);
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	spin_unlock_irq(&g_spi_lock);

	wait_for_completion(&done);
	ret = msg.status;
	if (ret == 0){
		ret = msg.actual_length;
	}
 
	return ret;
}

static int invensense_spi_bulk_write(struct spi_device *spi_device, u32 length, unsigned char * data)
{
	int ret = 0;
	DECLARE_COMPLETION_ONSTACK(done);
	struct spi_transfer	t = {
			.tx_buf		= data,
			.len			= length,
		};
	struct spi_message	msg;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!spi_device || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	spi_message_init(&msg);
	spi_message_add_tail(&t, &msg);

	msg.complete = invensense_spi_complete;
	msg.context = &done;

	spin_lock_irq(&g_spi_lock);
	ret = spi_async(spi_device, &msg);
	if(UNLIKELY(ret)){
		spin_unlock_irq(&g_spi_lock);
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	spin_unlock_irq(&g_spi_lock);

	wait_for_completion(&done);
	ret = msg.status;
	if(ret){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int invensense_spi_read(struct spi_device *spi_device, unsigned char reg, u32 length, unsigned char * data)
{
	int ret = 0;
	u8 temp[256] = {0};
	int i;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!spi_device || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	for(i=0; i < length; i++)
	{
		temp[i] = (0x80 | reg++);	
	}	
	temp[i] = 0xBE;

	ret = invensense_spi_bulk_read(spi_device, length + 1, temp, data);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	for(i=0; i < length; i++){
		data[i] = data[i + 1];
	}

	return ret;
}

int invensense_spi_write(struct spi_device *spi_device, unsigned char reg, u32 length, unsigned char * data)
{
	int ret = 0;
	u8 buffer[24] = {0};

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!spi_device || !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	buffer[0] = reg;
	memcpy(&buffer[1], data, length);

	ret = invensense_spi_bulk_write(spi_device, length +1, buffer);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

