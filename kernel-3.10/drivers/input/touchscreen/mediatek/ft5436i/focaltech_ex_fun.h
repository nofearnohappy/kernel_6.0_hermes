/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: focaltech_ex_fun.h
*
* Author:
*
* Created: 2015-01-01
*
* Abstract: function for fw upgrade, adb command, create apk second entrance
*
************************************************************************/
#ifndef __LINUX_fts_EX_FUN_H__
#define __LINUX_fts_EX_FUN_H__
 
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <mach/irqs.h>
 
#define FTS_DRV_VERSION	              "drv: MTK_2.7_20150409  \n"
#define FTS_UPGRADE_AA				0xAA
#define FTS_UPGRADE_55 			0x55
#define FTS_PACKET_LENGTH        		128
#define FTS_SETTING_BUF_LEN      	128
#define FTS_UPGRADE_LOOP			20
#define FTS_FACTORYMODE_VALUE		0x40
#define FTS_WORKMODE_VALUE		0x00
#define FTS_APP_INFO_ADDR			0xd7f8

#define BL_VERSION_LZ4        			0
#define BL_VERSION_Z7        			1
#define BL_VERSION_GZF        			2

//#define    AUTO_CLB
#define FTS_DBG
#ifdef FTS_DBG
	#define DBG(fmt, args...) printk("[FTS]" fmt, ## args)
#else
	#define DBG(fmt, args...) do{}while(0)
#endif

int fts_ctpm_auto_upgrade(struct i2c_client *client);
int fts_create_sysfs(struct i2c_client * client);
int fts_create_apk_debug_channel(struct i2c_client *client);
void fts_release_apk_debug_channel(void);
void fts_release_sysfs(struct i2c_client * client);
extern int fts_write_reg(struct i2c_client * client,u8 regaddr, u8 regvalue);
extern int fts_read_reg(struct i2c_client * client,u8 regaddr, u8 *regvalue);
#endif
