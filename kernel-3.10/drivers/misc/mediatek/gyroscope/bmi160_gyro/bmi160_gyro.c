/* BOSCH Gyroscope Sensor Driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * History: V1.0 --- [2013.01.29]Driver creation
 *          V1.1 --- [2013.03.28]
 *                   1.Instead late_resume, use resume to make sure
 *                     driver resume is ealier than processes resume.
 *                   2.Add power mode setting in read data.
 *          V1.2 --- [2013.06.28]Add self test function.
 *          V1.3 --- [2013.07.26]Fix the bug of wrong axis remapping
 *                   in rawdata inode.
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/module.h>

#if 0
#ifdef MT6516
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>
#endif

#ifdef MT6573
#include <mach/mt6573_devs.h>
#include <mach/mt6573_typedefs.h>
#include <mach/mt6573_gpio.h>
#include <mach/mt6573_pll.h>
#endif

#ifdef MT6575
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#endif

#ifdef MT6577
#include <mach/mt6577_devs.h>
#include <mach/mt6577_typedefs.h>
#include <mach/mt6577_gpio.h>
#include <mach/mt6577_pm_ldo.h>
#endif

#if defined(MT6573) || defined(MT6575) || defined(MT6577)
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#elif defined(MT6516)
#define POWER_NONE_MACRO MT6516_POWER_NONE
#endif
#endif

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <gyroscope.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE

#include <cust_gyro.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#include "bmi160_gyro.h"

/* sensor type */
enum SENSOR_TYPE_ENUM {
	BMI160_GYRO_TYPE = 0x0,

	INVALID_TYPE = 0xff
};

/* range */
enum BMG_RANGE_ENUM {
	BMG_RANGE_2000 = 0x0,	/* +/- 2000 degree/s */
	BMG_RANGE_1000,		/* +/- 1000 degree/s */
	BMG_RANGE_500,		/* +/- 500 degree/s */
	BMG_RANGE_250,		/* +/- 250 degree/s */
	BMG_RANGE_125,		/* +/- 125 degree/s */

	BMG_UNDEFINED_RANGE = 0xff
};

/* power mode */
enum BMG_POWERMODE_ENUM {
	BMG_SUSPEND_MODE = 0x0,
	BMG_NORMAL_MODE,

	BMG_UNDEFINED_POWERMODE = 0xff
};

/* debug infomation flags */
enum GYRO_TRC {
	GYRO_TRC_FILTER  = 0x01,
	GYRO_TRC_RAWDATA = 0x02,
	GYRO_TRC_IOCTL   = 0x04,
	GYRO_TRC_CALI	= 0x08,
	GYRO_TRC_INFO	= 0x10,
};

/* s/w data filter */
struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][BMG_AXES_NUM];
	int sum[BMG_AXES_NUM];
	int num;
	int idx;
};

/* bmg i2c client data */
struct bmg_i2c_data {
	struct i2c_client *client;
	struct gyro_hw *hw;
	struct hwmsen_convert   cvt;

	/* sensor info */
	u8 sensor_name[MAX_SENSOR_NAME];
	enum SENSOR_TYPE_ENUM sensor_type;
	enum BMG_POWERMODE_ENUM power_mode;
	enum BMG_RANGE_ENUM range;
	int datarate;
	/* sensitivity = 2^bitnum/range
	[+/-2000 = 4000; +/-1000 = 2000; +/-500 = 1000;
	+/-250 = 500; +/-125 = 250 ] */
	u16 sensitivity;

	/*misc*/
	struct mutex lock;
	atomic_t	trace;
	atomic_t	suspend;
	atomic_t	filter;
	s16	cali_sw[BMG_AXES_NUM+1];/* unmapped axis value */

	/* hw offset */
	s8	offset[BMG_AXES_NUM+1];/*+1: for 4-byte alignment*/

#if defined(CONFIG_BMG_LOWPASS)
	atomic_t	firlen;
	atomic_t	fir_en;
	struct data_filter	fir;
#endif

/*hzy add for early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif
};

#if 0
/* log macro */
#define GYRO_DEBUG	0
#define GYRO_TAG                  "[gyroscope] "
#if GYRO_DEBUG
#define GYRO_FUN(f)               printk(KERN_INFO GYRO_TAG"%s\n", __func__)
#define GYRO_LOG(fmt, args...)    printk(KERN_INFO GYRO_TAG fmt, ##args)
#else
#define GYRO_FUN(f)
#define GYRO_LOG(fmt, args...)
#endif
#define GYRO_ERR(fmt, args...) \
	printk(KERN_ERR GYRO_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#endif

#if 0
static struct platform_driver bmg_gyroscope_driver;
#endif
static struct gyro_init_info bmi160_gyro_init_info;
static int bmi160_gyro_init_flag =-1; // 0<==>OK -1 <==> fail
static struct i2c_driver bmg_i2c_driver;
static struct bmg_i2c_data *obj_i2c_data;
static int bmg_set_powermode(struct i2c_client *client,
		enum BMG_POWERMODE_ENUM power_mode);
static const struct i2c_device_id bmg_i2c_id[] = {
	{BMG_DEV_NAME, 0},
	{}
};

#define USE_DAEMON
extern struct mutex uplink_event_flag_mutex;
extern volatile u32 uplink_event_flag;
extern int m4g_data[];
extern int m4g_delay;
extern atomic_t g_flag;
extern wait_queue_head_t uplink_event_flag_wq;
extern struct mutex sensor_data_mutex;
static u32 BMMDRV_ULEVT_FLAG_G_ACTIVE = 0x0004;
static u32 BMMDRV_ULEVT_FLAG_G_DELAY = 0x0400;

static struct i2c_board_info __initdata bmg_i2c_info = {
	I2C_BOARD_INFO(BMG_DEV_NAME, BMI160_GYRO_I2C_ADDRESS)
};

/* I2C operation functions */
static int bmg_i2c_read_block(struct i2c_client *client, u8 addr,
				u8 *data, u8 len)
{
	u8 beg = addr;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,	.flags = 0,
			.len = 1,	.buf = &beg
		},
		{
			.addr = client->addr,	.flags = I2C_M_RD,
			.len = len,	.buf = data,
		}
	};
	int err;

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		GYRO_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (err != 2) {
		GYRO_ERR("i2c_transfer error: (%d %p %d) %d\n",
			addr, data, len, err);
		err = -EIO;
	} else {
		err = 0;/*no error*/
	}
	return err;
}

