/* bmi160_acc motion sensor driver
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 *
 * VERSION: V1.5
 * HISTORY: V1.0 --- Driver creation
 *          V1.1 --- Add share I2C address function
 *          V1.2 --- Fix the bug that sometimes sensor is stuck after system resume.
 *          V1.3 --- Add FIFO interfaces.
 *          V1.4 --- Use basic i2c function to read fifo data instead of i2c DMA mode.
 *          V1.5 --- Add compensated value performed by MTK acceleration calibration process.
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/module.h>

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <accel.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include "bmi160_acc.h"
#include <linux/hwmsen_helper.h>

//shihaobin@yulong.com add for compitable begin 20150414
#include <linux/batch.h>
//shihaobin@yulong.com add for compitable end 20150414

//shihaobin add for acc_calibration begin 20150330
#include <linux/wakelock.h>

#define NAND_FLASH_WR_RD_SIZE 10

extern int read_acc_ps_cal_data_from_flash(unsigned int offset, char* output, int counts);
extern int write_acc_ps_cal_data_to_flash(unsigned int offset, char* input, int counts);
static int yulong_accel_Calibration(struct i2c_client *client, char *buf, int bufsize,int count,unsigned long cal_arg);
//static int yulong_accel_Calibration(struct i2c_client *client, char *buf, int bufsize,int count);
static int yulong_acc_ReadCalibration(struct i2c_client *client);
//shihaobin add for acc_calibration end 20150330

/*----------------------------------------------------------------------------*/
#define DEBUG 0
#define MISC_FOR_DAEMON
/*----------------------------------------------------------------------------*/
//#define CONFIG_BMI160_ACC_LOWPASS   /*apply low pass filter on output*/
#define SW_CALIBRATION
//#define FIFO_READ_USE_DMA_MODE_I2C
//tad3sgh add ++
#define BMM050_DEFAULT_DELAY  100
#define CALIBRATION_DATA_SIZE 12

/*----------------------------------------------------------------------------*/
/*
* Enable the driver to block e-compass daemon on suspend
*/
#define BMC050_BLOCK_DAEMON_ON_SUSPEND
#undef  BMC050_BLOCK_DAEMON_ON_SUSPEND
/*
* Enable gyroscope feature with BMC050
*/
#define BMC050_M4G
//#undef BMC050_M4G

#ifdef BMC050_M4G
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_GFLAG      _IOR(MSENSOR, 0x30, short)
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define ECOMPASS_IOC_GET_GDELAY     _IOR(MSENSOR, 0x31, int)
#endif //BMC050_M4G
/* !!! add a new definition in linux/sensors_io.h if possible !!! */
#define BMM_IOC_GET_EVENT_FLAG  ECOMPASS_IOC_GET_OPEN_STATUS
//add for non-block
#define BMM_IOC_GET_NONBLOCK_EVENT_FLAG _IOR(MSENSOR, 0x38, int)

#ifdef MISC_FOR_DAEMON
// calibration msensor and orientation data
static int sensor_data[CALIBRATION_DATA_SIZE];
#if defined(BMC050_M4G)
int m4g_data[CALIBRATION_DATA_SIZE];
#endif //BMC050_M4G
#endif

struct mutex sensor_data_mutex;
DECLARE_WAIT_QUEUE_HEAD(uplink_event_flag_wq);

#ifdef MISC_FOR_DAEMON
static int bmm050d_delay = BMM050_DEFAULT_DELAY;
#endif

#ifdef BMC050_M4G
int m4g_delay = BMM050_DEFAULT_DELAY;
#endif //BMC050_M4G

static atomic_t m_flag = ATOMIC_INIT(0);
static atomic_t o_flag = ATOMIC_INIT(0);
#ifdef BMC050_M4G
atomic_t g_flag = ATOMIC_INIT(0);
#endif //BMC050_M4G

#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
static atomic_t driver_suspend_flag = ATOMIC_INIT(0);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND

struct mutex uplink_event_flag_mutex;
/* uplink event flag */
volatile u32 uplink_event_flag = 0;
/* uplink event flag bitmap */
enum {
  /* active */
  BMMDRV_ULEVT_FLAG_O_ACTIVE = 0x0001,
  BMMDRV_ULEVT_FLAG_M_ACTIVE = 0x0002,
  BMMDRV_ULEVT_FLAG_G_ACTIVE = 0x0004,

  /* delay */
  BMMDRV_ULEVT_FLAG_O_DELAY = 0x0100,
  BMMDRV_ULEVT_FLAG_M_DELAY = 0x0200,
  BMMDRV_ULEVT_FLAG_G_DELAY = 0x0400,

    /* all */
    BMMDRV_ULEVT_FLAG_ALL = 0xffff
};

//tad3sgh add --
#define MAX_FIFO_F_LEVEL 32
#define MAX_FIFO_F_BYTES 6

/*----------------------------------------------------------------------------*/
#define BMI160_ACC_AXIS_X          0
#define BMI160_ACC_AXIS_Y          1
#define BMI160_ACC_AXIS_Z          2
#define BMI160_ACC_AXES_NUM        3
#define BMI160_ACC_DATA_LEN        6
#define BMI160_DEV_NAME        "bmi160_acc"

#define BMI160_ACC_MODE_NORMAL      0
#define BMI160_ACC_MODE_LOWPOWER    1
#define BMI160_ACC_MODE_SUSPEND     2

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id bmi160_acc_i2c_id[] = {{BMI160_DEV_NAME,0},{}};
//static struct i2c_board_info __initdata bmi160_acc_i2c_info ={ I2C_BOARD_INFO(BMI160_DEV_NAME, BMI160_I2C_ADDR)};
static struct i2c_board_info __initdata bmi160_acc_i2c_info ={ I2C_BOARD_INFO(BMI160_DEV_NAME, BMI160_I2C_ADDR)};

/*----------------------------------------------------------------------------*/
static int bmi160_acc_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int bmi160_acc_i2c_remove(struct i2c_client *client);

//shihaobin@yulong.com add for compitable begin 20150414
static int bmi160_acc_local_init(void);
static int bmi160_acc_remove(void);

//shihaobin@yulong.com add for compitable end 20150414

/*----------------------------------------------------------------------------*/
typedef enum {
    BMA_TRC_FILTER  = 0x01,
    BMA_TRC_RAWDATA = 0x02,
    BMA_TRC_IOCTL   = 0x04,
    BMA_TRC_CALI    = 0X08,
    BMA_TRC_INFO    = 0X10,
} BMA_TRC;
/*----------------------------------------------------------------------------*/
struct scale_factor{
    u8  whole;
    u8  fraction;
};
/*----------------------------------------------------------------------------*/
struct data_resolution {
    struct scale_factor scalefactor;
    int                 sensitivity;
};
/*----------------------------------------------------------------------------*/
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
    s16 raw[C_MAX_FIR_LENGTH][BMI160_ACC_AXES_NUM];
    int sum[BMI160_ACC_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct bmi160_acc_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;

    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
    atomic_t                filter;
    s16                     cali_sw[BMI160_ACC_AXES_NUM+1];
    struct mutex lock;

    /*data*/
    s8                      offset[BMI160_ACC_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[BMI160_ACC_AXES_NUM+1];
    u8          fifo_count;

    u8    fifo_data_sel;
    u16   fifo_bytecount;
    struct  odr_t odr;
    u64   fifo_time;
    atomic_t  layout;

#if defined(CONFIG_BMI160_ACC_LOWPASS)
    atomic_t                firlen;
    atomic_t                fir_en;
    struct data_filter      fir;
#endif
    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif
};

/*----------------------------------------------------------------------------*/
struct i2c_client *bmi160_acc_i2c_client = NULL;
#if 0
static struct platform_driver bmi160_acc_gsensor_driver;
#endif
static struct acc_init_info bmi160_acc_init_info;
static struct bmi160_acc_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = true;
static GSENSOR_VECTOR3D gsensor_gain;

extern int bmi160_acc_init_flag; // 0<==>OK -1 <==> fail

/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(GSE_TAG fmt, ##args)

/*!bmi160 sensor time depends on ODR */
static const struct bmi_sensor_time_odr_tbl
                sensortime_duration_tbl[TS_MAX_HZ] = {
        {0x010000, 2560000, 0x00ffff},/*2560ms, 0.39hz, odr=resver*/
        {0x008000, 1280000, 0x007fff},/*1280ms, 0.78hz, odr_acc=1*/
        {0x004000, 640000, 0x003fff},/*640ms, 1.56hz, odr_acc=2*/
        {0x002000, 320000, 0x001fff},/*320ms, 3.125hz, odr_acc=3*/
        {0x001000, 160000, 0x000fff},/*160ms, 6.25hz, odr_acc=4*/
        {0x000800, 80000,  0x0007ff},/*80ms, 12.5hz*/
        {0x000400, 40000, 0x0003ff},/*40ms, 25hz, odr_acc = odr_gyro =6*/
        {0x000200, 20000, 0x0001ff},/*20ms, 50hz, odr = 7*/
        {0x000100, 10000, 0x0000ff},/*10ms, 100hz, odr=8*/
        {0x000080, 5000, 0x00007f},/*5ms, 200hz, odr=9*/
        {0x000040, 2500, 0x00003f},/*2.5ms, 400hz, odr=10*/
        {0x000020, 1250, 0x00001f},/*1.25ms, 800hz, odr=11*/
        {0x000010, 625, 0x00000f},/*0.625ms, 1600hz, odr=12*/

};

static unsigned char g_fifo_data_arr[2048];/*1024 + 12*4*/

/*----------------------------------------------------------------------------*/
static struct data_resolution bmi160_acc_data_resolution[1] = {
    {{1, 0}, 16384},
};
/*----------------------------------------------------------------------------*/
static struct data_resolution bmi160_acc_offset_resolution = {{3, 9}, 256};

#ifdef FIFO_READ_USE_DMA_MODE_I2C
#include <linux/dma-mapping.h>

#ifndef I2C_MASK_FLAG
#define I2C_MASK_FLAG   (0x00ff)
#define I2C_DMA_FLAG    (0x2000)
#endif

static void *I2CDMABuf_va = NULL;
static dma_addr_t I2CDMABuf_pa;

static int i2c_dma_read_fifo(struct i2c_client *client,
    uint8_t regaddr, uint8_t *readbuf, int32_t readlen)
{
  int ret;
  struct i2c_msg msg;

  ret= i2c_master_send(client, &regaddr, 1);
  if (ret < 0) {
    GSE_ERR("send command error!!\n");
    return -EFAULT;
  }

  msg.addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
  msg.flags = client->flags & I2C_M_TEN;
  msg.flags |= I2C_M_RD;
  msg.len = readlen;
  msg.buf = (char *)I2CDMABuf_pa;

  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret < 0) {
    GSE_ERR("dma receive data error!!\n");
    return -EFAULT;
  }

  memcpy(readbuf, I2CDMABuf_va, readlen);
  return ret;
}
#endif

/* I2C operation functions */
static int bma_i2c_read_block(struct i2c_client *client,
            u8 addr, u8 *data, u8 len)
{
  u8 beg = addr;
  struct i2c_msg msgs[2] = {
    {
      .addr = client->addr, .flags = 0,
      .len = 1,   .buf = &beg
    },
    {
      .addr = client->addr, .flags = I2C_M_RD,
      .len = len,   .buf = data,
    }
  };
  int err;

  if (!client)
    return -EINVAL;
  else if (len > C_I2C_FIFO_SIZE) {
    //GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
    //return -EINVAL;
  }

    err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
    if (err != 2) {
        GSE_ERR("i2c_transfer error: (%d %p %d) %d\n",
            addr, data, len, err);
        err = -EIO;
    } else {
        err = 0;/*no error*/
    }

  return err;
}
#define I2C_BUFFER_SIZE 256
static int bma_i2c_write_block(struct i2c_client *client, u8 addr,
            u8 *data, u8 len)
{
  /*
  *because address also occupies one byte,
  *the maximum length for write is 7 bytes
  */
  int err, idx = 0, num = 0;
  char buf[32];

  if (!client)
    return -EINVAL;
  else if (len > C_I2C_FIFO_SIZE) {
    //GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
    //return -EINVAL;
  }

  buf[num++] = addr;
  for (idx = 0; idx < len; idx++)
    buf[num++] = data[idx];

  err = i2c_master_send(client, buf, num);
  if (err < 0) {
    GSE_ERR("send command error!!\n");
    return -EFAULT;
  } else {
    err = 0;/*no error*/
  }
  return err;
}

//bool __attribute__((weak)) hwPowerOn(MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt, char *mode_name)
bool __attribute__((weak)) acc_hwPowerOn(MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt, char *mode_name)
{
  return 0;
}

//bool __attribute__((weak)) hwPowerDown(MT65XX_POWER powerId, char *mode_name)
bool __attribute__((weak)) acc_hwPowerDown(MT65XX_POWER powerId, char *mode_name)
{
  return 0;
}

/*--------------------BMI160_ACC power control function----------------------------------*/
static void BMI160_ACC_power(struct acc_hw *hw, unsigned int on)
{
  static unsigned int power_on = 0;

  if(hw->power_id != POWER_NONE_MACRO)    // have externel LDO
  {
    GSE_LOG("power %s\n", on ? "on" : "off");
    if(power_on == on)  // power status not change
    {
      GSE_LOG("ignore power control: %d\n", on);
    }
    else if(on) // power on
    {
      if(!acc_hwPowerOn(hw->power_id, hw->power_vol, "BMI160_ACC"))
      {
        GSE_ERR("power on fails!!\n");
      }
    }
    else  // power off
    {
      if (!acc_hwPowerDown(hw->power_id, "BMI160_ACC"))
      {
        GSE_ERR("power off fail!!\n");
      }
    }
  }
  power_on = on;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetDataResolution(struct bmi160_acc_i2c_data *obj)
{

/*set g sensor dataresolution here*/

/*BMI160_ACC only can set to 10-bit dataresolution, so do nothing in bmi160_acc driver here*/

/*end of set dataresolution*/

 /*we set measure range from -2g to +2g in BMI160_ACC_SetDataFormat(client, BMI160_ACC_RANGE_2G),
                                                    and set 10-bit dataresolution BMI160_ACC_SetDataResolution()*/

 /*so bmi160_acc_data_resolution[0] set value as {{ 3, 9}, 256} when declaration, and assign the value to obj->reso here*/

  obj->reso = &bmi160_acc_data_resolution[0];
  return 0;

/*if you changed the measure range, for example call: BMI160_ACC_SetDataFormat(client, BMI160_ACC_RANGE_4G),
you must set the right value to bmi160_acc_data_resolution*/

}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadData(struct i2c_client *client, s16 data[BMI160_ACC_AXES_NUM])
{
#ifdef CONFIG_BMI160_ACC_LOWPASS
  struct bmi160_acc_i2c_data *priv = obj_i2c_data;
#endif
  u8 addr = BMI160_USER_DATA_14_ACC_X_LSB__REG;
  u8 buf[BMI160_ACC_DATA_LEN] = {0};
  int err = 0;

  if(NULL == client)
  {
    return -EINVAL;
  }

  err = bma_i2c_read_block(client, addr, buf, BMI160_ACC_DATA_LEN);
  if(err) {
    GSE_ERR("error: %d\n", err);
  }
  else
  {
    /* Convert sensor raw data to 16-bit integer */
    /* Data X */
    data[BMI160_ACC_AXIS_X] = (s16) ((((s32)((s8)buf[1]))
        << BMI160_SHIFT_8_POSITION) | (buf[0]));
    /* Data Y */
    data[BMI160_ACC_AXIS_Y] = (s16) ((((s32)((s8)buf[3]))
        << BMI160_SHIFT_8_POSITION) | (buf[2]));
    /* Data Z */
    data[BMI160_ACC_AXIS_Z] = (s16) ((((s32)((s8)buf[5]))
        << BMI160_SHIFT_8_POSITION) | (buf[4]));

#ifdef CONFIG_BMI160_ACC_LOWPASS
    if(atomic_read(&priv->filter))
    {
      if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
      {
        int idx, firlen = atomic_read(&priv->firlen);
        if(priv->fir.num < firlen)
        {
          priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_X] = data[BMI160_ACC_AXIS_X];
          priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_Y] = data[BMI160_ACC_AXIS_Y];
          priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_Z] = data[BMI160_ACC_AXIS_Z];
          priv->fir.sum[BMI160_ACC_AXIS_X] += data[BMI160_ACC_AXIS_X];
          priv->fir.sum[BMI160_ACC_AXIS_Y] += data[BMI160_ACC_AXIS_Y];
          priv->fir.sum[BMI160_ACC_AXIS_Z] += data[BMI160_ACC_AXIS_Z];
          if(atomic_read(&priv->trace) & BMA_TRC_FILTER)
          {
            GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
              priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_X], priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_Y], priv->fir.raw[priv->fir.num][BMI160_ACC_AXIS_Z],
              priv->fir.sum[BMI160_ACC_AXIS_X], priv->fir.sum[BMI160_ACC_AXIS_Y], priv->fir.sum[BMI160_ACC_AXIS_Z]);
          }
          priv->fir.num++;
          priv->fir.idx++;
        }
        else
        {
          idx = priv->fir.idx % firlen;
          priv->fir.sum[BMI160_ACC_AXIS_X] -= priv->fir.raw[idx][BMI160_ACC_AXIS_X];
          priv->fir.sum[BMI160_ACC_AXIS_Y] -= priv->fir.raw[idx][BMI160_ACC_AXIS_Y];
          priv->fir.sum[BMI160_ACC_AXIS_Z] -= priv->fir.raw[idx][BMI160_ACC_AXIS_Z];
          priv->fir.raw[idx][BMI160_ACC_AXIS_X] = data[BMI160_ACC_AXIS_X];
          priv->fir.raw[idx][BMI160_ACC_AXIS_Y] = data[BMI160_ACC_AXIS_Y];
          priv->fir.raw[idx][BMI160_ACC_AXIS_Z] = data[BMI160_ACC_AXIS_Z];
          priv->fir.sum[BMI160_ACC_AXIS_X] += data[BMI160_ACC_AXIS_X];
          priv->fir.sum[BMI160_ACC_AXIS_Y] += data[BMI160_ACC_AXIS_Y];
          priv->fir.sum[BMI160_ACC_AXIS_Z] += data[BMI160_ACC_AXIS_Z];
          priv->fir.idx++;
          data[BMI160_ACC_AXIS_X] = priv->fir.sum[BMI160_ACC_AXIS_X]/firlen;
          data[BMI160_ACC_AXIS_Y] = priv->fir.sum[BMI160_ACC_AXIS_Y]/firlen;
          data[BMI160_ACC_AXIS_Z] = priv->fir.sum[BMI160_ACC_AXIS_Z]/firlen;
          if(atomic_read(&priv->trace) & BMA_TRC_FILTER)
          {
            GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
            priv->fir.raw[idx][BMI160_ACC_AXIS_X], priv->fir.raw[idx][BMI160_ACC_AXIS_Y], priv->fir.raw[idx][BMI160_ACC_AXIS_Z],
            priv->fir.sum[BMI160_ACC_AXIS_X], priv->fir.sum[BMI160_ACC_AXIS_Y], priv->fir.sum[BMI160_ACC_AXIS_Z],
            data[BMI160_ACC_AXIS_X], data[BMI160_ACC_AXIS_Y], data[BMI160_ACC_AXIS_Z]);
          }
        }
      }
    }
