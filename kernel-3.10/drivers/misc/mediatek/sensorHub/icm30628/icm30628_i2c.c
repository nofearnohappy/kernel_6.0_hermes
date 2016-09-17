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

#include "icm30628_i2c.h"
#include "icm30628_debug.h"

struct i2c_client *g_i2c_client = NULL;

static int invensense_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!client)){
		INV_ERR;
		ret = -EINVAL;
	}

	g_i2c_client = client;

	return ret;
}

static int invensense_i2c_remove(struct i2c_client *client)
{
	INV_DBG_FUNC_NAME;

	return 0;
}

static int invensense_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	INV_DBG_FUNC_NAME;

	return 0;
}

static int invensense_i2c_resume(struct i2c_client *client)
{	
	INV_DBG_FUNC_NAME;

	return 0;
}

static const u16 normal_i2c[] = { I2C_CLIENT_END };
static const struct i2c_device_id invsens_i2c_ids[] = {
	{"ICM30628I2C", 128},
	{}
};

static struct i2c_driver invsens_i2c_driver = {
	.driver = {
			.owner	=	THIS_MODULE,
			.name 	= 	"inv_sensors_i2c",
		   },
	.id_table = 		invsens_i2c_ids,
	.probe = 		invensense_i2c_probe,
	.remove = 		invensense_i2c_remove,
	.address_list = 	normal_i2c,
	.suspend = 		invensense_i2c_suspend,
	.resume = 		invensense_i2c_resume,
};

struct i2c_client * invensense_i2c_initialize(void)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = i2c_add_driver(&invsens_i2c_driver);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return NULL;
	}

	if(g_i2c_client == NULL){
		ret = -1;
		INV_ERR;
		return NULL;

	}

	return g_i2c_client;
}

int invensense_i2c_terminate(void)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	i2c_del_driver(&invsens_i2c_driver);
	
	return ret;
}

int invensense_i2c_read(struct i2c_client *i2c_client, unsigned char reg, u32 length, unsigned char * data)
{
	int ret = 0;
	int i;
	struct i2c_msg msgs[2];
	
	INV_DBG_FUNC_NAME;

	if (UNLIKELY(!i2c_client|| !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	msgs[0].addr = ICM30628_I2C_ADDRESS;
	msgs[0].flags = 0;
	msgs[0].buf = &reg;
	msgs[0].len = 1;

	msgs[1].addr = ICM30628_I2C_ADDRESS;
	msgs[1].flags = I2C_M_RD;
	msgs[1].buf = data;
	msgs[1].len = length;

	i = 0;	
	do{
		ret = i2c_transfer(i2c_client->adapter, msgs, 2);
		if (UNLIKELY(ret < 2)) {
			if (UNLIKELY(ret >= 0)){
				ret = -EIO;
				INV_ERR;
			}
		}else{
			ret = 0;
		}
		i++;
		if(ret != 0){
			msleep(1);
		}
	}while (i < I2C_TRY && ret != 0);
	
	return ret;
}

int invensense_i2c_write(struct i2c_client *i2c_client, unsigned char reg, u32 length, unsigned char * data)
{
	int ret = 0;
	struct i2c_msg msgs;
	u8 buffer[24] = {0};

	INV_DBG_FUNC_NAME;

	if (UNLIKELY(!i2c_client|| !data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	buffer[0] = reg;
	memcpy(&buffer[1], data, length);
	
	msgs.addr = ICM30628_I2C_ADDRESS;
	msgs.flags = 0;
	msgs.buf = (u8 *) buffer;
	msgs.len = length + 1;

	ret = i2c_transfer(i2c_client->adapter, &msgs, 1);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	
	return ret;
}