static int bmg_i2c_write_block(struct i2c_client *client, u8 addr,
				u8 *data, u8 len)
{
	/*
	*because address also occupies one byte,
	*the maximum length for write is 7 bytes
	*/
	int err, idx = 0, num = 0;
	char buf[C_I2C_FIFO_SIZE];

	if (!client)
		return -EINVAL;
	else if (len >= C_I2C_FIFO_SIZE) {
		GYRO_ERR("length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0) {
		GYRO_ERR("send command error!!\n");
		return -EFAULT;
	} else {
		err = 0;/*no error*/
	}
	return err;
}

#if 0
bool __attribute__((weak)) hwPowerOn(MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt, char *mode_name)
{
	return 1;
}

bool __attribute__((weak)) hwPowerDown(MT65XX_POWER powerId, char *mode_name)
{
	return 1;
}
#endif

static void bmg_power(struct gyro_hw *hw, unsigned int on)
{
	static unsigned int power_on;

	if (hw->power_id != POWER_NONE_MACRO) {/* have externel LDO */
		GYRO_LOG("power %s\n", on ? "on" : "off");
		if (power_on == on) {/* power status not change */
			GYRO_LOG("ignore power control: %d\n", on);
		} else if (on) {/* power on */
			if (!hwPowerOn(hw->power_id, hw->power_vol,
				BMG_DEV_NAME)) {
				GYRO_ERR("power on failed\n");
			}
		} else {/* power off */
			if (!hwPowerDown(hw->power_id, BMG_DEV_NAME))
				GYRO_ERR("power off failed\n");
		}
	}
	power_on = on;
}

static int bmg_read_raw_data(struct i2c_client *client, s16 data[BMG_AXES_NUM])
{
	struct bmg_i2c_data *priv = obj_i2c_data;
	int err = 0;

	if (priv->power_mode == BMG_SUSPEND_MODE) {
		err = bmg_set_powermode(client,
			(enum BMG_POWERMODE_ENUM)BMG_NORMAL_MODE);
		if (err < 0) {
			GYRO_ERR("set power mode failed, err = %d\n", err);
			return err;
		}
	}

	memset(data, 0, sizeof(s16)*BMG_AXES_NUM);

	if (NULL == client) {
		err = -EINVAL;
		return err;
	}

	if (priv->sensor_type == BMI160_GYRO_TYPE) {
		u8 buf_tmp[BMG_DATA_LEN] = {0};
		err = bmg_i2c_read_block(client, BMI160_USER_DATA_8_GYR_X_LSB__REG,
				buf_tmp, 6);
		if (err) {
			GYRO_ERR("[%s]read raw data failed, err = %d\n",
			priv->sensor_name, err);
			return err;
		}
		/* Data X */
		data[BMG_AXIS_X] = (s16)
			((((s32)((s8)buf_tmp[1]))
			  << BMI160_SHIFT_8_POSITION) | (buf_tmp[0]));
		/* Data Y */
		data[BMG_AXIS_Y] = (s16)
			((((s32)((s8)buf_tmp[3]))
			  << BMI160_SHIFT_8_POSITION) | (buf_tmp[2]));
		/* Data Z */
		data[BMG_AXIS_Z] = (s16)
			((((s32)((s8)buf_tmp[5]))
			  << BMI160_SHIFT_8_POSITION) | (buf_tmp[4]));

		if (atomic_read(&priv->trace) & GYRO_TRC_RAWDATA) {
			GYRO_LOG("[%s][16bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			data[BMG_AXIS_X],
			data[BMG_AXIS_Y],
			data[BMG_AXIS_Z],
			data[BMG_AXIS_X],
			data[BMG_AXIS_Y],
			data[BMG_AXIS_Z]);
		}
	}

#ifdef CONFIG_BMG_LOWPASS
/*
*Example: firlen = 16, filter buffer = [0] ... [15],
*when 17th data come, replace [0] with this new data.
*Then, average this filter buffer and report average value to upper layer.
*/
	if (atomic_read(&priv->filter)) {
		if (atomic_read(&priv->fir_en) &&
				 !atomic_read(&priv->suspend)) {
			int idx, firlen = atomic_read(&priv->firlen);
			if (priv->fir.num < firlen) {
				priv->fir.raw[priv->fir.num][BMG_AXIS_X] =
						data[BMG_AXIS_X];
				priv->fir.raw[priv->fir.num][BMG_AXIS_Y] =
						data[BMG_AXIS_Y];
				priv->fir.raw[priv->fir.num][BMG_AXIS_Z] =
						data[BMG_AXIS_Z];
				priv->fir.sum[BMG_AXIS_X] += data[BMG_AXIS_X];
				priv->fir.sum[BMG_AXIS_Y] += data[BMG_AXIS_Y];
				priv->fir.sum[BMG_AXIS_Z] += data[BMG_AXIS_Z];
				if (atomic_read(&priv->trace)&GYRO_TRC_FILTER) {
					GYRO_LOG("add [%2d]"
					"[%5d %5d %5d] => [%5d %5d %5d]\n",
					priv->fir.num,
					priv->fir.raw
					[priv->fir.num][BMG_AXIS_X],
					priv->fir.raw
					[priv->fir.num][BMG_AXIS_Y],
					priv->fir.raw
					[priv->fir.num][BMG_AXIS_Z],
					priv->fir.sum[BMG_AXIS_X],
					priv->fir.sum[BMG_AXIS_Y],
					priv->fir.sum[BMG_AXIS_Z]);
				}
				priv->fir.num++;
				priv->fir.idx++;
			} else {
				idx = priv->fir.idx % firlen;
				priv->fir.sum[BMG_AXIS_X] -=
					priv->fir.raw[idx][BMG_AXIS_X];
				priv->fir.sum[BMG_AXIS_Y] -=
					priv->fir.raw[idx][BMG_AXIS_Y];
				priv->fir.sum[BMG_AXIS_Z] -=
					priv->fir.raw[idx][BMG_AXIS_Z];
				priv->fir.raw[idx][BMG_AXIS_X] =
					data[BMG_AXIS_X];
				priv->fir.raw[idx][BMG_AXIS_Y] =
					data[BMG_AXIS_Y];
				priv->fir.raw[idx][BMG_AXIS_Z] =
					data[BMG_AXIS_Z];
				priv->fir.sum[BMG_AXIS_X] +=
					data[BMG_AXIS_X];
				priv->fir.sum[BMG_AXIS_Y] +=
					data[BMG_AXIS_Y];
				priv->fir.sum[BMG_AXIS_Z] +=
					data[BMG_AXIS_Z];
				priv->fir.idx++;
				data[BMG_AXIS_X] =
					priv->fir.sum[BMG_AXIS_X]/firlen;
				data[BMG_AXIS_Y] =
					priv->fir.sum[BMG_AXIS_Y]/firlen;
				data[BMG_AXIS_Z] =
					priv->fir.sum[BMG_AXIS_Z]/firlen;
				if (atomic_read(&priv->trace)&GYRO_TRC_FILTER) {
					GYRO_LOG("add [%2d]"
					"[%5d %5d %5d] =>"
					"[%5d %5d %5d] : [%5d %5d %5d]\n", idx,
					priv->fir.raw[idx][BMG_AXIS_X],
					priv->fir.raw[idx][BMG_AXIS_Y],
					priv->fir.raw[idx][BMG_AXIS_Z],
					priv->fir.sum[BMG_AXIS_X],
					priv->fir.sum[BMG_AXIS_Y],
					priv->fir.sum[BMG_AXIS_Z],
					data[BMG_AXIS_X],
					data[BMG_AXIS_Y],
					data[BMG_AXIS_Z]);
				}
			}
		}
	}
#endif
	return err;
}

#ifndef SW_CALIBRATION
/* get hardware offset value from chip register */
static int bmg_get_hw_offset(struct i2c_client *client,
		s8 offset[BMG_AXES_NUM + 1])
{
	int err = 0;

	/* HW calibration is under construction */
	GYRO_LOG("hw offset x=%x, y=%x, z=%x\n",
	offset[BMG_AXIS_X], offset[BMG_AXIS_Y], offset[BMG_AXIS_Z]);

	return err;
}
#endif

#ifndef SW_CALIBRATION
/* set hardware offset value to chip register*/
static int bmg_set_hw_offset(struct i2c_client *client,
		s8 offset[BMG_AXES_NUM + 1])
{
	int err = 0;

	/* HW calibration is under construction */
	GYRO_LOG("hw offset x=%x, y=%x, z=%x\n",
	offset[BMG_AXIS_X], offset[BMG_AXIS_Y], offset[BMG_AXIS_Z]);

	return err;
}
#endif

static int bmg_reset_calibration(struct i2c_client *client)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err = 0;

#ifdef SW_CALIBRATION

#else
	err = bmg_set_hw_offset(client, obj->offset);
	if (err) {
		GYRO_ERR("read hw offset failed, %d\n", err);
		return err;
	}
#endif

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;
}