#endif
  }
  return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadOffset(struct i2c_client *client, s8 ofs[BMI160_ACC_AXES_NUM])
{
  int err=0;
#ifdef SW_CALIBRATION
  ofs[0]=ofs[1]=ofs[2]=0x0;
#else
  err = bma_i2c_read_block(client, BMI160_ACC_REG_OFSX, ofs, BMI160_ACC_AXES_NUM);
  if(err) {
    GSE_ERR("error: %d\n", err);
  }
#endif
    //GSE_LOG("offesx=%x, y=%x, z=%x",ofs[0],ofs[1],ofs[2]);

    return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ResetCalibration(struct i2c_client *client)
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int err=0;

    #ifdef SW_CALIBRATION

  #else
  u8 ofs[4]={0,0,0,0};
  err = bma_i2c_write_block(client, BMI160_ACC_REG_OFSX, ofs, 4);
  if(err) {
    GSE_ERR("error: %d\n", err);
  }
  #endif

    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
    memset(obj->offset, 0x00, sizeof(obj->offset));
    return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadCalibration(struct i2c_client *client, int dat[BMI160_ACC_AXES_NUM])
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int err = 0;
    int mul;

#ifdef SW_CALIBRATION
    mul = 0;//only SW Calibration, disable HW Calibration
#else
    err = BMI160_ACC_ReadOffset(client, obj->offset);
    if(err) {
      GSE_ERR("read offset fail, %d\n", err);
      return err;
    }
    mul = obj->reso->sensitivity/bmi160_acc_offset_resolution.sensitivity;
#endif

    dat[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]*(obj->offset[BMI160_ACC_AXIS_X]*mul + obj->cali_sw[BMI160_ACC_AXIS_X]);
    dat[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*(obj->offset[BMI160_ACC_AXIS_Y]*mul + obj->cali_sw[BMI160_ACC_AXIS_Y]);
    dat[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*(obj->offset[BMI160_ACC_AXIS_Z]*mul + obj->cali_sw[BMI160_ACC_AXIS_Z]);

    return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadCalibrationEx(struct i2c_client *client, int act[BMI160_ACC_AXES_NUM], int raw[BMI160_ACC_AXES_NUM])
{
    /*raw: the raw calibration data; act: the actual calibration data*/
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int mul;

#ifdef SW_CALIBRATION
    mul = 0;//only SW Calibration, disable HW Calibration
#else
  int err;
  err = BMI160_ACC_ReadOffset(client, obj->offset);
  if(err) {
    GSE_ERR("read offset fail, %d\n", err);
    return err;
  }
  mul = obj->reso->sensitivity/bmi160_acc_offset_resolution.sensitivity;
#endif

    raw[BMI160_ACC_AXIS_X] = obj->offset[BMI160_ACC_AXIS_X]*mul + obj->cali_sw[BMI160_ACC_AXIS_X];
    raw[BMI160_ACC_AXIS_Y] = obj->offset[BMI160_ACC_AXIS_Y]*mul + obj->cali_sw[BMI160_ACC_AXIS_Y];
    raw[BMI160_ACC_AXIS_Z] = obj->offset[BMI160_ACC_AXIS_Z]*mul + obj->cali_sw[BMI160_ACC_AXIS_Z];

    act[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]*raw[BMI160_ACC_AXIS_X];
    act[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*raw[BMI160_ACC_AXIS_Y];
    act[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*raw[BMI160_ACC_AXIS_Z];

    return 0;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_WriteCalibration(struct i2c_client *client, int dat[BMI160_ACC_AXES_NUM])
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int err = 0;
    //shihaobin@yulong.com modify for debug 20150416 begin
    int cali[BMI160_ACC_AXES_NUM] = {0};
    int raw[BMI160_ACC_AXES_NUM] = {0};



#ifndef SW_CALIBRATION
    int lsb = bmi160_acc_offset_resolution.sensitivity;
    int divisor = obj->reso->sensitivity/lsb;
#endif

    /*if(err = BMI160_ACC_ReadCalibrationEx(client, cali, raw))
    {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    }*/
    //shihaobin@yulong.com modify for debug 20150416 end
    GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
        raw[BMI160_ACC_AXIS_X], raw[BMI160_ACC_AXIS_Y], raw[BMI160_ACC_AXIS_Z],
        obj->offset[BMI160_ACC_AXIS_X], obj->offset[BMI160_ACC_AXIS_Y], obj->offset[BMI160_ACC_AXIS_Z],
        obj->cali_sw[BMI160_ACC_AXIS_X], obj->cali_sw[BMI160_ACC_AXIS_Y], obj->cali_sw[BMI160_ACC_AXIS_Z]);


    /*calculate the real offset expected by caller*/
    cali[BMI160_ACC_AXIS_X] += dat[BMI160_ACC_AXIS_X];
    cali[BMI160_ACC_AXIS_Y] += dat[BMI160_ACC_AXIS_Y];
    cali[BMI160_ACC_AXIS_Z] += dat[BMI160_ACC_AXIS_Z];

    printk("yl_sensor_debug cali[0-2]= %d, %d, %d,  data[0-2]=%d, %d, %d\n",cali[BMI160_ACC_AXIS_X],cali[BMI160_ACC_AXIS_Y],cali[BMI160_ACC_AXIS_Z],dat[BMI160_ACC_AXIS_X],dat[BMI160_ACC_AXIS_Y],dat[BMI160_ACC_AXIS_Z]);
    printk("yl_sensor_debug cali sign obj->cvt.sign[0-2]=%d %d %d\n", obj->cvt.sign[BMI160_ACC_AXIS_X], obj->cvt.sign[BMI160_ACC_AXIS_Y], obj->cvt.sign[BMI160_ACC_AXIS_Z]);

    GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n",
        dat[BMI160_ACC_AXIS_X], dat[BMI160_ACC_AXIS_Y], dat[BMI160_ACC_AXIS_Z]);

#ifdef SW_CALIBRATION
    obj->cali_sw[BMI160_ACC_AXIS_X] = obj->cvt.sign[BMI160_ACC_AXIS_X]*(cali[obj->cvt.map[BMI160_ACC_AXIS_X]]);
    obj->cali_sw[BMI160_ACC_AXIS_Y] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Y]]);
    obj->cali_sw[BMI160_ACC_AXIS_Z] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Z]]);

    printk("yl_sensor_debug cali_sw[0-2]=%d, %d, %d\n",obj->cali_sw[BMI160_ACC_AXIS_X],obj->cali_sw[BMI160_ACC_AXIS_Y],obj->cali_sw[BMI160_ACC_AXIS_Z]);
#else
    obj->offset[BMI160_ACC_AXIS_X] = (s8)(obj->cvt.sign[BMI160_ACC_AXIS_X]*(cali[obj->cvt.map[BMI160_ACC_AXIS_X]])/(divisor));
    obj->offset[BMI160_ACC_AXIS_Y] = (s8)(obj->cvt.sign[BMI160_ACC_AXIS_Y]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Y]])/(divisor));
    obj->offset[BMI160_ACC_AXIS_Z] = (s8)(obj->cvt.sign[BMI160_ACC_AXIS_Z]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Z]])/(divisor));

    /*convert software calibration using standard calibration*/
    obj->cali_sw[BMI160_ACC_AXIS_X] = obj->cvt.sign[BMI160_ACC_AXIS_X]*(cali[obj->cvt.map[BMI160_ACC_AXIS_X]])%(divisor);
    obj->cali_sw[BMI160_ACC_AXIS_Y] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Y]])%(divisor);
    obj->cali_sw[BMI160_ACC_AXIS_Z] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*(cali[obj->cvt.map[BMI160_ACC_AXIS_Z]])%(divisor);

    GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
        obj->offset[BMI160_ACC_AXIS_X]*divisor + obj->cali_sw[BMI160_ACC_AXIS_X],
        obj->offset[BMI160_ACC_AXIS_Y]*divisor + obj->cali_sw[BMI160_ACC_AXIS_Y],
        obj->offset[BMI160_ACC_AXIS_Z]*divisor + obj->cali_sw[BMI160_ACC_AXIS_Z],
        obj->offset[BMI160_ACC_AXIS_X], obj->offset[BMI160_ACC_AXIS_Y], obj->offset[BMI160_ACC_AXIS_Z],
        obj->cali_sw[BMI160_ACC_AXIS_X], obj->cali_sw[BMI160_ACC_AXIS_Y], obj->cali_sw[BMI160_ACC_AXIS_Z]);

  err = bma_i2c_write_block(obj->client, BMI160_ACC_REG_OFSX, obj->offset, BMI160_ACC_AXES_NUM);
  if(err) {
    GSE_ERR("write offset fail: %d\n", err);
    return err;
  }
