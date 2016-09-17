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

#ifndef ICM30628_DEBUG_H
#define ICM30628_DEBUG_H

//#define INVENSENSE_DEBUG_FUNCTION_NAME
//#define INVENSENSE_DATA_LOG
//#define INVENSENSE_DEBUG_INFO
#define INVENSENSE_DEBUG_ERROR

#define INV_TAG "[Invensense]:"

#ifdef INVENSENSE_DEBUG_FUNCTION_NAME
#define INV_DBG_FUNC_NAME printk(INV_TAG"%s\n", __FUNCTION__);
#else
#define INV_DBG_FUNC_NAME
#endif

#ifdef CONFIG_INVENSENSE_DEBUG_DETAIL
#define INV_DBG_FUNC_NAME_DETAIL printk(INV_TAG"%s\n", __FUNCTION__);
#else
#define INV_DBG_FUNC_NAME_DETAIL
#endif

#ifdef INVENSENSE_DEBUG_INFO
#define INV_INFO(format, args...) printk(INV_TAG format, ##args)
#define INV_INFO2(format, args...) printk(format, ##args)
#else
#define INV_INFO(format, args...)
#define INV_INFO2(format, args...)
#endif

#ifdef INVENSENSE_DATA_LOG
#define INV_DATA(format, args...) printk(INV_TAG format, ##args)
#define INV_DATA2(format, args...) printk(format, ##args)
#else
#define INV_DATA(format, args...)
#define INV_DATA2(format, args...)
#endif

#ifdef INVENSENSE_DEBUG_ERROR
#define INV_ERR printk(INV_TAG"error ret = %d, %s, %d\n", ret, __FUNCTION__, __LINE__);
#define INV_PRINT_ERR(format, args...)  printk(INV_TAG"error, " format, ##args)
#else
#define INV_ERR
#define INV_PRINT_ERR(format, args...)
#endif

#endif /* ICM30628_DEBUG_H */