static int bmg_read_calibration(struct i2c_client *client,
	int act[BMG_AXES_NUM], int raw[BMG_AXES_NUM])
{
	/*
	*raw: the raw calibration data, unmapped;
	*act: the actual calibration data, mapped
	*/
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err = 0;
	int mul;

	#ifdef SW_CALIBRATION
	/* only sw calibration, disable hw calibration */
	mul = 0;
	#else
	err = bmg_get_hw_offset(client, obj->offset);
	if (err) {
		GYRO_ERR("read hw offset failed, %d\n", err);
		return err;
	}
	mul = 1; /* mul = sensor sensitivity / offset sensitivity */
	#endif

	raw[BMG_AXIS_X] =
		obj->offset[BMG_AXIS_X]*mul + obj->cali_sw[BMG_AXIS_X];
	raw[BMG_AXIS_Y] =
		obj->offset[BMG_AXIS_Y]*mul + obj->cali_sw[BMG_AXIS_Y];
	raw[BMG_AXIS_Z] =
		obj->offset[BMG_AXIS_Z]*mul + obj->cali_sw[BMG_AXIS_Z];

	act[obj->cvt.map[BMG_AXIS_X]] =
		obj->cvt.sign[BMG_AXIS_X]*raw[BMG_AXIS_X];
	act[obj->cvt.map[BMG_AXIS_Y]] =
		obj->cvt.sign[BMG_AXIS_Y]*raw[BMG_AXIS_Y];
	act[obj->cvt.map[BMG_AXIS_Z]] =
		obj->cvt.sign[BMG_AXIS_Z]*raw[BMG_AXIS_Z];

	return err;
}

static int bmg_write_calibration(struct i2c_client *client,
	int dat[BMG_AXES_NUM])
{
	/* dat array : Android coordinate system, mapped, unit:LSB */
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err = 0;
	int cali[BMG_AXES_NUM], raw[BMG_AXES_NUM];

	/*offset will be updated in obj->offset */
	err = bmg_read_calibration(client, cali, raw);
	if (err) {
		GYRO_ERR("read offset fail, %d\n", err);
		return err;
	}

	GYRO_LOG("OLD OFFSET:"
	"unmapped raw offset(%+3d %+3d %+3d),"
	"unmapped hw offset(%+3d %+3d %+3d),"
	"unmapped cali_sw (%+3d %+3d %+3d)\n",
	raw[BMG_AXIS_X], raw[BMG_AXIS_Y], raw[BMG_AXIS_Z],
	obj->offset[BMG_AXIS_X],
	obj->offset[BMG_AXIS_Y],
	obj->offset[BMG_AXIS_Z],
	obj->cali_sw[BMG_AXIS_X],
	obj->cali_sw[BMG_AXIS_Y],
	obj->cali_sw[BMG_AXIS_Z]);

	/* calculate the real offset expected by caller */
	cali[BMG_AXIS_X] += dat[BMG_AXIS_X];
	cali[BMG_AXIS_Y] += dat[BMG_AXIS_Y];
	cali[BMG_AXIS_Z] += dat[BMG_AXIS_Z];

	GYRO_LOG("UPDATE: add mapped data(%+3d %+3d %+3d)\n",
		dat[BMG_AXIS_X], dat[BMG_AXIS_Y], dat[BMG_AXIS_Z]);

#ifdef SW_CALIBRATION
	/* obj->cali_sw array : chip coordinate system, unmapped,unit:LSB */
	obj->cali_sw[BMG_AXIS_X] =
		obj->cvt.sign[BMG_AXIS_X]*(cali[obj->cvt.map[BMG_AXIS_X]]);
	obj->cali_sw[BMG_AXIS_Y] =
		obj->cvt.sign[BMG_AXIS_Y]*(cali[obj->cvt.map[BMG_AXIS_Y]]);
	obj->cali_sw[BMG_AXIS_Z] =
		obj->cvt.sign[BMG_AXIS_Z]*(cali[obj->cvt.map[BMG_AXIS_Z]]);
#else
	int divisor = 1; /* divisor = sensor sensitivity / offset sensitivity */
	obj->offset[BMG_AXIS_X] = (s8)(obj->cvt.sign[BMG_AXIS_X]*
		(cali[obj->cvt.map[BMG_AXIS_X]])/(divisor));
	obj->offset[BMG_AXIS_Y] = (s8)(obj->cvt.sign[BMG_AXIS_Y]*
		(cali[obj->cvt.map[BMG_AXIS_Y]])/(divisor));
	obj->offset[BMG_AXIS_Z] = (s8)(obj->cvt.sign[BMG_AXIS_Z]*
		(cali[obj->cvt.map[BMG_AXIS_Z]])/(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[BMG_AXIS_X] = obj->cvt.sign[BMG_AXIS_X]*
		(cali[obj->cvt.map[BMG_AXIS_X]])%(divisor);
	obj->cali_sw[BMG_AXIS_Y] = obj->cvt.sign[BMG_AXIS_Y]*
		(cali[obj->cvt.map[BMG_AXIS_Y]])%(divisor);
	obj->cali_sw[BMG_AXIS_Z] = obj->cvt.sign[BMG_AXIS_Z]*
		(cali[obj->cvt.map[BMG_AXIS_Z]])%(divisor);

	GYRO_LOG("NEW OFFSET:"
	"unmapped raw offset(%+3d %+3d %+3d),"
	"unmapped hw offset(%+3d %+3d %+3d),"
	"unmapped cali_sw(%+3d %+3d %+3d)\n",
	obj->offset[BMG_AXIS_X]*divisor + obj->cali_sw[BMG_AXIS_X],
	obj->offset[BMG_AXIS_Y]*divisor + obj->cali_sw[BMG_AXIS_Y],
	obj->offset[BMG_AXIS_Z]*divisor + obj->cali_sw[BMG_AXIS_Z],
	obj->offset[BMG_AXIS_X],
	obj->offset[BMG_AXIS_Y],
	obj->offset[BMG_AXIS_Z],
	obj->cali_sw[BMG_AXIS_X],
	obj->cali_sw[BMG_AXIS_Y],
	obj->cali_sw[BMG_AXIS_Z]);

	/* HW calibration is under construction */
	err = bmg_set_hw_offset(client, obj->offset);
	if (err) {
		GYRO_ERR("read hw offset failed, %d\n", err);
		return err;
	}
#endif

	return err;
}

/* get chip type */
static int bmg_get_chip_type(struct i2c_client *client)
{
	int err = 0;
	u8 chip_id = 0;
	struct bmg_i2c_data *obj = obj_i2c_data;
	GYRO_FUN(f);

	/* twice */
	err = bmg_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, &chip_id, 0x01);
	err = bmg_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, &chip_id, 0x01);
	if (err != 0)
		return err;

	switch (chip_id) {
	case SENSOR_CHIP_ID_BMI:
	case SENSOR_CHIP_ID_BMI_C2:
	case SENSOR_CHIP_ID_BMI_C3:
		obj->sensor_type = BMI160_GYRO_TYPE;
		strcpy(obj->sensor_name, "bmi160_gyro");
		break;
	default:
		obj->sensor_type = INVALID_TYPE;
		strcpy(obj->sensor_name, "unknown sensor");
		break;
	}

	GYRO_LOG("[%s]chip id = %#x, sensor name = %s\n",
	__func__, chip_id, obj->sensor_name);

	if (obj->sensor_type == INVALID_TYPE) {
		GYRO_ERR("unknown gyroscope\n");
		return -1;
	}
	return 0;
}