#endif

    return err;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_CheckDeviceID(struct i2c_client *client)
{
    u8 databuf[2];
    int res = 0;

    memset(databuf, 0, sizeof(u8)*2);

  res = bma_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, databuf, 0x01);
  res = bma_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, databuf, 0x01);
  if(res < 0)
    goto exit_BMI160_ACC_CheckDeviceID;

  switch (databuf[0]) {
  case SENSOR_CHIP_ID_BMI:
  case SENSOR_CHIP_ID_BMI_C2:
  case SENSOR_CHIP_ID_BMI_C3:
    GSE_LOG("BMI160_ACC_CheckDeviceID %d pass!\n ", databuf[0]);
    break;
  default:
    GSE_LOG("BMI160_ACC_CheckDeviceID %d failt!\n ", databuf[0]);
    break;
  }

  exit_BMI160_ACC_CheckDeviceID:
  if (res < 0)
  {
    return BMI160_ACC_ERR_I2C;
  }

    return BMI160_ACC_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetPowerMode(struct i2c_client *client, bool enable)
{
    u8 databuf[2] = {0};
    int res = 0;
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;

    if(enable == sensor_power )
    {
        GSE_LOG("Sensor power status is newest!\n");
        return BMI160_ACC_SUCCESS;
    }

  mutex_lock(&obj->lock);
  if(enable == TRUE)
  {
    databuf[0] = CMD_PMU_ACC_NORMAL;
  }
  else
  {
    databuf[0] = CMD_PMU_ACC_SUSPEND;
  }

    res = bma_i2c_write_block(client,
            BMI160_CMD_COMMANDS__REG, &databuf[0], 1);
    if(res < 0)
    {
        GSE_ERR("set power mode failed, res = %d\n", res);
        mutex_unlock(&obj->lock);
        return BMI160_ACC_ERR_I2C;
    }
    sensor_power = enable;
    mdelay(4);
    mutex_unlock(&obj->lock);

    return BMI160_ACC_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    u8 databuf[2] = {0};
    int res = 0;

    mutex_lock(&obj->lock);
    res = bma_i2c_read_block(client,
        BMI160_USER_ACC_RANGE__REG, &databuf[0], 1);
    databuf[0] = BMI160_SET_BITSLICE(databuf[0],
        BMI160_USER_ACC_RANGE, dataformat);
    res += bma_i2c_write_block(client,
        BMI160_USER_ACC_RANGE__REG, &databuf[0], 1);
    mdelay(1);

    if(res < 0)
    {
        GSE_ERR("set data format failed, res = %d\n", res);
        mutex_unlock(&obj->lock);
        return BMI160_ACC_ERR_I2C;
    }
    mutex_unlock(&obj->lock);

  return BMI160_ACC_SetDataResolution(obj);
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetBWRate(struct i2c_client *client, u8 bwrate)
{
    u8 databuf[2] = {0};
    int res = 0;
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;

        GSE_LOG("[%s] bandwidth = %d\n", __func__, bwrate);

    mutex_lock(&obj->lock);
    res = bma_i2c_read_block(client,
        BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
    //databuf[0] = databuf[0] & 0x0F; //shihaobin@yulong.com add for open filter 20150417
    databuf[0] = BMI160_SET_BITSLICE(databuf[0],
        BMI160_USER_ACC_CONF_ODR, bwrate);
    res += bma_i2c_write_block(client,
        BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
    mdelay(1);

    if(res < 0)
    {
        GSE_ERR("set bandwidth failed, res = %d\n", res);
        mutex_unlock(&obj->lock);
        return BMI160_ACC_ERR_I2C;
    }
    mutex_unlock(&obj->lock);

    return BMI160_ACC_SUCCESS;
}

//shihaobin@yulong.com add for open lowpass filter begin 20150420
static int BMI160_ACC_SetOSR4(struct i2c_client *client)
{
    int res = 0;
    uint8_t databuf[2] = {0};
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    uint8_t bandwidth = BMI160_ACCEL_OSR4_AVG1;
    /*      0       -       enable
     *      1       -       disable */
    uint8_t accel_undersampling_parameter = 0;

    GSE_LOG("[%s] acc_bmp %d, acc_us %d\n", __func__,
            bandwidth, accel_undersampling_parameter);

    mutex_lock(&obj->lock);
    res = bma_i2c_read_block(client,
        BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
    databuf[0] = BMI160_SET_BITSLICE(databuf[0],
            BMI160_USER_ACC_CONF_ACC_BWP, bandwidth);
    databuf[0] = BMI160_SET_BITSLICE(databuf[0],
            BMI160_USER_ACC_CONF_ACC_UNDER_SAMPLING,
            accel_undersampling_parameter);
    res += bma_i2c_write_block(client,
        BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
    mdelay(1);

    if(res < 0)
    {
        GSE_ERR("set OSR failed, res = %d\n", res);
        mutex_unlock(&obj->lock);
        return BMI160_ACC_ERR_I2C;
    }
    mutex_unlock(&obj->lock);

    return BMI160_ACC_SUCCESS;
}
//shihaobin@yulong.com add for open lowpass filter end 20150420

/*----------------------------------------------------------------------------*/
static int BMI160_ACC_SetIntEnable(struct i2c_client *client, u8 intenable)
{
    int res = 0;
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;

    mutex_lock(&obj->lock);
    res = bma_i2c_write_block(client, BMI160_USER_INT_EN_0_ADDR, &intenable, 0x01);
    mdelay(1);
    if(res != BMI160_ACC_SUCCESS)
    {
        mutex_unlock(&obj->lock);
        return res;
    }

    res = bma_i2c_write_block(client, BMI160_USER_INT_EN_1_ADDR, &intenable, 0x01);
    mdelay(1);
    if(res != BMI160_ACC_SUCCESS)
    {
        mutex_unlock(&obj->lock);
        return res;
    }

  res = bma_i2c_write_block(client, BMI160_USER_INT_EN_2_ADDR, &intenable, 0x01);
  mdelay(1);
  if(res != BMI160_ACC_SUCCESS)
  {
    mutex_unlock(&obj->lock);
    return res;
  }
  mutex_unlock(&obj->lock);
  GSE_LOG("BMI160_ACC disable interrupt ...\n");

    /*for disable interrupt function*/

    return BMI160_ACC_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int bmi160_acc_init_client(struct i2c_client *client, int reset_cali)
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int res = 0;
    GSE_LOG("bmi160_acc_init_client \n");

  res = BMI160_ACC_CheckDeviceID(client);
  if(res != BMI160_ACC_SUCCESS)
  {
    return res;
  }
  GSE_LOG("BMI160_ACC_CheckDeviceID ok \n");

  res = BMI160_ACC_SetBWRate(client, BMI160_ACCEL_ODR_200HZ);
  if(res != BMI160_ACC_SUCCESS )
  {
    return res;
  }
  GSE_LOG("BMI160_ACC_SetBWRate OK!\n");

    //shihaobin@yulong.com add for open lowpass filter begin 20150420
    res = BMI160_ACC_SetOSR4(client);
  if(res != BMI160_ACC_SUCCESS )
  {
    return res;
  }
  GSE_LOG("BMI160_ACC_SetOSR4 OK!\n");
  //shihaobin@yulong.com add for open lowpass filter end 20150420

  res = BMI160_ACC_SetDataFormat(client, BMI160_ACCEL_RANGE_2G);
  if(res != BMI160_ACC_SUCCESS)
  {
    return res;
  }
  GSE_LOG("BMI160_ACC_SetDataFormat OK!\n");

    gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

  res = BMI160_ACC_SetIntEnable(client, 0x00);
  if(res != BMI160_ACC_SUCCESS)
  {
    return res;
  }
  GSE_LOG("BMI160_ACC disable interrupt function!\n");

  res = BMI160_ACC_SetPowerMode(client, false);
  if(res != BMI160_ACC_SUCCESS)
  {
    return res;
  }
  GSE_LOG("BMI160_ACC_SetPowerMode OK!\n");

    if(0 != reset_cali)
    {
        /*reset calibration only in power on*/
        res = BMI160_ACC_ResetCalibration(client);
        if(res != BMI160_ACC_SUCCESS)
        {
            return res;
        }
    }
    GSE_LOG("bmi160_acc_init_client OK!\n");
#ifdef CONFIG_BMI160_ACC_LOWPASS
    memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

    mdelay(20);

    return BMI160_ACC_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
    u8 databuf[10];

    memset(databuf, 0, sizeof(u8)*10);

    if((NULL == buf)||(bufsize<=30))
    {
        return -1;
    }

    if(NULL == client)
    {
        *buf = 0;
        return -2;
    }

    sprintf(buf, "bmi160_acc");
    return 0;
}
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_CompassReadData(struct i2c_client *client, char *buf, int bufsize)
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    //u8 databuf[20];
    int acc[BMI160_ACC_AXES_NUM];
    int res = 0;
    s16 databuf[BMI160_ACC_AXES_NUM];
    //memset(databuf, 0, sizeof(u8)*10);

    if(NULL == buf)
    {
        return -1;
    }
    if(NULL == client)
    {
        *buf = 0;
        return -2;
    }

  if(sensor_power == FALSE)
  {
    res = BMI160_ACC_SetPowerMode(client, true);
    if(res)
    {
      GSE_ERR("Power on bmi160_acc error %d!\n", res);
    }
  }

  res = BMI160_ACC_ReadData(client, databuf);
  if(res)
  {
    GSE_ERR("I2C error: ret value=%d", res);
    return -3;
  }
  else
  {
    /* Add compensated value performed by MTK calibration process*/
    databuf[BMI160_ACC_AXIS_X] += obj->cali_sw[BMI160_ACC_AXIS_X];
    databuf[BMI160_ACC_AXIS_Y] += obj->cali_sw[BMI160_ACC_AXIS_Y];
    databuf[BMI160_ACC_AXIS_Z] += obj->cali_sw[BMI160_ACC_AXIS_Z];

        /*remap coordinate*/
        acc[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]*databuf[BMI160_ACC_AXIS_X];
        acc[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*databuf[BMI160_ACC_AXIS_Y];
        acc[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*databuf[BMI160_ACC_AXIS_Z];
        //GSE_LOG("cvt x=%d, y=%d, z=%d \n",obj->cvt.sign[BMI160_ACC_AXIS_X],obj->cvt.sign[BMI160_ACC_AXIS_Y],obj->cvt.sign[BMI160_ACC_AXIS_Z]);

        //GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[BMI160_ACC_AXIS_X], acc[BMI160_ACC_AXIS_Y], acc[BMI160_ACC_AXIS_Z]);

        sprintf(buf, "%d %d %d", (s16)acc[BMI160_ACC_AXIS_X], (s16)acc[BMI160_ACC_AXIS_Y], (s16)acc[BMI160_ACC_AXIS_Z]);
        if(atomic_read(&obj->trace) & BMA_TRC_IOCTL)
        {
            GSE_LOG("gsensor data for compass: %s!\n", buf);
        }
    }

    return 0;
}

//shihaobin@yulong.com add for sensor calibration begin 20150330
static int yulong_accel_Calibration(struct i2c_client *client, char *buf, int bufsize,int count,unsigned long cal_arg)
//static int yulong_accel_Calibration(struct i2c_client *client, char *buf, int bufsize,int count)
{
    int ret = 0;
    char databuf[6];
    char i;
    int data[3];
    //char tmp[1];
    int cali[3];
    long xavg = 0, yavg = 0, zavg = 0;
    char tempbuf[NAND_FLASH_WR_RD_SIZE];
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;    //shihaobin@yulong.com add for calibration
        //add by wangyufei begin  20140630
    //int ret = 0;
    struct acc_offset acc_cal_data;
    struct acc_offset *acc_cal_ptr = &acc_cal_data;
    //add end
    //struct bmi160_acc_i2c_data *obj = i2c_get_clientdata(client);

    BMI160_ACC_SetPowerMode(client, true);
    msleep(50);
    /*if (true != obj->enable)
    {
        GSE_ERR("ACC ic is not enable\n");
        BMI160_ACC_SetPowerMode(client, true);
        msleep(50);
    }*/

    for (i = 0; i < count; i++)
    {
        if(hwmsen_read_block(client, BMI160_ACC_DATA_REG_L_X, databuf, 6))
        {
            GSE_ERR("bmi160_acc read accscope data  error\n");
            return -2;
        }
        else
        {
            obj->data[BMI160_ACC_AXIS_X] = ((s16)((databuf[BMI160_ACC_AXIS_X*2]) | (databuf[BMI160_ACC_AXIS_X*2+1] << 8)));
            obj->data[BMI160_ACC_AXIS_Y] = ((s16)((databuf[BMI160_ACC_AXIS_Y*2]) | (databuf[BMI160_ACC_AXIS_Y*2+1] << 8)));
            obj->data[BMI160_ACC_AXIS_Z] = ((s16)((databuf[BMI160_ACC_AXIS_Z*2]) | (databuf[BMI160_ACC_AXIS_Z*2+1] << 8)));

            //printk("shihaobin (%d),obj->data[BMI160_ACC_AXIS_X]is %d,obj->data[BMI160_ACC_AXIS_Y] is %d,obj->data[BMI160_ACC_AXIS_Z] is %d\n",__LINE__,obj->data[BMI160_ACC_AXIS_X],obj->data[BMI160_ACC_AXIS_Y],obj->data[BMI160_ACC_AXIS_Z]);
            //printk("(%d),obj->cali_sw[BMI160_ACC_AXIS_X]is %d,obj->cali_sw[BMI160_ACC_AXIS_Y] is %d,obj->cali_sw[BMI160_ACC_AXIS_Z] is %d\n",__LINE__,obj->cali_sw[BMI160_ACC_AXIS_X],obj->cali_sw[BMI160_ACC_AXIS_Y],obj->cali_sw[BMI160_ACC_AXIS_Z]);

            obj->data[BMI160_ACC_AXIS_X] = obj->data[BMI160_ACC_AXIS_X] + obj->cali_sw[BMI160_ACC_AXIS_X];
            obj->data[BMI160_ACC_AXIS_Y] = obj->data[BMI160_ACC_AXIS_Y] + obj->cali_sw[BMI160_ACC_AXIS_Y];
            obj->data[BMI160_ACC_AXIS_Z] = obj->data[BMI160_ACC_AXIS_Z] + obj->cali_sw[BMI160_ACC_AXIS_Z];
            /*remap coordinate*/
            data[BMI160_ACC_AXIS_X] = obj->cvt.sign[BMI160_ACC_AXIS_X]*obj->data[BMI160_ACC_AXIS_X];
            data[BMI160_ACC_AXIS_Y] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*obj->data[BMI160_ACC_AXIS_Y];
            data[BMI160_ACC_AXIS_Z] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*obj->data[BMI160_ACC_AXIS_Z];

            ////Out put the mg
            data[BMI160_ACC_AXIS_X] = data[BMI160_ACC_AXIS_X] * GRAVITY_EARTH_1000 / 16384;
            data[BMI160_ACC_AXIS_Y] = data[BMI160_ACC_AXIS_Y] * GRAVITY_EARTH_1000 / 16384;
            data[BMI160_ACC_AXIS_Z] = data[BMI160_ACC_AXIS_Z] * GRAVITY_EARTH_1000 / 16384;

            printk("yl_sensor_debug (%d),data[BMI160_ACC_AXIS_X]is %d,data[BMI160_ACC_AXIS_Y] is %d,data[BMI160_ACC_AXIS_Z] is %d\n",__LINE__,data[BMI160_ACC_AXIS_X],data[BMI160_ACC_AXIS_Y],data[BMI160_ACC_AXIS_Z]);

            xavg += data[BMI160_ACC_AXIS_X];
            yavg += data[BMI160_ACC_AXIS_Y];
            zavg += data[BMI160_ACC_AXIS_Z];

        }
    }
    //printk("(%d),xavg is %d,Yavg is %d,Zavg is %d\n",__LINE__,xavg,xavg,xavg);
    cali[BMI160_ACC_AXIS_X] = xavg/count;
    cali[BMI160_ACC_AXIS_Y] = yavg/count;
    cali[BMI160_ACC_AXIS_Z] = zavg/count;
    printk("(%d),calix is %d,caliy is %d,caliz is %d\n",__LINE__,cali[BMI160_ACC_AXIS_X],cali[BMI160_ACC_AXIS_Y],cali[BMI160_ACC_AXIS_Z]);
    cali[BMI160_ACC_AXIS_X] = 0-cali[BMI160_ACC_AXIS_X];
    cali[BMI160_ACC_AXIS_Y] = 0-cali[BMI160_ACC_AXIS_Y];
    cali[BMI160_ACC_AXIS_Z] = 9807-cali[BMI160_ACC_AXIS_Z];
    printk("yl_sensor_debug (%d),calix is %d,caliy is %d,caliz is %d\n",__LINE__,cali[BMI160_ACC_AXIS_X],cali[BMI160_ACC_AXIS_Y],cali[BMI160_ACC_AXIS_Z]);

    /*printk("Yulong cali[BMI160_ACC_AXIS_X]=%d, obj->reso->sensitivity=%d,GRAVITY_EARTH_1000=%d\n",cali[BMI160_ACC_AXIS_X], obj->reso->sensitivity, GRAVITY_EARTH_1000);
    printk("Yulong cali[BMI160_ACC_AXIS_Y]=%d, obj->reso->sensitivity=%d,GRAVITY_EARTH_1000=%d\n",cali[BMI160_ACC_AXIS_Y], obj->reso->sensitivity, GRAVITY_EARTH_1000);
    printk("Yulong cali[BMI160_ACC_AXIS_Z]=%d, obj->reso->sensitivity=%d,GRAVITY_EARTH_1000=%d\n",cali[BMI160_ACC_AXIS_Z], obj->reso->sensitivity, GRAVITY_EARTH_1000);*/

    /*cali[BMI160_ACC_AXIS_X] = cali[BMI160_ACC_AXIS_X] * obj->reso->sensitivity / GRAVITY_EARTH_1000;
    cali[BMI160_ACC_AXIS_Y] = cali[BMI160_ACC_AXIS_Y] * obj->reso->sensitivity / GRAVITY_EARTH_1000;
    cali[BMI160_ACC_AXIS_Z] = cali[BMI160_ACC_AXIS_Z] * obj->reso->sensitivity / GRAVITY_EARTH_1000;*/

    ret = BMI160_ACC_WriteCalibration(client, cali);
    //GSE_LOG("fwq GSENSOR_IOCTL_SET_CALI!!xavg =%d,yavg =%d,zavg =%d \n",xavg,yavg,zavg);
    GSE_LOG("fwq GSENSOR_IOCTL_SET_CALI!!obj->reso->sensitivity = %d,cali[BMI160_ACC_AXIS_X] =%d,scali[BMI160_ACC_AXIS_X] =%d,cali[BMI160_ACC_AXIS_X] =%d \n",obj->reso->sensitivity,cali[BMI160_ACC_AXIS_X],cali[BMI160_ACC_AXIS_Y],cali[BMI160_ACC_AXIS_Z]);


    ((struct acc_offset *)acc_cal_ptr)->x = obj->cali_sw[BMI160_ACC_AXIS_X];
    ((struct acc_offset *)acc_cal_ptr)->y = obj->cali_sw[BMI160_ACC_AXIS_Y];
    ((struct acc_offset *)acc_cal_ptr)->z = obj->cali_sw[BMI160_ACC_AXIS_Z];
    ((struct acc_offset *)acc_cal_ptr)->key = ret ? 2 : 1;

    //printk("(%d),obj->cali_sw[BMI160_ACC_AXIS_X] is %d,obj->cali_sw[BMI160_ACC_AXIS_Y] is %d,obj->cali_sw[BMI160_ACC_AXIS_Z] is %d\n",__LINE__,obj->cali_sw[BMI160_ACC_AXIS_X],obj->cali_sw[BMI160_ACC_AXIS_Y],obj->cali_sw[BMI160_ACC_AXIS_Z]);

    printk(KERN_INFO "%s write: x = %d\n",  __func__, ((struct acc_offset *)acc_cal_ptr)->x);
    printk(KERN_INFO "%s write: y = %d\n",  __func__, ((struct acc_offset *)acc_cal_ptr)->y);
    printk(KERN_INFO "%s write: z = %d\n",  __func__,((struct acc_offset *)acc_cal_ptr)->z);
    printk(KERN_INFO "%s write: key = %d\n",  __func__,((struct acc_offset *)acc_cal_ptr)->key);
    memset(tempbuf, 0, sizeof(tempbuf));

    tempbuf[0] = 0x01;
    tempbuf[1] = (obj->cali_sw[BMI160_ACC_AXIS_X] & 0xff00) >> 8;
    tempbuf[2] = obj->cali_sw[BMI160_ACC_AXIS_X] & 0x00ff;
    tempbuf[3] = (obj->cali_sw[BMI160_ACC_AXIS_Y] & 0xff00) >> 8;
    tempbuf[4] = obj->cali_sw[BMI160_ACC_AXIS_Y] & 0x00ff;
    tempbuf[5] = (obj->cali_sw[BMI160_ACC_AXIS_Z] & 0xff00) >> 8;
    tempbuf[6] = obj->cali_sw[BMI160_ACC_AXIS_Z] & 0x00ff;

    if(copy_to_user((struct acc_offset *)cal_arg, acc_cal_ptr, sizeof(acc_cal_data)))
    {
        printk("%s:  Calibrate copy_to_user failed!\n", __func__);
    }
    #if 0
    printk("(%s) TEMPBUF is (%d),(%d),(%d),(%d),(%d),(%d),(%d)\n",
    __LINE__,tempbuf[0],tempbuf[1],tempbuf[2],tempbuf[3],tempbuf[4],tempbuf[5],tempbuf[6] );
    printk("(%s) TEMPBUF is (%x),(%x),(%x),(%x),(%x),(%x),(%x)\n",
        __LINE__,tempbuf[0],tempbuf[1],tempbuf[2],tempbuf[3],tempbuf[4],tempbuf[5],tempbuf[6] );
    #endif

    printk(" wangyufei_write _acc_ps_cal_data  %s\n", __func__);
    if(write_acc_ps_cal_data_to_flash(16, tempbuf, NAND_FLASH_WR_RD_SIZE)<0)
        printk("Create ACC  calibration file error!!");
    else
        printk("Create ACC  calibration file Success!!");

    return ret;

}

static int yulong_acc_ReadCalibration(struct i2c_client *client)
{
    char tempbuf[NAND_FLASH_WR_RD_SIZE];
    //char i;
    struct bmi160_acc_i2c_data *obj = i2c_get_clientdata(client);

    printk("yl_sensor_debug in yulong acc_read_calibration!\n");

    if(read_acc_ps_cal_data_from_flash(16, tempbuf, NAND_FLASH_WR_RD_SIZE)<0)
        printk("ACC use Default CALI , cali_sw[BMI160_ACC_AXIS_X] = %d , cali_sw[BMI160_ACC_AXIS_Y] = %d, cali_sw[BMI160_ACC_AXIS_Z] = %d\n",
        obj->cali_sw[BMI160_ACC_AXIS_X],obj->cali_sw[BMI160_ACC_AXIS_Y],obj->cali_sw[BMI160_ACC_AXIS_Z]);
    else{
        obj->cali_sw[BMI160_ACC_AXIS_X] = (s16)((tempbuf[1]<<8)|tempbuf[2]);
        obj->cali_sw[BMI160_ACC_AXIS_Y] = (s16)((tempbuf[3]<<8)|tempbuf[4]);
        obj->cali_sw[BMI160_ACC_AXIS_Z] = (s16)((tempbuf[5]<<8)|tempbuf[6]);

        printk("ACC use yulong CALI ,tempbuf is %d,%d,%d,%d,%d,%d,%d ",
            tempbuf[0],tempbuf[1],tempbuf[2],tempbuf[3],tempbuf[4],tempbuf[5],tempbuf[6]);

        printk("ACC use yulong CALI , cali_sw[MPU6050C_ACC_AXIS_X] = %d , cali_sw[MPU6050C_ACC_AXIS_Y] = %d, cali_sw[MPU6050C_ACC_AXIS_Z] = %d\n",
            obj->cali_sw[BMI160_ACC_AXIS_X],obj->cali_sw[BMI160_ACC_AXIS_Y],obj->cali_sw[BMI160_ACC_AXIS_Z]);

    }
    return 1;
}
//shihaobin@yulong.com add for sensor calibration end 20150330

/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    //u8 databuf[20];
    int acc[BMI160_ACC_AXES_NUM];
    int res = 0;
    s16 databuf[BMI160_ACC_AXES_NUM];
    static uint8_t bmi160_acc_enable_flag = 0;  //shihaobin add read calibration data 20150330
    //memset(databuf, 0, sizeof(u8)*10);

    if(NULL == buf)
    {
        return -1;
    }
    if(NULL == client)
    {
        *buf = 0;
        return -2;
    }

    //shihaobin add for read calibration data when first enable sensor begin 20150330
    if (0 == bmi160_acc_enable_flag)
    {
        bmi160_acc_enable_flag = 1;
        yulong_acc_ReadCalibration(bmi160_acc_i2c_client);
    }
    //shihaobin add for read calibration data when first enable sensor end 20150330

  if(sensor_power == FALSE)
  {
    res = BMI160_ACC_SetPowerMode(client, true);
    if(res)
    {
      GSE_ERR("Power on bmi160_acc error %d!\n", res);
    }
  }

  res = BMI160_ACC_ReadData(client, databuf);
  if(res)
  {
    GSE_ERR("I2C error: ret value=%d", res);
    return -3;
  }
  else
  {
    //GSE_LOG("raw data x=%d, y=%d, z=%d \n",obj->data[BMI160_ACC_AXIS_X],obj->data[BMI160_ACC_AXIS_Y],obj->data[BMI160_ACC_AXIS_Z]);
        //shihaobin add for debug 20150416 begin
        databuf[BMI160_ACC_AXIS_X] += (obj->cali_sw[BMI160_ACC_AXIS_X]) * 16384 / GRAVITY_EARTH_1000;
        databuf[BMI160_ACC_AXIS_Y] += (obj->cali_sw[BMI160_ACC_AXIS_Y]) * 16384 / GRAVITY_EARTH_1000;
        databuf[BMI160_ACC_AXIS_Z] += (obj->cali_sw[BMI160_ACC_AXIS_Z]) * 16384 / GRAVITY_EARTH_1000;
        //shihaobin add for debug 20150416 end

        /*databuf[BMI160_ACC_AXIS_X] += obj->cali_sw[BMI160_ACC_AXIS_X];
        databuf[BMI160_ACC_AXIS_Y] += obj->cali_sw[BMI160_ACC_AXIS_Y];
        databuf[BMI160_ACC_AXIS_Z] += obj->cali_sw[BMI160_ACC_AXIS_Z];*/

        //GSE_LOG("cali_sw x=%d, y=%d, z=%d \n",obj->cali_sw[BMI160_ACC_AXIS_X],obj->cali_sw[BMI160_ACC_AXIS_Y],obj->cali_sw[BMI160_ACC_AXIS_Z]);

        /*remap coordinate*/
        acc[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]*databuf[BMI160_ACC_AXIS_X];
        acc[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]*databuf[BMI160_ACC_AXIS_Y];
        acc[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]*databuf[BMI160_ACC_AXIS_Z];
        //GSE_LOG("cvt x=%d, y=%d, z=%d \n",obj->cvt.sign[BMI160_ACC_AXIS_X],obj->cvt.sign[BMI160_ACC_AXIS_Y],obj->cvt.sign[BMI160_ACC_AXIS_Z]);

        //GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[BMI160_ACC_AXIS_X], acc[BMI160_ACC_AXIS_Y], acc[BMI160_ACC_AXIS_Z]);

        //Out put the mg
        //GSE_LOG("mg acc=%d, GRAVITY=%d, sensityvity=%d \n",acc[BMI160_ACC_AXIS_X],GRAVITY_EARTH_1000,obj->reso->sensitivity);
        acc[BMI160_ACC_AXIS_X] = acc[BMI160_ACC_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        acc[BMI160_ACC_AXIS_Y] = acc[BMI160_ACC_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        acc[BMI160_ACC_AXIS_Z] = acc[BMI160_ACC_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;

        sprintf(buf, "%04x %04x %04x", acc[BMI160_ACC_AXIS_X], acc[BMI160_ACC_AXIS_Y], acc[BMI160_ACC_AXIS_Z]);
        if(atomic_read(&obj->trace) & BMA_TRC_IOCTL)
        {
            GSE_LOG("gsensor data: %s!\n", buf);
        }
    }

    return 0;
}
#ifdef MISC_FOR_DAEMON
/*----------------------------------------------------------------------------*/
static int BMI160_ACC_ReadRawData(struct i2c_client *client, char *buf)
{
  int res = 0;
  s16 databuf[BMI160_ACC_AXES_NUM];

    if (!buf || !client)
    {
        return EINVAL;
    }

  res = BMI160_ACC_ReadData(client, databuf);
  if(res)
  {
    GSE_ERR("I2C error: ret value=%d", res);
    return EIO;
  }
  else
  {
    sprintf(buf, "BMI160_ACC_ReadRawData %04x %04x %04x", databuf[BMI160_ACC_AXIS_X],
      databuf[BMI160_ACC_AXIS_Y], databuf[BMI160_ACC_AXIS_Z]);
  }

  return 0;
}
#endif
/*----------------------------------------------------------------------------*/
static int bmi160_acc_set_mode(struct i2c_client *client, unsigned char mode)
{
    int comres = 0;
    unsigned char databuf[2] = {0};
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;

    if ((client == NULL) || (mode >= 3))
    {
        return -1;
    }
    mutex_lock(&obj->lock);
    switch (mode) {
    case BMI160_ACC_MODE_NORMAL:
        databuf[0] = CMD_PMU_ACC_NORMAL;
        comres += bma_i2c_write_block(client,
                BMI160_CMD_COMMANDS__REG, &databuf[0], 1);
        break;
    case BMI160_ACC_MODE_SUSPEND:
        databuf[0] = CMD_PMU_ACC_SUSPEND;
        comres += bma_i2c_write_block(client,
                BMI160_CMD_COMMANDS__REG, &databuf[0], 1);
        break;
    default:
        break;
    }

    mdelay(4);

    mutex_unlock(&obj->lock);

    if(comres <= 0)
    {
        return BMI160_ACC_ERR_I2C;
    }
    else
    {
        return comres;
    }
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_get_mode(struct i2c_client *client, unsigned char *mode)
{
    int comres = 0;
    u8 v_data_u8r = C_BMI160_ZERO_U8X;

    if (client == NULL)
    {
        return -1;
    }
    comres = bma_i2c_read_block(client,
            BMI160_USER_ACC_PMU_STATUS__REG, &v_data_u8r, 1);
    *mode = BMI160_GET_BITSLICE(v_data_u8r,
            BMI160_USER_ACC_PMU_STATUS);

    return comres;
}

/*----------------------------------------------------------------------------*/
static int bmi160_acc_set_range(struct i2c_client *client, unsigned char range)
{
    int comres = 0;
    unsigned char data[2] = {BMI160_USER_ACC_RANGE__REG};
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;

    if (client == NULL)
    {
        return -1;
    }
    mutex_lock(&obj->lock);
    comres = bma_i2c_read_block(client,
            BMI160_USER_ACC_RANGE__REG, data+1, 1);

    data[1]  = BMI160_SET_BITSLICE(data[1],
            BMI160_USER_ACC_RANGE, range);

    comres = i2c_master_send(client, data, 2);
    mutex_unlock(&obj->lock);
    if(comres <= 0)
    {
        return BMI160_ACC_ERR_I2C;
    }
    else
    {
        return comres;
    }
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_get_range(struct i2c_client *client, unsigned char *range)
{
    int comres = 0;
    unsigned char data;

    if (client == NULL)
    {
        return -1;
    }

    comres = bma_i2c_read_block(client, BMI160_USER_ACC_RANGE__REG, &data, 1);
    *range = BMI160_GET_BITSLICE(data, BMI160_USER_ACC_RANGE);

    return comres;
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_set_bandwidth(struct i2c_client *client, unsigned char bandwidth)
{
    int comres = 0;
    unsigned char data[2] = {BMI160_USER_ACC_CONF_ODR__REG};
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;

        GSE_LOG("[%s] bandwidth = %d\n", __func__, bandwidth);

    if (client == NULL)
    {
        return -1;
    }

    mutex_lock(&obj->lock);
    comres = bma_i2c_read_block(client,
            BMI160_USER_ACC_CONF_ODR__REG, data+1, 1);

    data[1]  = BMI160_SET_BITSLICE(data[1],
            BMI160_USER_ACC_CONF_ODR, bandwidth);

    comres = i2c_master_send(client, data, 2);
    mdelay(1);
    mutex_unlock(&obj->lock);
    if(comres <= 0)
    {
        return BMI160_ACC_ERR_I2C;
    }
    else
    {
        return comres;
    }
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_get_bandwidth(struct i2c_client *client, unsigned char *bandwidth)
{
    int comres = 0;

    if (client == NULL)
    {
        return -1;
    }

    comres = bma_i2c_read_block(client, BMI160_USER_ACC_CONF_ODR__REG, bandwidth, 1);
    *bandwidth = BMI160_GET_BITSLICE(*bandwidth, BMI160_USER_ACC_CONF_ODR);

    return comres;
}

/*----------------------------------------------------------------------------*/
#ifdef MISC_FOR_DAEMON
//tad3sgh add++
// Daemon application save the data
static int ECS_SaveData(int buf[CALIBRATION_DATA_SIZE])
{
#if DEBUG
    struct bmi160_acc_i2c_data *data = obj_i2c_data;
#endif

    mutex_lock(&sensor_data_mutex);
    switch (buf[0])
    {
    case 2: /* SENSOR_HANDLE_MAGNETIC_FIELD */
        memcpy(sensor_data+4, buf+1, 4*sizeof(int));
        break;
    case 3: /* SENSOR_HANDLE_ORIENTATION */
        memcpy(sensor_data+8, buf+1, 4*sizeof(int));
        break;
#ifdef BMC050_M4G
    case 4: /* SENSOR_HANDLE_GYROSCOPE */
        memcpy(m4g_data, buf+1, 4*sizeof(int));
        break;
#endif //BMC050_M4G
  default:
    break;
  }
  mutex_unlock(&sensor_data_mutex);

#if DEBUG
  if(atomic_read(&data->trace) & BMA_TRC_INFO)
  {
    GSE_LOG("Get daemon data: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d!\n",
      sensor_data[0],sensor_data[1],sensor_data[2],sensor_data[3],
      sensor_data[4],sensor_data[5],sensor_data[6],sensor_data[7],
      sensor_data[8],sensor_data[9],sensor_data[10],sensor_data[11]);
#if defined(BMC050_M4G)
    GSE_LOG("Get m4g data: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d!\n",
      m4g_data[0],m4g_data[1],m4g_data[2],m4g_data[3],
      m4g_data[4],m4g_data[5],m4g_data[6],m4g_data[7],
      m4g_data[8],m4g_data[9],m4g_data[10],m4g_data[11]);
#endif //BMC050_M4G
  }
#endif

    return 0;
}
#endif

//tad3sgh add--
/*----------------------------------------------------------------------------*/

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = bmi160_acc_i2c_client;
    char strbuf[BMI160_BUFSIZE];
    if(NULL == client)
    {
        GSE_ERR("i2c client is null!!\n");
        return 0;
    }

  BMI160_ACC_ReadChipInfo(client, strbuf, BMI160_BUFSIZE);
  return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
/*
g sensor opmode for compass tilt compensation
*/
static ssize_t show_cpsopmode_value(struct device_driver *ddri, char *buf)
{
    unsigned char data;

    if (bmi160_acc_get_mode(bmi160_acc_i2c_client, &data) < 0)
    {
        return sprintf(buf, "Read error\n");
    }
    else
    {
        return sprintf(buf, "%d\n", data);
    }
}

/*----------------------------------------------------------------------------*/
/*
g sensor opmode for compass tilt compensation
*/
static ssize_t store_cpsopmode_value(struct device_driver *ddri, const char *buf, size_t count)
{
    unsigned long data;
    int error;

  error = strict_strtoul(buf, 10, &data);
  if (error)
  {
    return error;
  }
  if (data == BMI160_ACC_MODE_NORMAL)
  {
    BMI160_ACC_SetPowerMode(bmi160_acc_i2c_client, true);
  }
  else if (data == BMI160_ACC_MODE_SUSPEND)
  {
    BMI160_ACC_SetPowerMode(bmi160_acc_i2c_client, false);
  }
  else if (bmi160_acc_set_mode(bmi160_acc_i2c_client, (unsigned char) data) < 0)
  {
    GSE_ERR("invalid content: '%s', length = %lu\n", buf, count);
  }

    return count;
}

/*----------------------------------------------------------------------------*/
/*
g sensor range for compass tilt compensation
*/
static ssize_t show_cpsrange_value(struct device_driver *ddri, char *buf)
{
    unsigned char data;

    if (bmi160_acc_get_range(bmi160_acc_i2c_client, &data) < 0)
    {
        return sprintf(buf, "Read error\n");
    }
    else
    {
        return sprintf(buf, "%d\n", data);
    }
}

/*----------------------------------------------------------------------------*/
/*
g sensor range for compass tilt compensation
*/
static ssize_t store_cpsrange_value(struct device_driver *ddri, const char *buf, size_t count)
{
    unsigned long data;
    int error;

  error = strict_strtoul(buf, 10, &data);
  if (error)
  {
        return error;
    }
    if (bmi160_acc_set_range(bmi160_acc_i2c_client, (unsigned char) data) < 0)
    {
        GSE_ERR("invalid content: '%s', length = %lu\n", buf, count);
    }

    return count;
}
/*----------------------------------------------------------------------------*/
/*
g sensor bandwidth for compass tilt compensation
*/
static ssize_t show_cpsbandwidth_value(struct device_driver *ddri, char *buf)
{
    unsigned char data;
    //shihaobin@yulong.com add for read chip filter value begin 20150417
    static int beg = 0x40;
    int filter_value = 0;
    int i2c_rea_err = 0;
    
     struct i2c_msg filter_msgs[2] = {
        {
            .addr = obj_i2c_data->client->addr,   .flags = 0,
            .len = 1,               .buf = &beg
        },
        {
            .addr = obj_i2c_data->client->addr,   .flags = I2C_M_RD,
            .len = 1,               .buf = &filter_value
        }
        };

    if (bmi160_acc_get_bandwidth(bmi160_acc_i2c_client, &data) < 0)
    {
        return sprintf(buf, "Read error\n");
    }
    else
    {
        i2c_rea_err = i2c_transfer(obj_i2c_data->client->adapter, filter_msgs, sizeof(filter_msgs)/sizeof(filter_msgs[0]));
        
        return sprintf(buf, "%d %d\n", data, filter_value);
    }
	//shihaobin@yulong.com add for read chip filter value end 20150417
}

/*----------------------------------------------------------------------------*/
/*
g sensor bandwidth for compass tilt compensation
*/
static ssize_t store_cpsbandwidth_value(struct device_driver *ddri, const char *buf, size_t count)
{
    unsigned long data;
    int error;

  error = strict_strtoul(buf, 10, &data);
  if (error)
  {
    return error;
  }
  if (bmi160_acc_set_bandwidth(bmi160_acc_i2c_client, (unsigned char) data) < 0)
  {
    GSE_ERR("invalid content: '%s', length = %lu\n", buf, count);
  }

    return count;
}

/*----------------------------------------------------------------------------*/
/*
g sensor data for compass tilt compensation
*/
static ssize_t show_cpsdata_value(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = bmi160_acc_i2c_client;
    char strbuf[BMI160_BUFSIZE];

  if(NULL == client)
  {
    GSE_ERR("i2c client is null!!\n");
    return 0;
  }
  BMI160_ACC_CompassReadData(client, strbuf, BMI160_BUFSIZE);
  return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = bmi160_acc_i2c_client;
    char strbuf[BMI160_BUFSIZE];

  if(NULL == client)
  {
    GSE_ERR("i2c client is null!!\n");
    return 0;
  }
  BMI160_ACC_ReadSensorData(client, strbuf, BMI160_BUFSIZE);
  //BMI160_ACC_ReadRawData(client, strbuf);
  return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = bmi160_acc_i2c_client;
    struct bmi160_acc_i2c_data *obj;
    int err, len = 0, mul;
    int tmp[BMI160_ACC_AXES_NUM];

    if(NULL == client)
    {
        GSE_ERR("i2c client is null!!\n");
        return 0;
    }

    obj = obj_i2c_data;

  err = BMI160_ACC_ReadOffset(client, obj->offset);
  if(err)
  {
    return -EINVAL;
  }

  err = BMI160_ACC_ReadCalibration(client, tmp);
  if(err) {
    return -EINVAL;
  }
  else
  {
    mul = obj->reso->sensitivity/bmi160_acc_offset_resolution.sensitivity;
    len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,
      obj->offset[BMI160_ACC_AXIS_X], obj->offset[BMI160_ACC_AXIS_Y], obj->offset[BMI160_ACC_AXIS_Z],
      obj->offset[BMI160_ACC_AXIS_X], obj->offset[BMI160_ACC_AXIS_Y], obj->offset[BMI160_ACC_AXIS_Z]);
    len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
      obj->cali_sw[BMI160_ACC_AXIS_X], obj->cali_sw[BMI160_ACC_AXIS_Y], obj->cali_sw[BMI160_ACC_AXIS_Z]);

        len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
            obj->offset[BMI160_ACC_AXIS_X]*mul + obj->cali_sw[BMI160_ACC_AXIS_X],
            obj->offset[BMI160_ACC_AXIS_Y]*mul + obj->cali_sw[BMI160_ACC_AXIS_Y],
            obj->offset[BMI160_ACC_AXIS_Z]*mul + obj->cali_sw[BMI160_ACC_AXIS_Z],
            tmp[BMI160_ACC_AXIS_X], tmp[BMI160_ACC_AXIS_Y], tmp[BMI160_ACC_AXIS_Z]);

        return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
    struct i2c_client *client = bmi160_acc_i2c_client;
    int err, x, y, z;
    int dat[BMI160_ACC_AXES_NUM];

  if(!strncmp(buf, "rst", 3))
  {
    err = BMI160_ACC_ResetCalibration(client);
    if(err) {
      GSE_ERR("reset offset err = %d\n", err);
    }
  }
  else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
  {
    dat[BMI160_ACC_AXIS_X] = x;
    dat[BMI160_ACC_AXIS_Y] = y;
    dat[BMI160_ACC_AXIS_Z] = z;
    err = BMI160_ACC_WriteCalibration(client, dat);
    if(err)
    {
      GSE_ERR("write calibration err = %d\n", err);
    }
  }
  else
  {
    GSE_ERR("invalid format\n");
  }

    return count;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMI160_ACC_LOWPASS
    struct i2c_client *client = bmi160_acc_i2c_client;
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    if(atomic_read(&obj->firlen))
    {
        int idx, len = atomic_read(&obj->firlen);
        GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

        for(idx = 0; idx < len; idx++)
        {
            GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][BMI160_ACC_AXIS_X], obj->fir.raw[idx][BMI160_ACC_AXIS_Y], obj->fir.raw[idx][BMI160_ACC_AXIS_Z]);
        }

        GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[BMI160_ACC_AXIS_X], obj->fir.sum[BMI160_ACC_AXIS_Y], obj->fir.sum[BMI160_ACC_AXIS_Z]);
        GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[BMI160_ACC_AXIS_X]/len, obj->fir.sum[BMI160_ACC_AXIS_Y]/len, obj->fir.sum[BMI160_ACC_AXIS_Z]/len);
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
    return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_BMI160_ACC_LOWPASS
    struct i2c_client *client = bmi160_acc_i2c_client;
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int firlen;

    if(1 != sscanf(buf, "%d", &firlen))
    {
        GSE_ERR("invallid format\n");
    }
    else if(firlen > C_MAX_FIR_LENGTH)
    {
        GSE_ERR("exceeds maximum filter length\n");
    }
    else
    {
        atomic_set(&obj->firlen, firlen);
        if(NULL == firlen)
        {
            atomic_set(&obj->fir_en, 0);
        }
        else
        {
            memset(&obj->fir, 0x00, sizeof(obj->fir));
            atomic_set(&obj->fir_en, 1);
        }
    }
#endif
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
    ssize_t res;
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    if (obj == NULL)
    {
        GSE_ERR("i2c_data obj is null!!\n");
        return 0;
    }

    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
    return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int trace;
    if (obj == NULL)
    {
        GSE_ERR("i2c_data obj is null!!\n");
        return 0;
    }

    if(1 == sscanf(buf, "0x%x", &trace))
    {
        atomic_set(&obj->trace, trace);
    }
    else
    {
        GSE_ERR("invalid content: '%s', length = %lu\n", buf, count);
    }

    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    if (obj == NULL)
    {
        GSE_ERR("i2c_data obj is null!!\n");
        return 0;
    }

    if(obj->hw)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n",
                obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);
    }
    else
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
    }
    return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{
    if(sensor_power)
        GSE_LOG("G sensor is in work mode, sensor_power = %d\n", sensor_power);
    else
        GSE_LOG("G sensor is in standby mode, sensor_power = %d\n", sensor_power);

    return 0;
}

/*----------------------------------------------------------------------------*/
static void bmi_fifo_frame_bytes_extend_calc(
  struct bmi160_acc_i2c_data *client_data,
  unsigned int *fifo_frmbytes_extend)
{
  switch (client_data->fifo_data_sel) {
  case BMI_FIFO_A_SEL:
  case BMI_FIFO_G_SEL:
    *fifo_frmbytes_extend = 7;
    break;
  case BMI_FIFO_G_A_SEL:
    *fifo_frmbytes_extend = 13;
    break;
  case BMI_FIFO_M_SEL:
    *fifo_frmbytes_extend = 9;
    break;
  case BMI_FIFO_M_A_SEL:
  case BMI_FIFO_M_G_SEL:
    /*8(mag) + 6(gyro or acc) +1(head) = 15*/
    *fifo_frmbytes_extend = 15;
    break;
  case BMI_FIFO_M_G_A_SEL:
    /*8(mag) + 6(gyro or acc) + 6 + 1 = 21*/
    *fifo_frmbytes_extend = 21;
    break;
  default:
    *fifo_frmbytes_extend = 0;
    break;
  };
}

static int bmi_fifo_analysis_handle(struct bmi160_acc_i2c_data *client_data,
        u8 *fifo_data, u16 fifo_length, char *buf)
{
  u8 frame_head = 0;/* every frame head*/
  int len = 0;
  u8 acc_frm_cnt = 0;/*0~146*/
  u8 gyro_frm_cnt = 0;
  u64 fifo_time = 0;
  static u32 current_frm_ts;
  u16 fifo_index = 0;/* fifo data buff index*/
  u16 i = 0;
  s8 last_return_st = 0;
  int err = 0;
  unsigned int frame_bytes = 0;
  struct bmi160acc_t acc_frame_arr[FIFO_FRAME_CNT], acc_tmp;
  struct bmi160gyro_t gyro_frame_arr[FIFO_FRAME_CNT], gyro_tmp;
  struct bmi160_acc_i2c_data *obj = obj_i2c_data;

  struct odr_t odr;

  memset(&odr, 0, sizeof(odr));
  for (i = 0; i < FIFO_FRAME_CNT; i++) {
    memset(&acc_frame_arr[i], 0, sizeof(struct bmi160acc_t));
    memset(&gyro_frame_arr[i], 0, sizeof(struct bmi160gyro_t));
  }

  /* no fifo select for bmi sensor*/
  if (!client_data->fifo_data_sel) {
    GSE_ERR("No select any sensor FIFO for BMI16x\n");
    return -EINVAL;
  }

  /*driver need read acc_odr/gyro_odr/mag_odr*/
  if ((client_data->fifo_data_sel) & (1 << BMI_ACC_SENSOR))
    odr.acc_odr = client_data->odr.acc_odr;
  if ((client_data->fifo_data_sel) & (1 << BMI_GYRO_SENSOR))
    odr.gyro_odr = client_data->odr.gyro_odr;
  if ((client_data->fifo_data_sel) & (1 << BMI_MAG_SENSOR))
    odr.mag_odr = client_data->odr.mag_odr;
  bmi_fifo_frame_bytes_extend_calc(client_data, &frame_bytes);
  /* search sensor time sub function firstly */
  for (fifo_index = 0; fifo_index < fifo_length;) {

    frame_head = fifo_data[fifo_index];

    switch (frame_head) {
      /*skip frame 0x40 22 0x84*/
      case FIFO_HEAD_SKIP_FRAME:
        /*fifo data frame index + 1*/
        fifo_index = fifo_index + 1;
        if (fifo_index + 1 > fifo_length) {
          last_return_st = FIFO_SKIP_OVER_LEN;
          break;
        }
        /*skip_frame_cnt = fifo_data[fifo_index];*/
        fifo_index = fifo_index + 1;
        break;

        /*M & G & A*/
      case FIFO_HEAD_M_G_A:
        {/*fifo data frame index + 1*/
          fifo_index = fifo_index + 1;
          if (fifo_index + MGA_BYTES_FRM > fifo_length) {
            last_return_st = FIFO_M_G_A_OVER_LEN;
            break;
          }

          fifo_index = fifo_index + MGA_BYTES_FRM;
          break;
        }

      case FIFO_HEAD_M_A:
        {/*fifo data frame index + 1*/
          fifo_index = fifo_index + 1;
          if (fifo_index + MA_BYTES_FRM > fifo_length) {
            last_return_st = FIFO_M_A_OVER_LEN;
            break;
          }

          fifo_index = fifo_index + MA_BYTES_FRM;
          break;
        }

      case FIFO_HEAD_M_G:
        {/*fifo data frame index + 1*/
          fifo_index = fifo_index + 1;
          if (fifo_index + MG_BYTES_FRM > fifo_length) {
            last_return_st = FIFO_M_G_OVER_LEN;
            break;
          }

          fifo_index = fifo_index + MG_BYTES_FRM;
          break;
        }

      case FIFO_HEAD_G_A:
        { /*fifo data frame index + 1*/
          fifo_index = fifo_index + 1;
          if (fifo_index + GA_BYTES_FRM > fifo_length) {
            last_return_st = FIFO_G_A_OVER_LEN;
            break;
          }

          gyro_tmp.x = fifo_data[fifo_index + 1] << 8 | fifo_data[fifo_index + 0];
          gyro_tmp.y = fifo_data[fifo_index + 3] << 8 | fifo_data[fifo_index + 2];
          gyro_tmp.z = fifo_data[fifo_index + 5] << 8 | fifo_data[fifo_index + 4];

          acc_tmp.x = fifo_data[fifo_index + 7] << 8 | fifo_data[fifo_index + 6];
          acc_tmp.y = fifo_data[fifo_index + 9] << 8 | fifo_data[fifo_index + 8];
          acc_tmp.z = fifo_data[fifo_index + 11] << 8 | fifo_data[fifo_index + 10];

          gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_X]] =
            obj->cvt.sign[BMI160_ACC_AXIS_X]*gyro_tmp.v[BMI160_ACC_AXIS_X];
          gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
            obj->cvt.sign[BMI160_ACC_AXIS_Y]*gyro_tmp.v[BMI160_ACC_AXIS_Y];
          gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
            obj->cvt.sign[BMI160_ACC_AXIS_Z]*gyro_tmp.v[BMI160_ACC_AXIS_Z];

          acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_X]] =
            obj->cvt.sign[BMI160_ACC_AXIS_X]*acc_tmp.v[BMI160_ACC_AXIS_X];
          acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
            obj->cvt.sign[BMI160_ACC_AXIS_Y]*acc_tmp.v[BMI160_ACC_AXIS_Y];
          acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
            obj->cvt.sign[BMI160_ACC_AXIS_Z]*acc_tmp.v[BMI160_ACC_AXIS_Z];

          gyro_frm_cnt++;
          acc_frm_cnt++;
          fifo_index = fifo_index + GA_BYTES_FRM;

          break;
        }
      case FIFO_HEAD_A:
        { /*fifo data frame index + 1*/
          fifo_index = fifo_index + 1;
          if (fifo_index + A_BYTES_FRM > fifo_length) {
            last_return_st = FIFO_A_OVER_LEN;
            break;
          }

          acc_tmp.x = fifo_data[fifo_index + 1] << 8 | fifo_data[fifo_index + 0];
          acc_tmp.y = fifo_data[fifo_index + 3] << 8 | fifo_data[fifo_index + 2];
          acc_tmp.z = fifo_data[fifo_index + 5] << 8 | fifo_data[fifo_index + 4];

          acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_X]] =
            obj->cvt.sign[BMI160_ACC_AXIS_X]*acc_tmp.v[BMI160_ACC_AXIS_X];
          acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
            obj->cvt.sign[BMI160_ACC_AXIS_Y]*acc_tmp.v[BMI160_ACC_AXIS_Y];
          acc_frame_arr[acc_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
            obj->cvt.sign[BMI160_ACC_AXIS_Z]*acc_tmp.v[BMI160_ACC_AXIS_Z];

          acc_frm_cnt++;/*acc_frm_cnt*/
          fifo_index = fifo_index + A_BYTES_FRM;
          break;
        }
      case FIFO_HEAD_G:
        { /*fifo data frame index + 1*/
          fifo_index = fifo_index + 1;
          if (fifo_index + G_BYTES_FRM > fifo_length) {
            last_return_st = FIFO_G_OVER_LEN;
            break;
          }

          gyro_tmp.x = fifo_data[fifo_index + 1] << 8 | fifo_data[fifo_index + 0];
          gyro_tmp.y = fifo_data[fifo_index + 3] << 8 | fifo_data[fifo_index + 2];
          gyro_tmp.z = fifo_data[fifo_index + 5] << 8 | fifo_data[fifo_index + 4];

          gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_X]] =
            obj->cvt.sign[BMI160_ACC_AXIS_X]*gyro_tmp.v[BMI160_ACC_AXIS_X];
          gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
            obj->cvt.sign[BMI160_ACC_AXIS_Y]*gyro_tmp.v[BMI160_ACC_AXIS_Y];
          gyro_frame_arr[gyro_frm_cnt].v[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
            obj->cvt.sign[BMI160_ACC_AXIS_Z]*gyro_tmp.v[BMI160_ACC_AXIS_Z];

          gyro_frm_cnt++;/*gyro_frm_cnt*/

          fifo_index = fifo_index + G_BYTES_FRM;
          break;
        }
      case FIFO_HEAD_M:
        { /*fifo data frame index + 1*/
          fifo_index = fifo_index + 1;
          if (fifo_index + A_BYTES_FRM > fifo_length) {
            last_return_st = FIFO_M_OVER_LEN;
            break;
          }

          fifo_index = fifo_index + M_BYTES_FRM;
          break;
        }

        /* sensor time frame*/
      case FIFO_HEAD_SENSOR_TIME:
        {
          /*fifo data frame index + 1*/
          fifo_index = fifo_index + 1;

          if (fifo_index + 3 > fifo_length) {
            last_return_st = FIFO_SENSORTIME_RETURN;
            break;
          }
          fifo_time =
            fifo_data[fifo_index + 2] << 16 |
            fifo_data[fifo_index + 1] << 8 |
            fifo_data[fifo_index + 0];

          client_data->fifo_time = fifo_time;
          /*fifo sensor time frame index + 3*/
          fifo_index = fifo_index + 3;
          break;
        }
      case FIFO_HEAD_OVER_READ_LSB:
        /*fifo data frame index + 1*/
        fifo_index = fifo_index + 1;

        if (fifo_index + 1 > fifo_length) {
          last_return_st = FIFO_OVER_READ_RETURN;
          break;
        }
        if (fifo_data[fifo_index] ==
            FIFO_HEAD_OVER_READ_MSB) {
          /*fifo over read frame index + 1*/
          fifo_index = fifo_index + 1;
          break;
        } else {
          last_return_st = FIFO_OVER_READ_RETURN;
          break;
        }

      default:
        last_return_st = 1;
        break;

    }
    if (last_return_st)
      break;
  }
  fifo_time = 0;

  /*Acc Only*/
  if (client_data->fifo_data_sel == BMI_FIFO_A_SEL) {
    for (i = 0; i < acc_frm_cnt; i++) {
      /*current_frm_ts += 256;*/
      current_frm_ts +=
        sensortime_duration_tbl[odr.acc_odr].ts_duration_us*LMADA;

      len = sprintf(buf, "%s %d %d %d %d ",
          ACC_FIFO_HEAD,
          acc_frame_arr[i].x,
          acc_frame_arr[i].y,
          acc_frame_arr[i].z,
          current_frm_ts);
      buf += len;
      err += len;
    }
  }


  /*only for G*/
  if (client_data->fifo_data_sel == BMI_FIFO_G_SEL) {
    for (i = 0; i < gyro_frm_cnt; i++) {
      /*current_frm_ts += 256;*/
      current_frm_ts +=
        sensortime_duration_tbl[odr.gyro_odr].ts_duration_us*LMADA;

      len = sprintf(buf, "%s %d %d %d %d ",
          GYRO_FIFO_HEAD,
          gyro_frame_arr[i].x,
          gyro_frame_arr[i].y,
          gyro_frame_arr[i].z,
          current_frm_ts
             );
      buf += len;
      err += len;
    }
  }

  /*only for A G*/
  if (client_data->fifo_data_sel == BMI_FIFO_G_A_SEL) {

    GSE_LOG("gyro_frm_cnt %d\n", gyro_frm_cnt);

    for (i = 0; i < gyro_frm_cnt; i++) {
      /*sensor timeLSB*/
      /*dia(sensor_time) = fifo_time & (0xff), uint:LSB, 39.0625us*/
      /*AP tinmestamp 390625/10000 = 625 /16 */
      current_frm_ts +=
        sensortime_duration_tbl[odr.gyro_odr].ts_duration_us*LMADA;

      len = sprintf(buf,
          "%s %d %d %d %d %s %d %d %d %d ",
          GYRO_FIFO_HEAD,
          gyro_frame_arr[i].x,
          gyro_frame_arr[i].y,
          gyro_frame_arr[i].z,
          current_frm_ts,
          ACC_FIFO_HEAD,
          acc_frame_arr[i].x,
          acc_frame_arr[i].y,
          acc_frame_arr[i].z,
          current_frm_ts
             );

      buf += len;
      err += len;
    }

  }

  return err;
}

