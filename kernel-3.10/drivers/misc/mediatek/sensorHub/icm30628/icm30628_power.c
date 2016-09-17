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

#include "icm30628_power.h"

struct regulator * g_vdd_ana = NULL;
struct regulator * g_vdd_1_8 = NULL;

int invensense_power_initialize(struct device * dev)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!dev)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	g_vdd_ana = regulator_get(dev, "inven,vdd_ana");
	if (UNLIKELY(IS_ERR(g_vdd_ana))){
		ret = PTR_ERR(g_vdd_ana);
		INV_ERR;
		return ret;
	}

	g_vdd_1_8 = regulator_get(dev, "inven,vcc_1_8");
	if (UNLIKELY(IS_ERR(g_vdd_1_8))) {
		ret = PTR_ERR(g_vdd_1_8);
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_power_onoff(bool onoff)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!g_vdd_ana || !g_vdd_1_8)){
		INV_ERR;
		return -EINVAL;
	}

	if(onoff){
		ret = regulator_enable(g_vdd_ana);
		if (UNLIKELY(ret < 0)){
			INV_ERR;
		}
		ret = regulator_enable(g_vdd_1_8);
		if (UNLIKELY(ret < 0)){
			INV_ERR;
		}
		msleep(200);
	}else{
		ret = regulator_disable(g_vdd_ana);
		if (UNLIKELY(ret < 0)){
			INV_ERR;
		}
		ret = regulator_disable(g_vdd_1_8);
		if (UNLIKELY(ret < 0)){
			INV_ERR;
		}
	}
	
	return ret;
}