/* set power mode */
static int bmg_set_powermode(struct i2c_client *client,
		enum BMG_POWERMODE_ENUM power_mode)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	u8 err = 0, data = 0, actual_power_mode = 0;

	GYRO_LOG("[%s] power_mode = %d, old power_mode = %d\n",
	__func__, power_mode, obj->power_mode);

	if (power_mode == obj->power_mode)
		return 0;

	mutex_lock(&obj->lock);

	if (obj->sensor_type == BMI160_GYRO_TYPE) {
		if (power_mode == BMG_SUSPEND_MODE) {
			actual_power_mode = CMD_PMU_GYRO_SUSPEND;
		} else if (power_mode == BMG_NORMAL_MODE) {
			actual_power_mode = CMD_PMU_GYRO_NORMAL;
		} else {
			err = -EINVAL;
			GYRO_ERR("invalid power mode = %d\n", power_mode);
			mutex_unlock(&obj->lock);
			return err;
		}
		data = actual_power_mode;
		err += bmg_i2c_write_block(client,
			BMI160_CMD_COMMANDS__REG, &data, 1);
		mdelay(55);
	}

	if (err < 0)
		GYRO_ERR("set power mode failed, err = %d, sensor name = %s\n",
			err, obj->sensor_name);
	else
		obj->power_mode = power_mode;

	mutex_unlock(&obj->lock);
	return err;
}

static int bmg_set_range(struct i2c_client *client, enum BMG_RANGE_ENUM range)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	u8 err = 0, data = 0, actual_range = 0;

	GYRO_LOG("[%s] range = %d, old range = %d\n",
	__func__, range, obj->range);

	if (range == obj->range)
		return 0;

	mutex_lock(&obj->lock);

	if (obj->sensor_type == BMI160_GYRO_TYPE) {/* BMI160_GYRO */
		if (range == BMG_RANGE_2000)
			actual_range = BMI160_RANGE_2000;
		else if (range == BMG_RANGE_1000)
			actual_range = BMI160_RANGE_1000;
		else if (range == BMG_RANGE_500)
			actual_range = BMI160_RANGE_500;
		else if (range == BMG_RANGE_500)
			actual_range = BMI160_RANGE_250;
		else if (range == BMG_RANGE_500)
			actual_range = BMI160_RANGE_125;
		else {
			err = -EINVAL;
			GYRO_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bmg_i2c_read_block(client,
			BMI160_USER_GYR_RANGE__REG, &data, 1);
		data = BMG_SET_BITSLICE(data,
			BMI160_USER_GYR_RANGE, actual_range);
		err += bmg_i2c_write_block(client,
			BMI160_USER_GYR_RANGE__REG, &data, 1);
		mdelay(1);

		if (err < 0)
			GYRO_ERR("set range failed,"
			"err = %d, sensor name = %s\n", err, obj->sensor_name);
		else {
			obj->range = range;
			/* bitnum: 16bit */
			switch (range) {
			case BMG_RANGE_2000:
				obj->sensitivity = 16;
			break;
			case BMG_RANGE_1000:
				obj->sensitivity = 33;
			break;
			case BMG_RANGE_500:
				obj->sensitivity = 66;
			break;
			case BMG_RANGE_250:
				obj->sensitivity = 131;
			break;
			case BMG_RANGE_125:
				obj->sensitivity = 262;
			break;
			default:
				obj->sensitivity = 16;
			break;
			}
		}
	}

	mutex_unlock(&obj->lock);
	return err;
}

static int bmg_set_datarate(struct i2c_client *client,
		int datarate)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	u8 err = 0, data = 0;

	GYRO_LOG("[%s] datarate = %d, old datarate = %d\n",
	__func__, datarate, obj->datarate);

	if (datarate == obj->datarate)
		return 0;

	mutex_lock(&obj->lock);

	if (obj->sensor_type == BMI160_GYRO_TYPE) {/* BMI160_GYRO */
		err = bmg_i2c_read_block(client,
			BMI160_USER_GYR_CONF_ODR__REG, &data, 1);
		data = BMG_SET_BITSLICE(data,
			BMI160_USER_GYR_CONF_ODR, datarate);
		err += bmg_i2c_write_block(client,
			BMI160_USER_GYR_CONF_ODR__REG, &data, 1);
	}

	if (err < 0)
		GYRO_ERR("set bandwidth failed,"
		"err = %d,sensor type = %d\n", err, obj->sensor_type);
	else
		obj->datarate = datarate;

	mutex_unlock(&obj->lock);
	return err;
}

static int bmg_selftest(struct i2c_client *client, u8 *result)
{
	int err = 0 ;
/* shall be finished later together with bmi160_acc */
#if 0
	struct bmg_i2c_data *obj = obj_i2c_data;
	u8 data1 = 0, data2 = 0;

	err = bmg_set_powermode(client,
		(enum BMG_POWERMODE_ENUM)BMG_NORMAL_MODE);

	mutex_lock(&obj->lock);

	if (obj->sensor_type == BMI160_GYRO_TYPE) {
		err = bmg_i2c_read_block(client,
				BMI160_GYRO_SELF_TEST_ADDR, &data1, 1);
		data2  = BMG_GET_BITSLICE(data1, BMI160_GYRO_SELF_TEST_ADDR_RATEOK);
		data1  = BMG_SET_BITSLICE(data1,
				BMI160_GYRO_SELF_TEST_ADDR_TRIGBIST, 1);
		err += bmg_i2c_write_block(client,
				BMI160_GYRO_SELF_TEST_ADDR_TRIGBIST__REG, &data1, 1);

		/* waiting time to complete the selftest process */
		mdelay(10);

		/* reading Selftest result bir bist_failure */
		err = bmg_i2c_read_block(client, \
		BMI160_GYRO_SELF_TEST_ADDR_BISTFAIL__REG, &data1, 1);
		data1  = BMG_GET_BITSLICE(data1,
				BMI160_GYRO_SELF_TEST_ADDR_BISTFAIL);
		if ((data1 == 0x00) && (data2 == 0x01))
			*result = 1;/* success */
		else
			*result = 0;/* failure */
	}

	GYRO_LOG("%s: %s\n", __func__,
			*result ? "success" : "failure");
	mutex_unlock(&obj->lock);
#endif
	return err;
}


/* bmg setting initialization */
static int bmg_init_client(struct i2c_client *client, int reset_cali)
{
#ifdef CONFIG_BMG_LOWPASS
	struct bmg_i2c_data *obj =
		(struct bmg_i2c_data *)obj_i2c_data;
#endif
	int err = 0;
	GYRO_FUN(f);

	err = bmg_get_chip_type(client);
	if (err < 0) {
		GYRO_ERR("get chip type failed, err = %d\n", err);
		return err;
	}

	err = bmg_set_datarate(client, BMI160_GYRO_ODR_100HZ);
	if (err < 0) {
		GYRO_ERR("set bandwidth failed, err = %d\n", err);
		return err;
	}

	err = bmg_set_range(client, (enum BMG_RANGE_ENUM)BMG_RANGE_2000);
	if (err < 0) {
		GYRO_ERR("set range failed, err = %d\n", err);
		return err;
	}

	err = bmg_set_powermode(client,
		(enum BMG_POWERMODE_ENUM)BMG_SUSPEND_MODE);
	if (err < 0) {
		GYRO_ERR("set power mode failed, err = %d\n", err);
		return err;
	}

	if (0 != reset_cali) {
		/*reset calibration only in power on*/
		err = bmg_reset_calibration(client);
		if (err < 0) {
			GYRO_ERR("reset calibration failed, err = %d\n", err);
			return err;
		}
	}

#ifdef CONFIG_BMG_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

	return 0;
}