static int bmi160_fifo_length(uint32_t *fifo_length)
{
        int comres=0;
  struct i2c_client *client = bmi160_acc_i2c_client;
        uint8_t a_data_u8r[2] = {0, 0};

  comres += bma_i2c_read_block(client, BMI160_USER_FIFO_BYTE_COUNTER_LSB__REG, a_data_u8r, 2);
  a_data_u8r[1] = BMI160_GET_BITSLICE(a_data_u8r[1], BMI160_USER_FIFO_BYTE_COUNTER_MSB);
  *fifo_length = (uint32_t)(((uint32_t)((uint8_t)(a_data_u8r[1])<<BMI160_SHIFT_8_POSITION)) | a_data_u8r[0]);

        return comres;
}

int bmi160_set_command_register(u8 cmd_reg)
{
        int comres=0;
  struct i2c_client *client = bmi160_acc_i2c_client;

  comres += bma_i2c_write_block(client, BMI160_CMD_COMMANDS__REG, &cmd_reg, 1);

        return comres;
}

static ssize_t bmi160_fifo_bytecount_show(struct device_driver *ddri, char *buf)
{
        int comres=0;
        uint32_t fifo_bytecount = 0;
        uint8_t a_data_u8r[2] = {0, 0};
  struct i2c_client *client = bmi160_acc_i2c_client;

  comres += bma_i2c_read_block(client, BMI160_USER_FIFO_BYTE_COUNTER_LSB__REG, a_data_u8r, 2);
  a_data_u8r[1] = BMI160_GET_BITSLICE(a_data_u8r[1], BMI160_USER_FIFO_BYTE_COUNTER_MSB);
  fifo_bytecount = (uint32_t)(((uint32_t)((uint8_t)(a_data_u8r[1])<<BMI160_SHIFT_8_POSITION)) | a_data_u8r[0]);

        comres = sprintf(buf, "%u\n", fifo_bytecount);
        return comres;
}

