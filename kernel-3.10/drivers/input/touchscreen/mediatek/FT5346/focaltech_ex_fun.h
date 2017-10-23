#ifndef __FOCALTECH_EX_FUN_H__
#define __FOCALTECH_EX_FUN_H__

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>

#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>

#define FT_UPGRADE_AA            0xAA
#define FT_UPGRADE_55            0x55


/*****************************************************************************/
#define PAGE_SIZE                128
#define FTS_PACKET_LENGTH        128
#define FTS_SETTING_BUF_LEN      128
#define FTS_DMA_BUF_SIZE         4096

#define FTS_UPGRADE_LOOP         30

#define FTS_FACTORYMODE_VALUE    0x40
#define FTS_WORKMODE_VALUE       0x00

#define FTS_MIN_FW_LENGTH        (8)
#define FTS_ALL_FW_LENGTH        (64*1024)
#define FTS_APP_FW_LENGTH        (54*1024)

/*create sysfs for debug*/
int fts_create_sysfs(struct i2c_client * client);
void fts_release_sysfs(struct i2c_client * client);

int ft5x0x_create_apk_debug_channel(struct i2c_client *client);
void ft5x0x_release_apk_debug_channel(void);

int fts_ctpm_auto_upgrade(struct i2c_client *client);

int fts_dma_buffer_init(void);
int fts_dma_buffer_deinit(void);

/*
*fts_write_reg- write register
*@client: handle of i2c
*@regaddr: register address
*@regvalue: register value
*
*/
int fts_i2c_Read(struct i2c_client *client, u8 *writebuf,int writelen, u8 *readbuf, int readlen);

int fts_i2c_Write(struct i2c_client *client, u8 *writebuf, int writelen);

int fts_write_reg(struct i2c_client * client,u8 regaddr, u8 regvalue);

int fts_read_reg(struct i2c_client * client,u8 regaddr, u8 *regvalue);

void fts_ctpm_hw_reset(void);
#endif