/*
*Returns compensated and mapped value. unit is :degree/second
*/
static int bmg_read_sensor_data(struct i2c_client *client,
		char *buf, int bufsize)
{
	struct bmg_i2c_data *obj =
		(struct bmg_i2c_data *)obj_i2c_data;
	s16 databuf[BMG_AXES_NUM];
	int gyro[BMG_AXES_NUM];
	int err = 0;

	memset(databuf, 0, sizeof(s16)*BMG_AXES_NUM);
	memset(gyro, 0, sizeof(int)*BMG_AXES_NUM);

	if (NULL == buf)
		return -1;

	if (NULL == client) {
		*buf = 0;
		return -2;
	}

	err = bmg_read_raw_data(client, databuf);
	if (err) {
		GYRO_ERR("bmg read raw data failed, err = %d\n", err);
		return -3;
	} else {
		/* compensate data */
		databuf[BMG_AXIS_X] += obj->cali_sw[BMG_AXIS_X];
		databuf[BMG_AXIS_Y] += obj->cali_sw[BMG_AXIS_Y];
		databuf[BMG_AXIS_Z] += obj->cali_sw[BMG_AXIS_Z];

		/* remap coordinate */
		gyro[obj->cvt.map[BMG_AXIS_X]] =
			obj->cvt.sign[BMG_AXIS_X]*databuf[BMG_AXIS_X];
		gyro[obj->cvt.map[BMG_AXIS_Y]] =
			obj->cvt.sign[BMG_AXIS_Y]*databuf[BMG_AXIS_Y];
		gyro[obj->cvt.map[BMG_AXIS_Z]] =
			obj->cvt.sign[BMG_AXIS_Z]*databuf[BMG_AXIS_Z];

		/* convert: LSB -> degree/second(o/s) */
		gyro[BMG_AXIS_X] = gyro[BMG_AXIS_X] / obj->sensitivity;
		gyro[BMG_AXIS_Y] = gyro[BMG_AXIS_Y] / obj->sensitivity;
		gyro[BMG_AXIS_Z] = gyro[BMG_AXIS_Z] / obj->sensitivity;

		sprintf(buf, "%04x %04x %04x",
			gyro[BMG_AXIS_X], gyro[BMG_AXIS_Y], gyro[BMG_AXIS_Z]);
		if (atomic_read(&obj->trace) & GYRO_TRC_IOCTL)
			GYRO_LOG("gyroscope data: %s\n", buf);
	}

	return 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct bmg_i2c_data *obj = obj_i2c_data;

	if (NULL == obj) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", obj->sensor_name);
}

/*
* sensor data format is hex, unit:degree/second
*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	char strbuf[BMG_BUFSIZE] = "";

	if (NULL == obj) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	bmg_read_sensor_data(obj->client, strbuf, BMG_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*
* raw data format is s16, unit:LSB, axis mapped
*/
static ssize_t show_rawdata_value(struct device_driver *ddri, char *buf)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	s16 databuf[BMG_AXES_NUM];
	s16 dataraw[BMG_AXES_NUM];

	if (NULL == obj) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	bmg_read_raw_data(obj->client, dataraw);

	/*remap coordinate*/
	databuf[obj->cvt.map[BMG_AXIS_X]] =
			obj->cvt.sign[BMG_AXIS_X]*dataraw[BMG_AXIS_X];
	databuf[obj->cvt.map[BMG_AXIS_Y]] =
			obj->cvt.sign[BMG_AXIS_Y]*dataraw[BMG_AXIS_Y];
	databuf[obj->cvt.map[BMG_AXIS_Z]] =
			obj->cvt.sign[BMG_AXIS_Z]*dataraw[BMG_AXIS_Z];

	return snprintf(buf, PAGE_SIZE, "%hd %hd %hd\n",
			databuf[BMG_AXIS_X],
			databuf[BMG_AXIS_Y],
			databuf[BMG_AXIS_Z]);
}

static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err, len = 0, mul;
	int act[BMG_AXES_NUM], raw[BMG_AXES_NUM];

	if (NULL == obj) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	err = bmg_read_calibration(obj->client, act, raw);
	if (err)
		return -EINVAL;
	else {
		mul = 1; /* mul = sensor sensitivity / offset sensitivity */
		len += snprintf(buf+len, PAGE_SIZE-len,
		"[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
		mul,
		obj->offset[BMG_AXIS_X],
		obj->offset[BMG_AXIS_Y],
		obj->offset[BMG_AXIS_Z],
		obj->offset[BMG_AXIS_X],
		obj->offset[BMG_AXIS_Y],
		obj->offset[BMG_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len,
		"[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
		obj->cali_sw[BMG_AXIS_X],
		obj->cali_sw[BMG_AXIS_Y],
		obj->cali_sw[BMG_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len,
		"[ALL]unmapped(%+3d, %+3d, %+3d), mapped(%+3d, %+3d, %+3d)\n",
		raw[BMG_AXIS_X], raw[BMG_AXIS_Y], raw[BMG_AXIS_Z],
		act[BMG_AXIS_X], act[BMG_AXIS_Y], act[BMG_AXIS_Z]);

		return len;
	}
}

/*
*unit:mapped LSB
*Example:
*	if only force +1 LSB to android z-axis via s/w calibration,
		type command in terminal:
*		>echo 0x0 0x0 0x1 > cali
*	if only force -1(32 bit hex is 0xFFFFFFFF) LSB, type:
*               >echo 0x0 0x0 0xFFFFFFFF > cali
*/
static ssize_t store_cali_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err = 0;
	int dat[BMG_AXES_NUM];

	if (NULL == obj) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	if (!strncmp(buf, "rst", 3)) {
		err = bmg_reset_calibration(obj->client);
		if (err)
			GYRO_ERR("reset offset err = %d\n", err);
	} else if (BMG_AXES_NUM == sscanf(buf, "0x%02X 0x%02X 0x%02X",
		&dat[BMG_AXIS_X], &dat[BMG_AXIS_Y], &dat[BMG_AXIS_Z])) {
		err = bmg_write_calibration(obj->client, dat);
		if (err) {
			GYRO_ERR("bmg write calibration failed, err = %d\n",
				err);
		}
	} else {
		GYRO_ERR("invalid format\n");
	}

	return count;
}