static ssize_t bmi160_fifo_bytecount_store(struct device_driver *ddri, const char *buf, size_t count)
{
        struct bmi160_acc_i2c_data *client_data = obj_i2c_data;
        int err;
        unsigned long data;
        err = kstrtoul(buf, 10, &data);
        if (err)
                return err;
        client_data->fifo_bytecount = (unsigned int) data;

        return count;
}

static int bmi160_fifo_data_sel_get(struct bmi160_acc_i2c_data *client_data)
{
        int err = 0;
  struct i2c_client *client = bmi160_acc_i2c_client;
  unsigned char data;
        unsigned char fifo_acc_en, fifo_gyro_en, fifo_mag_en;
        unsigned char fifo_datasel;


  err = bma_i2c_read_block(client, BMI160_USER_FIFO_ACC_EN__REG, &data, 1);
  fifo_acc_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_ACC_EN);

  err += bma_i2c_read_block(client, BMI160_USER_FIFO_GYRO_EN__REG, &data, 1);
  fifo_gyro_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_GYRO_EN);

  err += bma_i2c_read_block(client, BMI160_USER_FIFO_MAG_EN__REG, &data, 1);
  fifo_mag_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_MAG_EN);

        if (err)
                return err;

        fifo_datasel = (fifo_acc_en << BMI_ACC_SENSOR) |
                        (fifo_gyro_en << BMI_GYRO_SENSOR) |
                                (fifo_mag_en << BMI_MAG_SENSOR);

  client_data->fifo_data_sel = fifo_datasel;

        return err;
}