static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMG_LOWPASS
	struct i2c_client *client = bmg222_i2c_client;
	struct bmg_i2c_data *obj = obj_i2c_data;
	if (atomic_read(&obj->firlen)) {
		int idx, len = atomic_read(&obj->firlen);
		GYRO_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++) {
			GYRO_LOG("[%5d %5d %5d]\n",
			obj->fir.raw[idx][BMG_AXIS_X],
			obj->fir.raw[idx][BMG_AXIS_Y],
			obj->fir.raw[idx][BMG_AXIS_Z]);
		}

		GYRO_LOG("sum = [%5d %5d %5d]\n",
			obj->fir.sum[BMG_AXIS_X],
			obj->fir.sum[BMG_AXIS_Y],
			obj->fir.sum[BMG_AXIS_Z]);
		GYRO_LOG("avg = [%5d %5d %5d]\n",
			obj->fir.sum[BMG_AXIS_X]/len,
			obj->fir.sum[BMG_AXIS_Y]/len,
			obj->fir.sum[BMG_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}

static ssize_t store_firlen_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
#ifdef CONFIG_BMG_LOWPASS
	struct i2c_client *client = bmg222_i2c_client;
	struct bmg_i2c_data *obj = obj_i2c_data;
	int firlen;

	if (1 != sscanf(buf, "%d", &firlen)) {
		GYRO_ERR("invallid format\n");
	} else if (firlen > C_MAX_FIR_LENGTH) {
		GYRO_ERR("exceeds maximum filter length\n");
	} else {
		atomic_set(&obj->firlen, firlen);
		if (NULL == firlen) {
			atomic_set(&obj->fir_en, 0);
		} else {
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif

	return count;
}

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct bmg_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	int trace;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	if (1 == sscanf(buf, "0x%x", &trace))
		atomic_set(&obj->trace, trace);
	else
		//GYRO_ERR("invalid content: '%s', length = %d\n", buf, count);

	return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmg_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	if (obj->hw)
		len += snprintf(buf+len, PAGE_SIZE-len,
		"CUST: %d %d (%d %d)\n",
		obj->hw->i2c_num, obj->hw->direction,
		obj->hw->power_id, obj->hw->power_vol);
	else
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");

	len += snprintf(buf+len, PAGE_SIZE-len, "i2c addr:%#x,ver:%s\n",
		obj->client->addr, BMG_DRIVER_VERSION);

	return len;
}

static ssize_t show_power_mode_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmg_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	len += snprintf(buf+len, PAGE_SIZE-len, "%s mode\n",
		obj->power_mode == BMG_NORMAL_MODE ? "normal" : "suspend");

	return len;
}

static ssize_t store_power_mode_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	unsigned long power_mode;
	int err;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	err = kstrtoul(buf, 10, &power_mode);

	if (err == 0) {
		err = bmg_set_powermode(obj->client,
			(enum BMG_POWERMODE_ENUM)(!!(power_mode)));
		if (err)
			return err;
		return count;
	}
	return err;
}

static ssize_t show_range_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmg_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	len += snprintf(buf+len, PAGE_SIZE-len, "%d\n", obj->range);

	return len;
}

static ssize_t store_range_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	unsigned long range;
	int err;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	err = kstrtoul(buf, 10, &range);

	if (err == 0) {
		if ((range == BMG_RANGE_2000)
		|| (range == BMG_RANGE_1000)
		|| (range == BMG_RANGE_500)
		|| (range == BMG_RANGE_250)
		|| (range == BMG_RANGE_125)) {
			err = bmg_set_range(obj->client, range);
			if (err)
				return err;
			return count;
		}
	}
	return err;
}

static ssize_t show_datarate_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmg_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	len += snprintf(buf+len, PAGE_SIZE-len, "%d\n", obj->datarate);

	return len;
}

static ssize_t store_datarate_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	unsigned long datarate;
	int err;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	err = kstrtoul(buf, 10, &datarate);

	if (err == 0) {
		err = bmg_set_datarate(obj->client, datarate);
		if (err)
			return err;
		return count;
	}

	return err;
}

/*
* return value:
* 0 --> failure
* 1 --> success
*/
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	u8 result = 0;
	struct bmg_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("bmg i2c data pointer is null\n");
		return 0;
	}

	bmg_selftest(obj->client, &result);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d\n", result);

	return len;
}

static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(rawdata, S_IWUSR | S_IRUGO, show_rawdata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, show_cali_value, store_cali_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO,
		show_firlen_value, store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO,
		show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(powermode, S_IWUSR | S_IRUGO,
		show_power_mode_value, store_power_mode_value);
static DRIVER_ATTR(range, S_IWUSR | S_IRUGO,
		show_range_value, store_range_value);
static DRIVER_ATTR(datarate, S_IWUSR | S_IRUGO,
		show_datarate_value, store_datarate_value);
static DRIVER_ATTR(selftest, S_IRUGO, show_selftest_value, NULL);

static struct driver_attribute *bmg_attr_list[] = {
	/* chip information */
	&driver_attr_chipinfo,
	/* dump sensor data */
	&driver_attr_sensordata,
	/* dump raw data */
	&driver_attr_rawdata,
	/* show calibration data */
	&driver_attr_cali,
	/* filter length: 0: disable, others: enable */
	&driver_attr_firlen,
	/* trace flag */
	&driver_attr_trace,
	/* get hw configuration */
	&driver_attr_status,
	/* get power mode */
	&driver_attr_powermode,
	/* get range */
	&driver_attr_range,
	/* get data rate */
	&driver_attr_datarate,
	/* self test */
	&driver_attr_selftest,
};

static int bmg_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bmg_attr_list)/sizeof(bmg_attr_list[0]));
	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, bmg_attr_list[idx]);
		if (err) {
			GYRO_ERR("driver_create_file (%s) = %d\n",
				bmg_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

static int bmg_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bmg_attr_list)/sizeof(bmg_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, bmg_attr_list[idx]);

	return err;
}

int gyroscope_operate(void *self, uint32_t command, void *buff_in, int size_in,
		void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value, sample_delay;
	struct bmg_i2c_data *priv = (struct bmg_i2c_data *)self;
	hwm_sensor_data *gyroscope_data;
	char buff[BMG_BUFSIZE];

	switch (command) {
	case SENSOR_DELAY:
	if ((buff_in == NULL) || (size_in < sizeof(int))) {
		GYRO_ERR("set delay parameter error\n");
		err = -EINVAL;
	} else {
		value = *(int *)buff_in;

		/*
		*Currently, fix data rate to 100Hz.
		*/
		sample_delay = BMI160_GYRO_ODR_100HZ;

		GYRO_LOG("sensor delay command: %d, sample_delay = %d\n",
			value, sample_delay);

		err = bmg_set_datarate(priv->client, sample_delay);
		if (err < 0)
			GYRO_ERR("set delay parameter error\n");

		if (value >= 40)
			atomic_set(&priv->filter, 0);
		else {
		#if defined(CONFIG_BMG_LOWPASS)
			priv->fir.num = 0;
			priv->fir.idx = 0;
			priv->fir.sum[BMG_AXIS_X] = 0;
			priv->fir.sum[BMG_AXIS_Y] = 0;
			priv->fir.sum[BMG_AXIS_Z] = 0;
			atomic_set(&priv->filter, 1);
		#endif
		}
	}
	break;
	case SENSOR_ENABLE:
	if ((buff_in == NULL) || (size_in < sizeof(int))) {
		GYRO_ERR("enable sensor parameter error\n");
		err = -EINVAL;
	} else {
		/* value:[0--->suspend, 1--->normal] */
		value = *(int *)buff_in;
		GYRO_LOG("sensor enable/disable command: %s\n",
			value ? "enable" : "disable");

		err = bmg_set_powermode(priv->client,
			(enum BMG_POWERMODE_ENUM)(!!value));
		if (err)
			GYRO_ERR("set power mode failed, err = %d\n", err);
	}
	break;
	case SENSOR_GET_DATA:
	if ((buff_out == NULL) || (size_out < sizeof(hwm_sensor_data))) {
		GYRO_ERR("get sensor data parameter error\n");
		err = -EINVAL;
	} else {
		gyroscope_data = (hwm_sensor_data *)buff_out;
		bmg_read_sensor_data(priv->client, buff, BMG_BUFSIZE);
		sscanf(buff, "%x %x %x", &gyroscope_data->values[0],
			&gyroscope_data->values[1], &gyroscope_data->values[2]);
		gyroscope_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
		gyroscope_data->value_divide = DEGREE_TO_RAD;
	}
	break;
	default:
	GYRO_ERR("gyroscope operate function no this parameter %d\n", command);
	err = -1;
	break;
	}

	return err;
}