static ssize_t bmi160_fifo_data_sel_show(struct device_driver *ddri, char *buf)
{
        int err = 0;
        struct bmi160_acc_i2c_data *client_data = obj_i2c_data;
        err = bmi160_fifo_data_sel_get(client_data);
        if (err)
                return -EINVAL;
        return sprintf(buf, "%d\n", client_data->fifo_data_sel);
}

static ssize_t bmi160_fifo_data_sel_store(struct device_driver *ddri, const char *buf, size_t count)
{
        struct bmi160_acc_i2c_data *client_data = obj_i2c_data;
  struct i2c_client *client = bmi160_acc_i2c_client;
        int err;
        unsigned long data;
        unsigned char fifo_datasel;
        unsigned char fifo_acc_en, fifo_gyro_en, fifo_mag_en;

        err = kstrtoul(buf, 10, &data);
        if (err)
                return err;
        /* data format: aimed 0b0000 0x(m)x(g)x(a), x:1 enable, 0:disable*/
        if (data > 7)
                return -EINVAL;


        fifo_datasel = (unsigned char)data;
  fifo_acc_en = fifo_datasel & (1 << BMI_ACC_SENSOR) ? 1 : 0;
  fifo_gyro_en = fifo_datasel & (1 << BMI_GYRO_SENSOR) ? 1 : 0;
  fifo_mag_en = fifo_datasel & (1 << BMI_MAG_SENSOR) ? 1 : 0;

  err += bma_i2c_read_block(client, BMI160_USER_FIFO_ACC_EN__REG, &fifo_datasel, 1);
  fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_ACC_EN, fifo_acc_en);
  err += bma_i2c_write_block(client, BMI160_USER_FIFO_ACC_EN__REG, &fifo_datasel, 1);

  err += bma_i2c_read_block(client, BMI160_USER_FIFO_GYRO_EN__REG, &fifo_datasel, 1);
  fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_GYRO_EN, fifo_gyro_en);
  err += bma_i2c_write_block(client, BMI160_USER_FIFO_GYRO_EN__REG, &fifo_datasel, 1);

  err += bma_i2c_read_block(client, BMI160_USER_FIFO_MAG_EN__REG, &fifo_datasel, 1);
  fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_MAG_EN, fifo_mag_en);
  err += bma_i2c_write_block(client, BMI160_USER_FIFO_MAG_EN__REG, &fifo_datasel, 1);

        if (err)
                return -EIO;

  client_data->fifo_data_sel = (unsigned char)data;
  GSE_LOG("FIFO fifo_data_sel %d, A_en:%d, G_en:%d, M_en:%d\n",
      client_data->fifo_data_sel, fifo_acc_en, fifo_gyro_en, fifo_mag_en);

        return count;
}

static ssize_t bmi160_fifo_data_out_frame_show(struct device_driver *ddri, char *buf)
{
  struct i2c_client *client = bmi160_acc_i2c_client;
  struct bmi160_acc_i2c_data *client_data = obj_i2c_data;
  int err = 0;
  unsigned int fifo_bytecount_tmp;
  if (NULL == g_fifo_data_arr) {
    GSE_ERR("no memory available in fifo_data_frame\n");
    return -ENOMEM;
  }

  if (!client_data->fifo_data_sel)
    return sprintf(buf, "no selsect sensor fifo, fifo_data_sel:%d\n",
        client_data->fifo_data_sel);

  if (client_data->fifo_bytecount == 0)
    return -EINVAL;

  //g_current_apts_us = get_current_timestamp();

  bmi160_fifo_length(&fifo_bytecount_tmp);
  if (fifo_bytecount_tmp > client_data->fifo_bytecount)
    client_data->fifo_bytecount = fifo_bytecount_tmp;
  if (client_data->fifo_bytecount > 210) {
    err += bmi160_set_command_register(CMD_CLR_FIFO_DATA);
    client_data->fifo_bytecount = 210;
  }
  if (!err) {
    memset(g_fifo_data_arr, 0, 2048);
#ifdef FIFO_READ_USE_DMA_MODE_I2C
    err = i2c_dma_read_fifo(client, BMI160_USER_FIFO_DATA__REG,
        g_fifo_data_arr, client_data->fifo_bytecount);
#else
    err = bma_i2c_read_block(client, BMI160_USER_FIFO_DATA__REG,
        g_fifo_data_arr, client_data->fifo_bytecount);
#endif
  } else
    GSE_ERR("read fifo leght err");
  if (err) {
    GSE_ERR("brust read fifo err\n");
    return err;
  }

#define isprint(a) ((a >=' ')&&(a <= '~'))
  if (0) {
    int len = client_data->fifo_bytecount;
    const char *ptr = g_fifo_data_arr;
    int i, j;

    for (i = 0; i < len; i += 16) {
      printk(KERN_INFO "%.8x:", i);
      for (j = 0; j < 16; j++) {
        if (!(j % 4))
          printk(" ");
        printk("%.2x", ptr[i + j]);
      }
      printk(" ");
      for (j = 0; j < 16; j++)
        printk("%c", isprint(ptr[i + j]) ? ptr[i + j] : '.');
      printk("\n");
    }
  }

  err = bmi_fifo_analysis_handle(client_data, g_fifo_data_arr,
      client_data->fifo_bytecount, buf);

  return err;
}

static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
  struct bmi160_acc_i2c_data *data = obj_i2c_data;

  return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
    data->hw->direction,atomic_read(&data->layout), data->cvt.sign[0], data->cvt.sign[1],
    data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
  struct bmi160_acc_i2c_data *data = obj_i2c_data;
  int layout = 0;

  if(1 == sscanf(buf, "%d", &layout))
  {
    atomic_set(&data->layout, layout);
    if(!hwmsen_get_convert(layout, &data->cvt))
    {
      GSE_ERR( "HWMSEN_GET_CONVERT function error!\r\n");
    }
    else if(!hwmsen_get_convert(data->hw->direction, &data->cvt))
    {
      GSE_ERR( "invalid layout: %d, restore to %d\n", layout, data->hw->direction);
    }
    else
    {
      GSE_ERR( "invalid layout: (%d, %d)\n", layout, data->hw->direction);
      hwmsen_get_convert(0, &data->cvt);
    }
  }
  else
  {
    GSE_ERR( "invalid format = '%s'\n", buf);
  }

  return count;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo,   S_IWUSR | S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(cpsdata,      S_IWUSR | S_IRUGO, show_cpsdata_value,    NULL);
static DRIVER_ATTR(cpsopmode,  S_IWUSR | S_IRUGO, show_cpsopmode_value,    store_cpsopmode_value);
static DRIVER_ATTR(cpsrange,     S_IWUSR | S_IRUGO, show_cpsrange_value,     store_cpsrange_value);
static DRIVER_ATTR(cpsbandwidth, S_IWUSR | S_IRUGO, show_cpsbandwidth_value,    store_cpsbandwidth_value);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
static DRIVER_ATTR(powerstatus,               S_IRUGO, show_power_status_value,        NULL);

static DRIVER_ATTR(fifo_bytecount, S_IRUGO | S_IWUSR, bmi160_fifo_bytecount_show, bmi160_fifo_bytecount_store);
static DRIVER_ATTR(fifo_data_sel, S_IRUGO | S_IWUSR, bmi160_fifo_data_sel_show, bmi160_fifo_data_sel_store);
static DRIVER_ATTR(fifo_data_frame, S_IRUGO, bmi160_fifo_data_out_frame_show, NULL);
static DRIVER_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value );
/*----------------------------------------------------------------------------*/
static struct driver_attribute *bmi160_acc_attr_list[] = {
  &driver_attr_chipinfo,     /*chip information*/
  &driver_attr_sensordata,   /*dump sensor data*/
  &driver_attr_cali,         /*show calibration data*/
  &driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
  &driver_attr_trace,        /*trace log*/
  &driver_attr_status,
  &driver_attr_powerstatus,
  &driver_attr_cpsdata, /*g sensor data for compass tilt compensation*/
  &driver_attr_cpsopmode, /*g sensor opmode for compass tilt compensation*/
  &driver_attr_cpsrange,  /*g sensor range for compass tilt compensation*/
  &driver_attr_cpsbandwidth,  /*g sensor bandwidth for compass tilt compensation*/