static int bmg_open(struct inode *inode, struct file *file)
{
	file->private_data = obj_i2c_data;

	if (file->private_data == NULL) {
		GYRO_ERR("null pointer\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

static int bmg_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long bmg_unlocked_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	struct bmg_i2c_data *obj = (struct bmg_i2c_data *)file->private_data;
	struct i2c_client *client = obj->client;
	char strbuf[BMG_BUFSIZE] = "";
	int raw_offset[BMG_BUFSIZE] = {0};
	void __user *data;
	SENSOR_DATA sensor_data;
	long err = 0;
	int cali[BMG_AXES_NUM];

	if (obj == NULL)
		return -EFAULT;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
			(void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
			(void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		GYRO_ERR("access error: %08x, (%2d, %2d)\n",
			cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GYROSCOPE_IOCTL_INIT:
	bmg_init_client(client, 0);
	err = bmg_set_powermode(client, BMG_NORMAL_MODE);
	if (err) {
		err = -EFAULT;
		break;
	}
	break;
	case GYROSCOPE_IOCTL_READ_SENSORDATA:
	data = (void __user *) arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}

	bmg_read_sensor_data(client, strbuf, BMG_BUFSIZE);
	if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
		err = -EFAULT;
		break;
	}
	break;
	case GYROSCOPE_IOCTL_SET_CALI:
	/* data unit is degree/second */
	data = (void __user *)arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}
	if (copy_from_user(&sensor_data, data, sizeof(sensor_data))) {
		err = -EFAULT;
		break;
	}
	if (atomic_read(&obj->suspend)) {
		GYRO_ERR("perform calibration in suspend mode\n");
		err = -EINVAL;
	} else {
		/* convert: degree/second -> LSB */
		cali[BMG_AXIS_X] = sensor_data.x * obj->sensitivity;
		cali[BMG_AXIS_Y] = sensor_data.y * obj->sensitivity;
		cali[BMG_AXIS_Z] = sensor_data.z * obj->sensitivity;
		err = bmg_write_calibration(client, cali);
	}
	break;
	case GYROSCOPE_IOCTL_CLR_CALI:
	err = bmg_reset_calibration(client);
	break;
	case GYROSCOPE_IOCTL_GET_CALI:
	data = (void __user *)arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}
	err = bmg_read_calibration(client, cali, raw_offset);
	if (err)
		break;

	sensor_data.x = cali[BMG_AXIS_X] * obj->sensitivity;
	sensor_data.y = cali[BMG_AXIS_Y] * obj->sensitivity;
	sensor_data.z = cali[BMG_AXIS_Z] * obj->sensitivity;
	if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
		err = -EFAULT;
		break;
	}
	break;
	default:
	GYRO_ERR("unknown IOCTL: 0x%08x\n", cmd);
	err = -ENOIOCTLCMD;
	break;
	}

	return err;
}

static const struct file_operations bmg_fops = {
	.owner = THIS_MODULE,
	.open = bmg_open,
	.release = bmg_release,
	.unlocked_ioctl = bmg_unlocked_ioctl,
};

static struct miscdevice bmg_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gyroscope",
	.fops = &bmg_fops,
};

#ifndef CONFIG_HAS_EARLYSUSPEND
static int bmg_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err = 0;
	GYRO_FUN();

	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			GYRO_ERR("null pointer\n");
			return -EINVAL;
		}

		atomic_set(&obj->suspend, 1);
		err = bmg_set_powermode(obj->client, BMG_SUSPEND_MODE);
		if (err) {
			GYRO_ERR("bmg set suspend mode failed, err = %d\n",
				err);
			return err;
		}
		bmg_power(obj->hw, 0);
	}
	return err;
}

static int bmg_resume(struct i2c_client *client)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err;
	GYRO_FUN();

	if (obj == NULL) {
		GYRO_ERR("null pointer\n");
		return -EINVAL;
	}

	bmg_power(obj->hw, 1);
	err = bmg_init_client(client, 0);
	if (err) {
		GYRO_ERR("initialize client failed, err = %d\n", err);
		return err;
	}

	atomic_set(&obj->suspend, 0);
	return 0;
}
#else
static void bmi160_gyro_early_suspend(struct early_suspend *h)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err = 0;
	GYRO_FUN();

	if (obj == NULL) {
		GYRO_ERR("null pointer\n");
		return;
	}

	atomic_set(&obj->suspend, 1);
	err = bmg_set_powermode(obj->client, BMG_SUSPEND_MODE);
	if (err) {
		GYRO_ERR("bmg set suspend mode failed, err = %d\n",err);
		return;
	}
	bmg_power(obj->hw, 0);
	return;
}
static void bmi160_gyro_late_resume(struct early_suspend *h)
{
	struct bmg_i2c_data *obj = obj_i2c_data;
	int err;
	GYRO_FUN();

	if (obj == NULL) {
		GYRO_ERR("null pointer\n");
		return;
	}

	bmg_power(obj->hw, 1);
	err = bmg_init_client(obj->client, 0);
	if (err) {
		GYRO_ERR("initialize client failed, err = %d\n", err);
		return;
	}

	atomic_set(&obj->suspend, 0);
	return;
}
#endif/*CONFIG_HAS_EARLYSUSPEND*/

static int bmg_i2c_detect(struct i2c_client *client,
		struct i2c_board_info *info)
{
	strcpy(info->type, BMG_DEV_NAME);
	return 0;
}

// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int bmi160_gyro_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL

static int bmi160_gyro_enable_nodata(int en)
{
#ifdef USE_DAEMON

	if(en == 1) {
		atomic_set(&g_flag, 1);
	} else {
		atomic_set(&g_flag, 0);
	}

	/* set the flag */
	mutex_lock(&uplink_event_flag_mutex);
	uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
	mutex_unlock(&uplink_event_flag_mutex);
	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);

	return 0;
#else
	int res =0;
	int retry = 0;
	bool power=false;
	
	if(1==en)
	{
		power=true;
	}
	if(0==en)
	{
		power =false;
	}

	for(retry = 0; retry < 3; retry++){
		res = bmg_set_powermode(obj_i2c_data->client, power);
		if(res == 0)
		{
			GYRO_LOG("BMI160_SetPowerMode done\n");
			break;
		}
		GYRO_LOG("BMI160_SetPowerMode fail\n");
	}

	if(res != 0)
	{
		GYRO_LOG("BMI160_SetPowerMode fail!\n");
		return -1;
	}
	GYRO_LOG("bmi160_gyro_enable_nodata OK!\n");
	return 0;
#endif
}

static int bmi160_gyro_set_delay(u64 ns)
{
#ifdef USE_DAEMON
	int value = (int)ns/1000/1000 ;

	m4g_delay = value;

	/* set the flag */
	mutex_lock(&uplink_event_flag_mutex);
	uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_DELAY;
	mutex_unlock(&uplink_event_flag_mutex);

	/* wake up the wait queue */
	wake_up(&uplink_event_flag_wq);
#endif

	return 0;
}