  &driver_attr_fifo_bytecount,
  &driver_attr_fifo_data_sel,
  &driver_attr_fifo_data_frame,
  &driver_attr_layout,
};
/*----------------------------------------------------------------------------*/
static int bmi160_acc_create_attr(struct device_driver *driver)
{
    int idx, err = 0;
    int num = (int)(sizeof(bmi160_acc_attr_list)/sizeof(bmi160_acc_attr_list[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }

  for(idx = 0; idx < num; idx++)
  {
    err = driver_create_file(driver, bmi160_acc_attr_list[idx]);
    if(err) {
      GSE_ERR("driver_create_file (%s) = %d\n", bmi160_acc_attr_list[idx]->attr.name, err);
      break;
    }
  }
  return err;
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(bmi160_acc_attr_list)/sizeof(bmi160_acc_attr_list[0]));

    if(driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, bmi160_acc_attr_list[idx]);
    }

    return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void* self, uint32_t command, void* buff_in, int size_in,
    void* buff_out, int size_out, int* actualout)
{
  int err = 0;
  int value, sample_delay;
  struct bmi160_acc_i2c_data *priv = (struct bmi160_acc_i2c_data*)self;
  hwm_sensor_data* gsensor_data;
  char buff[BMI160_BUFSIZE];

  //GSE_FUN(f);
  switch (command)
  {
    case SENSOR_DELAY:
      if((buff_in == NULL) || (size_in < sizeof(int)))
      {
        GSE_ERR("Set delay parameter error!\n");
        err = -EINVAL;
      }
      else
      {
        value = *(int *)buff_in;
        if(value <= 5)
        {
          sample_delay = BMI160_ACCEL_ODR_400HZ;
        }
        else if(value <= 10)
        {
          sample_delay = BMI160_ACCEL_ODR_200HZ;
        }
        else
        {
          sample_delay = BMI160_ACCEL_ODR_100HZ;
        }

        //err = BMI160_ACC_SetBWRate(priv->client, sample_delay);
        if(err != BMI160_ACC_SUCCESS ) //0x2C->BW=100Hz
        {
          GSE_ERR("Set delay parameter error!\n");
        }

        if(value >= 50)
        {
          atomic_set(&priv->filter, 0);
        }
        else
        {
        #if defined(CONFIG_BMI160_ACC_LOWPASS)
          priv->fir.num = 0;
          priv->fir.idx = 0;
          priv->fir.sum[BMI160_ACC_AXIS_X] = 0;
          priv->fir.sum[BMI160_ACC_AXIS_Y] = 0;
          priv->fir.sum[BMI160_ACC_AXIS_Z] = 0;
          atomic_set(&priv->filter, 1);
        #endif
        }
      }
      break;

    case SENSOR_ENABLE:
      if((buff_in == NULL) || (size_in < sizeof(int)))
      {
        GSE_ERR("Enable sensor parameter error!\n");
        err = -EINVAL;
      }
      else
      {
        value = *(int *)buff_in;
        if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
        {
          GSE_LOG("Gsensor device have updated!\n");
        }
        else
        {
          err = BMI160_ACC_SetPowerMode( priv->client, !sensor_power);
        }
      }
      break;

    case SENSOR_GET_DATA:
      if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
      {
        GSE_ERR("get sensor data parameter error!\n");
        err = -EINVAL;
      }
      else
      {
        gsensor_data = (hwm_sensor_data *)buff_out;
        BMI160_ACC_ReadSensorData(priv->client, buff, BMI160_BUFSIZE);
        sscanf(buff, "%x %x %x", &gsensor_data->values[0],
          &gsensor_data->values[1], &gsensor_data->values[2]);
        gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
        gsensor_data->value_divide = 1000;
      }
      break;
    default:
      GSE_ERR("gsensor operate function no this parameter %d!\n", command);
      err = -1;
      break;
  }

  return err;
}

#ifdef MISC_FOR_DAEMON
/******************************************************************************
 * Function Configuration
******************************************************************************/
static int bmi160_acc_open(struct inode *inode, struct file *file)
{
    file->private_data = bmi160_acc_i2c_client;

    if(file->private_data == NULL)
    {
        GSE_ERR("null pointer!!\n");
        return -EINVAL;
    }
    return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}
/*----------------------------------------------------------------------------*/
static long bmi160_acc_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client*)file->private_data;
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    char strbuf[BMI160_BUFSIZE];
    void __user *data;
    SENSOR_DATA sensor_data;
    long err = 0;
    int cali[3];
    //tad3sgh add ++
    int status;                 /* for OPEN/CLOSE_STATUS */
    short sensor_status;        /* for Orientation and Msensor status */
    int value[CALIBRATION_DATA_SIZE];           /* for SET_YPR */
    //tad3sgh add --
    //GSE_FUN(f);
    if(_IOC_DIR(cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
    {
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if(err)
    {
        GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
    }

    switch(cmd)
    {
        //shihaobin add for do acc-sensor calibrate begin 20150330
        case ACC_CALIBRATE:

            err = BMI160_ACC_ResetCalibration(client);

            yulong_accel_Calibration(client, strbuf, BMI160_BUFSIZE , 20, arg);
            //yulong_accel_Calibration(client, strbuf, BMI160_BUFSIZE , 20);

            break;
        //shihaobin add for do acc-sensor calibrate end 20150330

        case GSENSOR_IOCTL_INIT:
            bmi160_acc_init_client(client, 0);
            break;

        case GSENSOR_IOCTL_READ_CHIPINFO:
            data = (void __user *) arg;
            if(data == NULL)
            {
                err = -EINVAL;
                break;
            }

      BMI160_ACC_ReadChipInfo(client, strbuf, BMI160_BUFSIZE);
      if(copy_to_user(data, strbuf, strlen(strbuf)+1))
      {
        err = -EFAULT;
        break;
      }
      break;

        case GSENSOR_IOCTL_READ_SENSORDATA:

            data = (void __user *) arg;
            if(data == NULL)
            {
                err = -EINVAL;
                break;
            }

      BMI160_ACC_ReadSensorData(client, strbuf, BMI160_BUFSIZE);
      if(copy_to_user(data, strbuf, strlen(strbuf)+1))
      {
        err = -EFAULT;
        break;
      }
      break;

        case GSENSOR_IOCTL_READ_GAIN:
            data = (void __user *) arg;
            if(data == NULL)
            {
                err = -EINVAL;
                break;
            }

            if(copy_to_user(data, &gsensor_gain, sizeof(GSENSOR_VECTOR3D)))
            {
                err = -EFAULT;
                break;
            }
            break;

    case GSENSOR_IOCTL_READ_RAW_DATA:
      data = (void __user *) arg;
      if(data == NULL)
      {
        err = -EINVAL;
        break;
      }
      BMI160_ACC_ReadRawData(client, strbuf);
      if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
      {
        err = -EFAULT;
        break;
      }
      break;

        case GSENSOR_IOCTL_SET_CALI:
            data = (void __user*)arg;
            if(data == NULL)
            {
                err = -EINVAL;
                break;
            }
            if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
            {
                err = -EFAULT;
                break;
            }
            if(atomic_read(&obj->suspend))
            {
                GSE_ERR("Perform calibration in suspend state!!\n");
                err = -EINVAL;
            }
            else
            {
                cali[BMI160_ACC_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
                cali[BMI160_ACC_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
                cali[BMI160_ACC_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;
                err = BMI160_ACC_WriteCalibration(client, cali);
            }
            break;

        case GSENSOR_IOCTL_CLR_CALI:
            err = BMI160_ACC_ResetCalibration(client);
            break;

    case GSENSOR_IOCTL_GET_CALI:
      data = (void __user*)arg;
      if(data == NULL)
      {
        err = -EINVAL;
        break;
      }
      err = BMI160_ACC_ReadCalibration(client, cali);
      if(err) {
        break;
      }

            sensor_data.x = cali[BMI160_ACC_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
            sensor_data.y = cali[BMI160_ACC_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
            sensor_data.z = cali[BMI160_ACC_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
            if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
            {
                err = -EFAULT;
                break;
            }
            break;
        //tad3sgh add ++
        case BMM_IOC_GET_EVENT_FLAG:    // used by daemon only
            data = (void __user *) arg;
            /* block if no event updated */
            wait_event_interruptible(uplink_event_flag_wq, (uplink_event_flag != 0));
            mutex_lock(&uplink_event_flag_mutex);
            status = uplink_event_flag;
            mutex_unlock(&uplink_event_flag_mutex);
            if(copy_to_user(data, &status, sizeof(status)))
            {
                GSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            break;

        case BMM_IOC_GET_NONBLOCK_EVENT_FLAG:   // used by daemon only
            data = (void __user *) arg;
            /* nonblock daemon process */
            //wait_event_interruptible(uplink_event_flag_wq, (uplink_event_flag != 0));
            mutex_lock(&uplink_event_flag_mutex);
            status = uplink_event_flag;
            mutex_unlock(&uplink_event_flag_mutex);
            if(copy_to_user(data, &status, sizeof(status)))
            {
                GSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            break;

        case ECOMPASS_IOC_GET_DELAY:            //used by daemon
            data = (void __user *) arg;
            if(copy_to_user(data, &bmm050d_delay, sizeof(bmm050d_delay)))
            {
                GSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            /* clear the flag */
            mutex_lock(&uplink_event_flag_mutex);
            if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_M_DELAY) != 0)
            {
                uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_M_DELAY;
            }
            else if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_O_DELAY) != 0)
            {
                uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_O_DELAY;
            }
            mutex_unlock(&uplink_event_flag_mutex);
            /* wake up the wait queue */
            wake_up(&uplink_event_flag_wq);
            break;

        case ECOMPASS_IOC_SET_YPR:              //used by daemon
            data = (void __user *) arg;
            if(data == NULL)
            {
                GSE_ERR("invalid argument.");
                return -EINVAL;
            }
            if(copy_from_user(value, data, sizeof(value)))
            {
                GSE_ERR("copy_from_user failed.");
                return -EFAULT;
            }
            ECS_SaveData(value);
            break;

        case ECOMPASS_IOC_GET_MFLAG:        //used by daemon
            data = (void __user *) arg;
            sensor_status = atomic_read(&m_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
            if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
            {
                /* de-active m-channel when driver suspend regardless of m_flag*/
                sensor_status = 0;
            }
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
            if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
            {
                GSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            /* clear the flag */
            mutex_lock(&uplink_event_flag_mutex);
            if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_M_ACTIVE) != 0)
            {
                uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_M_ACTIVE;
            }
            mutex_unlock(&uplink_event_flag_mutex);
            /* wake up the wait queue */
            wake_up(&uplink_event_flag_wq);
            break;

        case ECOMPASS_IOC_GET_OFLAG:        //used by daemon
            data = (void __user *) arg;
            sensor_status = atomic_read(&o_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
            if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
            {
                /* de-active m-channel when driver suspend regardless of m_flag*/
                sensor_status = 0;
            }
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
            if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
            {
                GSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            /* clear the flag */
            mutex_lock(&uplink_event_flag_mutex);
            if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_O_ACTIVE) != 0)
            {
                uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_O_ACTIVE;
            }
            mutex_unlock(&uplink_event_flag_mutex);
            /* wake up the wait queue */
            wake_up(&uplink_event_flag_wq);
            break;

#ifdef BMC050_M4G
        case ECOMPASS_IOC_GET_GDELAY:           //used by daemon
            data = (void __user *) arg;
            if(copy_to_user(data, &m4g_delay, sizeof(m4g_delay)))
            {
                GSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            /* clear the flag */
            mutex_lock(&uplink_event_flag_mutex);
            if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_G_DELAY) != 0)
            {
                uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_G_DELAY;
            }
            mutex_unlock(&uplink_event_flag_mutex);
            /* wake up the wait queue */
            wake_up(&uplink_event_flag_wq);
            break;

        case ECOMPASS_IOC_GET_GFLAG:        //used by daemon
            data = (void __user *) arg;
            sensor_status = atomic_read(&g_flag);
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
            if ((sensor_status == 1) && (atomic_read(&driver_suspend_flag) == 1))
            {
                /* de-active g-channel when driver suspend regardless of g_flag*/
                sensor_status = 0;
            }
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
            if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
            {
                GSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            /* clear the flag */
            mutex_lock(&uplink_event_flag_mutex);
            if ((uplink_event_flag & BMMDRV_ULEVT_FLAG_G_ACTIVE) != 0)
            {
                uplink_event_flag &= ~BMMDRV_ULEVT_FLAG_G_ACTIVE;
            }
            mutex_unlock(&uplink_event_flag_mutex);
            /* wake up the wait queue */
            wake_up(&uplink_event_flag_wq);
            break;
#endif //BMC050_M4G
    //tad3sgh add --
    default:
      GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
      err = -ENOIOCTLCMD;
      break;

    }

    return err;
}

#ifdef CONFIG_COMPAT
static long bmi160_acc_compat_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
    long err = 0;

    void __user *arg32 = compat_ptr(arg);

    if (!file->f_op || !file->f_op->unlocked_ioctl)
        return -ENOTTY;

    switch (cmd)
    {
        case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
            if (arg32 == NULL)
            {
                err = -EINVAL;
                break;
            }

            err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_READ_SENSORDATA, (unsigned long)arg32);
            if (err){
                GSE_ERR("GSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed.");
                return err;
            }
            break;

        case COMPAT_GSENSOR_IOCTL_SET_CALI:
            if (arg32 == NULL)
            {
                err = -EINVAL;
                break;
            }

            err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_SET_CALI, (unsigned long)arg32);
            if (err){
                GSE_ERR("GSENSOR_IOCTL_SET_CALI unlocked_ioctl failed.");
                return err;
            }
            break;

        case COMPAT_GSENSOR_IOCTL_GET_CALI:
            if (arg32 == NULL)
            {
                err = -EINVAL;
                break;
            }

            err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_GET_CALI, (unsigned long)arg32);
            if (err){
                GSE_ERR("GSENSOR_IOCTL_GET_CALI unlocked_ioctl failed.");
                return err;
            }
            break;

        case COMPAT_GSENSOR_IOCTL_CLR_CALI:
            if (arg32 == NULL)
            {
                err = -EINVAL;
                break;
            }

            err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg32);
            if (err){
                GSE_ERR("GSENSOR_IOCTL_CLR_CALI unlocked_ioctl failed.");
                return err;
            }
            break;

        default:
            GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
            err = -ENOIOCTLCMD;
        break;

    }

    return err;
}
#endif

/*----------------------------------------------------------------------------*/
static struct file_operations bmi160_acc_fops = {
    //.owner = THIS_MODULE,
    .open = bmi160_acc_open,
    .release = bmi160_acc_release,
    .unlocked_ioctl = bmi160_acc_unlocked_ioctl,
    #ifdef CONFIG_COMPAT
        .compat_ioctl = bmi160_acc_compat_ioctl,
    #endif
};
/*----------------------------------------------------------------------------*/
static struct miscdevice bmi160_acc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "gsensor",
    .fops = &bmi160_acc_fops,
};
#endif
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int bmi160_acc_suspend(struct i2c_client *client, pm_message_t msg)
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int err = 0;

    GSE_FUN();

    if(msg.event == PM_EVENT_SUSPEND)
    {
        if(obj == NULL)
        {
            GSE_ERR("null pointer!!\n");
            return -EINVAL;
        }
        atomic_set(&obj->suspend, 1);
        //tad3sgh add ++
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
        /* set driver suspend flag */
        atomic_set(&driver_suspend_flag, 1);
        if (atomic_read(&m_flag) == 1)
        {
            /* set the flag to block e-compass daemon*/
            mutex_lock(&uplink_event_flag_mutex);
            uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
            mutex_unlock(&uplink_event_flag_mutex);
        }
        if (atomic_read(&o_flag) == 1)
        {
            /* set the flag to block e-compass daemon*/
            mutex_lock(&uplink_event_flag_mutex);
            uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
            mutex_unlock(&uplink_event_flag_mutex);
        }
#ifdef BMC050_M4G
        if (atomic_read(&g_flag) == 1)
        {
            /* set the flag to block e-compass daemon*/
            mutex_lock(&uplink_event_flag_mutex);
            uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
            mutex_unlock(&uplink_event_flag_mutex);
        }
#endif //BMC050_M4G
    /* wake up the wait queue */
    wake_up(&uplink_event_flag_wq);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND

//tad3sgh add --
    err = BMI160_ACC_SetPowerMode(obj->client, false);
    if(err) {
      GSE_ERR("write power control fail!!\n");
      return err;
    }
    BMI160_ACC_power(obj->hw, 0);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
static int bmi160_acc_resume(struct i2c_client *client)
{
    struct bmi160_acc_i2c_data *obj = obj_i2c_data;
    int err;

    GSE_FUN();

    if(obj == NULL)
    {
        GSE_ERR("null pointer!!\n");
        return -EINVAL;
    }

  BMI160_ACC_power(obj->hw, 1);
  err = bmi160_acc_init_client(client, 0);
  if(err) {
    GSE_ERR("initialize client fail!!\n");
    return err;
  }
  //tad3sgh add ++
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
    /* clear driver suspend flag */
    atomic_set(&driver_suspend_flag, 0);
    if (atomic_read(&m_flag) == 1)
    {
        /* set the flag to unblock e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
    if (atomic_read(&o_flag) == 1)
    {
        /* set the flag to unblock e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
#ifdef BMC050_M4G
    if (atomic_read(&g_flag) == 1)
    {
        /* set the flag to unblock e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
#endif //BMC050_M4G
  /* wake up the wait queue */
  wake_up(&uplink_event_flag_wq);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
//tad3sgh add --

    atomic_set(&obj->suspend, 0);

    return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void bmi160_acc_early_suspend(struct early_suspend *h)
{
    struct bmi160_acc_i2c_data *obj = container_of(h, struct bmi160_acc_i2c_data, early_drv);
    int err;

    GSE_FUN();

    if(obj == NULL)
    {
        GSE_ERR("null pointer!!\n");
        return;
    }
    atomic_set(&obj->suspend, 1);
//tad3sgh add ++
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
    /* set driver suspend flag */
    atomic_set(&driver_suspend_flag, 1);
    if (atomic_read(&m_flag) == 1)
    {
        /* set the flag to block e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
    if (atomic_read(&o_flag) == 1)
    {
        /* set the flag to block e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
#ifdef BMC050_M4G
    if (atomic_read(&g_flag) == 1)
    {
        /* set the flag to block e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
#endif //BMC050_M4G
  /* wake up the wait queue */
  wake_up(&uplink_event_flag_wq);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND

//tad3sgh add --
  err = BMI160_ACC_SetPowerMode(obj->client, false);
  if(err) {
    GSE_ERR("write power control fail!!\n");
    return;
  }

  BMI160_ACC_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void bmi160_acc_late_resume(struct early_suspend *h)
{
    struct bmi160_acc_i2c_data *obj = container_of(h, struct bmi160_acc_i2c_data, early_drv);
    int err;

    GSE_FUN();

    if(obj == NULL)
    {
        GSE_ERR("null pointer!!\n");
        return;
    }

  BMI160_ACC_power(obj->hw, 1);
  err = bmi160_acc_init_client(obj->client, 0);
  if(err) {
    GSE_ERR("initialize client fail!!\n");
    return;
  }
//tad3sgh add ++
#ifdef BMC050_BLOCK_DAEMON_ON_SUSPEND
    /* clear driver suspend flag */
    atomic_set(&driver_suspend_flag, 0);
    if (atomic_read(&m_flag) == 1)
    {
        /* set the flag to unblock e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
    if (atomic_read(&o_flag) == 1)
    {
        /* set the flag to unblock e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
#ifdef BMC050_M4G
    if (atomic_read(&g_flag) == 1)
    {
        /* set the flag to unblock e-compass daemon*/
        mutex_lock(&uplink_event_flag_mutex);
        uplink_event_flag |= BMMDRV_ULEVT_FLAG_G_ACTIVE;
        mutex_unlock(&uplink_event_flag_mutex);
    }
#endif //BMC050_M4G
  /* wake up the wait queue */
  wake_up(&uplink_event_flag_wq);
#endif //BMC050_BLOCK_DAEMON_ON_SUSPEND
//tad3sgh add --
    atomic_set(&obj->suspend, 0);
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/

/*----------------------------------------------------------------------------*/
static struct i2c_driver bmi160_acc_i2c_driver = {
    .driver = {
        .name           = BMI160_DEV_NAME,
    },
  .probe          = bmi160_acc_i2c_probe,
  .remove         = bmi160_acc_i2c_remove,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
    .suspend            = bmi160_acc_suspend,
    .resume             = bmi160_acc_resume,
#endif
  .id_table = bmi160_acc_i2c_id,
};

// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int bmi160_acc_open_report_data(int open)
{
    //should queuq work to report event if  is_report_input_direct=true
    return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL

static int bmi160_acc_enable_nodata(int en)
{
#ifdef MISC_FOR_DAEMON
  int err = 0;

  if(((en == 0) && (sensor_power == false))
      ||((en == 1) && (sensor_power == true))) {
    GSE_LOG("Gsensor device have updated!\n");
  } else {
    err = BMI160_ACC_SetPowerMode(obj_i2c_data->client, !sensor_power);
  }

  return err;
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
    res = BMI160_ACC_SetPowerMode(obj_i2c_data->client, power);
    if(res == 0)
    {
      GSE_LOG("BMI160_ACC_SetPowerMode done\n");
      break;
    }
    GSE_LOG("BMI160_ACC_SetPowerMode fail\n");
  }

  if(res != BMI160_ACC_SUCCESS)
  {
    printk("BMI160_ACC_SetPowerMode fail!\n");
    return -1;
  }
  printk("bmi160_acc_enable_nodata OK!\n");
  return 0;
#endif
}

static int bmi160_acc_set_delay(u64 ns)
{
#ifdef MISC_FOR_DAEMON
  int err = 0;
  int value, sample_delay;

  value = (int)ns/1000/1000;
  if(value <= 5) {
    sample_delay = BMI160_ACCEL_ODR_400HZ;
  } else if(value <= 10) {
    sample_delay = BMI160_ACCEL_ODR_200HZ;
  } else {
    sample_delay = BMI160_ACCEL_ODR_100HZ;
  }

  //err = BMI160_ACC_SetBWRate(obj_i2c_data->client, sample_delay);
  if(err != BMI160_ACC_SUCCESS ) {
    GSE_ERR("Set delay parameter error!\n");
  }

  if(value >= 50) {
    atomic_set(&obj_i2c_data->filter, 0);
  } else {
#if defined(CONFIG_BMI160_ACC_LOWPASS)
    obj_i2c_data->fir.num = 0;
    obj_i2c_data->fir.idx = 0;
    obj_i2c_data->fir.sum[BMI160_ACC_AXIS_X] = 0;
    obj_i2c_data->fir.sum[BMI160_ACC_AXIS_Y] = 0;
    obj_i2c_data->fir.sum[BMI160_ACC_AXIS_Z] = 0;
    atomic_set(&obj_i2c_data->filter, 1);
#endif
  }

  return 0;
#else
  int value =0;
  int sample_delay=0;
  int err=0;
  value = (int)ns/1000/1000;
  if(value <= 5)
  {
    sample_delay = BMI160_ACCEL_ODR_400HZ;
  }
  else if(value <= 10)
  {
    sample_delay = BMI160_ACCEL_ODR_200HZ;
  }
  else
  {
    sample_delay = BMI160_ACCEL_ODR_100HZ;
  }

  err = BMI160_ACC_SetBWRate(obj_i2c_data->client, sample_delay);
  if(err != BMI160_ACC_SUCCESS ) //0x2C->BW=100Hz
  {
    GSE_ERR("bmi160_acc_set_delay Set delay parameter error!\n");
    return -1;
  }
  GSE_LOG("bmi160_acc_set_delay (%d)\n",value);
  return 0;
#endif
}

static int bmi160_acc_get_data(int* x ,int* y,int* z, int* status)
{
  char buff[BMI160_BUFSIZE];
  /* use acc raw data for gsensor */
  BMI160_ACC_ReadSensorData(obj_i2c_data->client, buff, BMI160_BUFSIZE);

  sscanf(buff, "%x %x %x", x, y, z);
  *status = SENSOR_STATUS_ACCURACY_MEDIUM;

  return 0;
}
int bmi160_m_enable(int en)
{
  if(en == 1) {
    atomic_set(&m_flag, 1);
  } else {
    atomic_set(&m_flag, 0);
  }

  /* set the flag */
  mutex_lock(&uplink_event_flag_mutex);
  uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_ACTIVE;
  mutex_unlock(&uplink_event_flag_mutex);
  /* wake up the wait queue */
  wake_up(&uplink_event_flag_wq);

  return 0;
}

int bmi160_m_set_delay(u64 ns)
{
  int value = (int)ns/1000/1000;

  bmm050d_delay = value;
  /* set the flag */
  mutex_lock(&uplink_event_flag_mutex);
  uplink_event_flag |= BMMDRV_ULEVT_FLAG_M_DELAY;
  mutex_unlock(&uplink_event_flag_mutex);
  /* wake up the wait queue */
  wake_up(&uplink_event_flag_wq);

  return 0;
}

int bmi160_m_open_report_data(int open)
{
  return 0;
}

int bmi160_m_get_data(int* x ,int* y,int* z, int* status)
{
  mutex_lock(&sensor_data_mutex);

  *x = sensor_data[4];
  *y = sensor_data[5];
  *z = sensor_data[6];
  *status = sensor_data[7];

  mutex_unlock(&sensor_data_mutex);

  return 0;
}

int bmi160_o_enable(int en)
{
  if(en == 1) {
    atomic_set(&o_flag, 1);
  } else {
    atomic_set(&o_flag, 0);
  }

  /* set the flag */
  mutex_lock(&uplink_event_flag_mutex);
  uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_ACTIVE;
  mutex_unlock(&uplink_event_flag_mutex);
  /* wake up the wait queue */
  wake_up(&uplink_event_flag_wq);

  return 0;
}

int bmi160_o_set_delay(u64 ns)
{
  int value = (int)ns/1000/1000;

  bmm050d_delay = value;
  /* set the flag */
  mutex_lock(&uplink_event_flag_mutex);
  uplink_event_flag |= BMMDRV_ULEVT_FLAG_O_DELAY;
  mutex_unlock(&uplink_event_flag_mutex);
  /* wake up the wait queue */
  wake_up(&uplink_event_flag_wq);

  return 0;
}

int bmi160_o_open_report_data(int open)
{
  return 0;
}

int bmi160_o_get_data(int* x ,int* y,int* z, int* status)
{
  mutex_lock(&sensor_data_mutex);

  *x = sensor_data[8];
  *y = sensor_data[9];
  *z = sensor_data[10];
  *status = sensor_data[11];

  mutex_unlock(&sensor_data_mutex);

  return 0;
}
//shihaobin@yulong.com add for compatible of gsensor 20150414 end
/*----------------------------------------------------------------------------*/
static int bmi160_acc_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_client *new_client;
    struct bmi160_acc_i2c_data *obj;
    //shihaobin add for compatible of gsensor 20150414 begin
    //struct hwmsen_object sobj;
    struct acc_control_path ctl={0};
    struct acc_data_path data={0};
    //shihaobin add for compatible of gsensor 20150414 end
    char strbuf[BMI160_BUFSIZE]; //shihaobin add for debug

    //tad3sgh add ++
//tad3sgh add --
    int err = 0;
    GSE_FUN();

    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }

    memset(obj, 0, sizeof(struct bmi160_acc_i2c_data));

    obj->hw = get_cust_acc_hw();

  err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
  if(err) {
    GSE_ERR("invalid direction: %d\n", obj->hw->direction);
    goto exit;
  }

    obj_i2c_data = obj;
    obj->client = client;
    new_client = obj->client;
    i2c_set_clientdata(new_client,obj);

    atomic_set(&obj->trace, 0);
    atomic_set(&obj->suspend, 0);
    mutex_init(&obj->lock);

    //tad3sgh add ++
    mutex_init(&sensor_data_mutex);
    mutex_init(&uplink_event_flag_mutex);

    init_waitqueue_head(&uplink_event_flag_wq);
    //tad3sgh add --

#ifdef CONFIG_BMI160_ACC_LOWPASS
    if(obj->hw->firlen > C_MAX_FIR_LENGTH)
    {
        atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
    }
    else
    {
        atomic_set(&obj->firlen, obj->hw->firlen);
    }

    if(atomic_read(&obj->firlen) > 0)
    {
        atomic_set(&obj->fir_en, 1);
    }

#endif

  bmi160_acc_i2c_client = new_client;

  err = bmi160_acc_init_client(new_client, 1);
  if(err) {
    goto exit_init_failed;
  }

#ifdef MISC_FOR_DAEMON
  err = misc_register(&bmi160_acc_device);
  if(err) {
    GSE_ERR("bmi160_acc_device register failed\n");
    goto exit_misc_device_register_failed;
  }
#endif

  err = bmi160_acc_create_attr(&(bmi160_acc_init_info.platform_diver_addr->driver));
  if(err) {
    GSE_ERR("create attribute err = %d\n", err);
    goto exit_create_attr_failed;
  }

#ifdef FIFO_READ_USE_DMA_MODE_I2C
  I2CDMABuf_va = dma_alloc_coherent(NULL, 1024, &I2CDMABuf_pa, GFP_KERNEL);
  if(!I2CDMABuf_va) {
    GSE_ERR("Allocate DMA I2C Buffer failed!\n");
    goto exit_create_attr_failed;
  }
#endif

  ctl.open_report_data= bmi160_acc_open_report_data;
  ctl.enable_nodata = bmi160_acc_enable_nodata;
  ctl.set_delay  = bmi160_acc_set_delay;
  ctl.is_report_input_direct = false;

  err = acc_register_control_path(&ctl);
  if(err)
  {
    GSE_ERR("register acc control path err\n");
    goto exit_kfree;
  }

  data.get_data = bmi160_acc_get_data;
  data.vender_div = 1000;
  err = acc_register_data_path(&data);
  if(err)
  {
    GSE_ERR("register acc data path err\n");
    goto exit_kfree;
  }

//tad3sgh add --
#ifdef CONFIG_HAS_EARLYSUSPEND
    obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    obj->early_drv.suspend  = bmi160_acc_early_suspend,
    obj->early_drv.resume   = bmi160_acc_late_resume,
    register_early_suspend(&obj->early_drv);
#endif

    //shihaobin add for debug
    //yulong_accel_Calibration(new_client, strbuf, BMI160_BUFSIZE , 20);
    bmi160_acc_init_flag = 0;
    printk("yl_sensor_debug %s OK!\n", __func__);
    GSE_LOG("%s: OK\n", __func__);
    return 0;

  exit_create_attr_failed:
#ifdef MISC_FOR_DAEMON
  misc_deregister(&bmi160_acc_device);
  exit_misc_device_register_failed:
#endif
  exit_init_failed:
  exit_kfree:
  kfree(obj);
  exit:
#ifdef FIFO_READ_USE_DMA_MODE_I2C
  if(I2CDMABuf_va) {
    dma_free_coherent(NULL, 1024, I2CDMABuf_va, I2CDMABuf_pa);
    I2CDMABuf_va = NULL;
    I2CDMABuf_pa = 0;
  }
#endif
  GSE_ERR("%s: err = %d\n", __func__, err);
  bmi160_acc_init_flag =-1;
  return err;
}

/*----------------------------------------------------------------------------*/
static int bmi160_acc_i2c_remove(struct i2c_client *client)
{
    int err = 0;

  err = bmi160_acc_delete_attr(&(bmi160_acc_init_info.platform_diver_addr->driver));
  if(err) {
    GSE_ERR("bma150_delete_attr fail: %d\n", err);
  }

#ifdef MISC_FOR_DAEMON
  err = misc_deregister(&bmi160_acc_device);
  if(err) {
    GSE_ERR("misc_deregister fail: %d\n", err);
  }
#endif

#ifdef FIFO_READ_USE_DMA_MODE_I2C
  if(I2CDMABuf_va) {
    dma_free_coherent(NULL, 1024, I2CDMABuf_va, I2CDMABuf_pa);
    I2CDMABuf_va = NULL;
    I2CDMABuf_pa = 0;
  }
#endif

    bmi160_acc_i2c_client = NULL;
    i2c_unregister_device(client);
    kfree(obj_i2c_data);
    return 0;
}

static int  bmi160_acc_local_init(void)
{
    struct acc_hw *hw = get_cust_acc_hw();
  GSE_LOG("fwq loccal init+++\n");

  BMI160_ACC_power(hw, 1);
  if(i2c_add_driver(&bmi160_acc_i2c_driver))
  {
    GSE_ERR("add driver error\n");
    return -1;
  }
  if(-1 == bmi160_acc_init_flag)
  {
    return -1;
  }
  GSE_LOG("fwq loccal init---\n");
  return 0;
}

static int bmi160_acc_remove(void)
{
  struct acc_hw *hw = get_cust_acc_hw();

  GSE_FUN();
  BMI160_ACC_power(hw, 0);
  i2c_del_driver(&bmi160_acc_i2c_driver);
  return 0;
}

static struct acc_init_info bmi160_acc_init_info = {
  .name = "bmi160_acc",
  .init = bmi160_acc_local_init,
  .uninit = bmi160_acc_remove,
};

/*----------------------------------------------------------------------------*/
static int __init bmi160_acc_init(void)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();
    i2c_register_board_info(hw->i2c_num, &bmi160_acc_i2c_info, 1);
    //shihaobin@yulong.com modify for compatible of gsensor 20150414 begin
    /*if(platform_driver_register(&bmi160_acc_gsensor_driver))
    {
        GSE_ERR("failed to register driver");
        return -ENODEV;
    }*/
    acc_driver_add(&bmi160_acc_init_info);
    //shihaobin@yulong.com modify for compatible of gsensor 20150414 end
    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit bmi160_acc_exit(void)
{
    GSE_FUN();
    //shihaobin@yulong.com delete for compatible of gsensor 20150414
    //platform_driver_unregister(&bmi160_acc_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(bmi160_acc_init);
module_exit(bmi160_acc_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BMI160_ACC I2C driver");
MODULE_AUTHOR("hongji.zhou@bosch-sensortec.com");