static int bmi160_gyro_get_data(int* x ,int* y,int* z, int* status)
{
#ifdef USE_DAEMON

	mutex_lock(&sensor_data_mutex);

	*x = m4g_data[0];
	*y = m4g_data[1];
	*z = m4g_data[2];
	*status = m4g_data[3];

	mutex_unlock(&sensor_data_mutex);

	return 0;
#else
	char buff[BMG_BUFSIZE];
	bmg_read_sensor_data(obj_i2c_data->client, buff, BMG_BUFSIZE);
	
	sscanf(buff, "%x %x %x", x, y, z);		
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
#endif
}

extern struct i2c_client *bmi160_acc_i2c_client;

static int bmg_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct bmg_i2c_data *obj;
	struct hwmsen_object sobj;
	struct gyro_control_path ctl={0};
	struct gyro_data_path data={0};
	int err = 0;
	GYRO_FUN();

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}

	obj->hw = get_cust_gyro_hw();
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (err) {
		GYRO_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit_hwmsen_get_convert_failed;
	}

	obj_i2c_data = obj;
	obj->client = bmi160_acc_i2c_client;
	i2c_set_clientdata(obj->client, obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	obj->power_mode = BMG_UNDEFINED_POWERMODE;
	obj->range = BMG_UNDEFINED_RANGE;
	obj->datarate = BMI160_GYRO_ODR_RESERVED;
	mutex_init(&obj->lock);

#ifdef CONFIG_BMG_LOWPASS
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);

	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);
#endif

	err = bmg_init_client(obj->client, 1);
	if (err)
		goto exit_init_client_failed;

	err = misc_register(&bmg_device);
	if (err) {
		GYRO_ERR("misc device register failed, err = %d\n", err);
		goto exit_misc_device_register_failed;
	}

	//err = bmg_create_attr(&bmg_gyroscope_driver.driver);
	err = bmg_create_attr(&bmi160_gyro_init_info.platform_diver_addr->driver);
	if (err) {
		GYRO_ERR("create attribute failed, err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data= bmi160_gyro_open_report_data;
	ctl.enable_nodata = bmi160_gyro_enable_nodata;
	ctl.set_delay  = bmi160_gyro_set_delay;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = obj->hw->is_batch_supported;
	err = gyro_register_control_path(&ctl);
	if(err) {
		GYRO_ERR("register gyro control path err\n");
		goto exit_kfree;
	}

	data.get_data = bmi160_gyro_get_data;
	data.vender_div = DEGREE_TO_RAD;
	err = gyro_register_data_path(&data);
	if(err) {
		GYRO_ERR("gyro_register_data_path fail = %d\n", err);
		goto exit_kfree;
	}

	sobj.self = obj;
	sobj.polling = 1;
	sobj.sensor_operate = gyroscope_operate;
/*
	err = hwmsen_attach(ID_GYROSCOPE, &sobj);
	if (err) {
		GYRO_ERR("hwmsen attach failed, err = %d\n", err);
		goto exit_hwmsen_attach_failed;
	}
*/

#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 2,
	obj->early_drv.suspend  = bmi160_gyro_early_suspend,
	obj->early_drv.resume   = bmi160_gyro_late_resume,
	register_early_suspend(&obj->early_drv);
#endif
	bmi160_gyro_init_flag =0;
	GYRO_LOG("%s: OK\n", __func__);
	return 0;
/*
exit_hwmsen_attach_failed:
	bmg_delete_attr(&bmg_gyroscope_driver.driver);
*/
exit_create_attr_failed:
	misc_deregister(&bmg_device);
exit_misc_device_register_failed:
exit_init_client_failed:
exit_hwmsen_get_convert_failed:
exit_kfree:
	kfree(obj);
exit:
	bmi160_gyro_init_flag =-1;
	GYRO_ERR("err = %d\n", err);
	return err;
}

static int bmg_i2c_remove(struct i2c_client *client)
{
	int err = 0;

/*
	err = hwmsen_detach(ID_GYROSCOPE);
	if (err)
		GYRO_ERR("hwmsen_detach failed, err = %d\n", err);
*/
	//err = bmg_delete_attr(&bmg_gyroscope_driver.driver);
	err = bmg_delete_attr(&bmi160_gyro_init_info.platform_diver_addr->driver);
	if (err)
		GYRO_ERR("bmg_delete_attr failed, err = %d\n", err);

#if 0
	err = misc_deregister(&bmg_device);
	if (err)
		GYRO_ERR("misc_deregister failed, err = %d\n", err);
#endif

	obj_i2c_data = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static struct i2c_driver bmg_i2c_driver = {
	.driver = {
		.name = BMG_DEV_NAME,
	},
	.probe = bmg_i2c_probe,
	.remove	= bmg_i2c_remove,
	.detect	= bmg_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	.suspend = bmg_suspend,
	.resume = bmg_resume,
#endif
	.id_table = bmg_i2c_id,
};

#if 0
static int bmg_probe(struct platform_device *pdev)
{
	struct gyro_hw *hw = get_cust_gyro_hw();
	GYRO_FUN();

	bmg_power(hw, 1);
	if (i2c_add_driver(&bmg_i2c_driver)) {
		GYRO_ERR("add i2c driver failed\n");
		return -1;
	}

	return 0;
}

static int bmg_remove(struct platform_device *pdev)
{
	struct gyro_hw *hw = get_cust_gyro_hw();
	GYRO_FUN();

	bmg_power(hw, 0);
	i2c_del_driver(&bmg_i2c_driver);

	return 0;
}

static struct platform_driver bmg_gyroscope_driver = {
	.probe      = bmg_probe,
	.remove     = bmg_remove,
	.driver     = {
		.name   = "gyroscope",
		.owner  = THIS_MODULE,
	}
};
#endif

static int bmi160_gyro_remove(void)
{
    struct gyro_hw *hw = get_cust_gyro_hw();
    GYRO_FUN();    
    bmg_power(hw, 0);    
    i2c_del_driver(&bmg_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int bmi160_gyro_local_init(void)
{
    struct gyro_hw *hw = get_cust_gyro_hw();
	GYRO_LOG("fwq loccal init+++\n");

	bmg_power(hw, 1);
	if(i2c_add_driver(&bmg_i2c_driver))
	{
		GYRO_ERR("add driver error\n");
		return -1;
	}
	if(-1 == bmi160_gyro_init_flag)
	{
	   return -1;
	}
	GYRO_LOG("fwq loccal init---\n");
	return 0;
}

static struct gyro_init_info bmi160_gyro_init_info = {
		.name = "bmi160_gyro",
		.init = bmi160_gyro_local_init,
		.uninit = bmi160_gyro_remove,
};

static int __init bmg_init(void)
{
	struct gyro_hw *hw = get_cust_gyro_hw();

	GYRO_LOG("%s: bosch gyroscope driver version: %s\n",
	__func__, BMG_DRIVER_VERSION);

	i2c_register_board_info(hw->i2c_num, &bmg_i2c_info, 1);

	gyro_driver_add(&bmi160_gyro_init_info);
#if 0
	if (platform_driver_register(&bmg_gyroscope_driver)) {
		GYRO_ERR("register gyroscope platform driver failed\n");
		return -ENODEV;
	}
#endif
	return 0;
}

static void __exit bmg_exit(void)
{
	GYRO_FUN();
#if 0
	platform_driver_unregister(&bmg_gyroscope_driver);
#endif
}

module_init(bmg_init);
module_exit(bmg_exit);

MODULE_LICENSE("GPLv2");
MODULE_DESCRIPTION("BMG I2C Driver");
MODULE_AUTHOR("deliang.tao@bosch-sensortec.com");
MODULE_VERSION(BMG_DRIVER_VERSION);
