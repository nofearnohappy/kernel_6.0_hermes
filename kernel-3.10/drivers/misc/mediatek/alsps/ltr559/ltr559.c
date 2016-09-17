/*
 *
 * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
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

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include <linux/hwmsen_helper.h>
#include "ltr559.h"

#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <linux/sched.h>
#include <alsps.h>
#include "linux/mutex.h"
#undef CUSTOM_KERNEL_SENSORHUB
#ifdef CUSTOM_KERNEL_SENSORHUB
#include <SCP_sensorHub.h>
#endif
#include <linux/jiffies.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE
#define GN_MTK_BSP_PS_DYNAMIC_CALI
#define NEAR_DELAY_TIME 	((500 * HZ) / 1000)
#define TH_ADD			100


#define LTR559_DEV_NAME   "LTR_559ALS"

#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_ERR APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)

#define APS_ERR_ST(f)    printk(KERN_ERR  APS_TAG"%s %d : ", __FUNCTION__, __LINE__)


#define APS_LOG(fmt, args...)    printk(KERN_ERR APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_ERR APS_TAG fmt, ##args)                 

extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
extern void mt_eint_print_status(void);

static void ltr559_dyna_polling_do_work(struct work_struct *w);
static DECLARE_DELAYED_WORK(ltr559_dyna_polling_work, ltr559_dyna_polling_do_work);

static struct i2c_client *ltr559_i2c_client = NULL;

static const struct i2c_device_id ltr559_i2c_id[] = {{LTR559_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_ltr559={ I2C_BOARD_INFO("LTR_559ALS", 0x23)};

static int ltr559_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr559_i2c_remove(struct i2c_client *client);
static int ltr559_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int ltr559_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr559_i2c_resume(struct i2c_client *client);
static int ltr559_ps_enable(int gainrange);
static int dynamic_calibrate=0;

static int ps_trigger_high = 800;	
static int ps_trigger_low = 760;

static int ps_gainrange = PS_RANGE32;
static int als_gainrange = ALS_RANGE_600;

static int final_prox_val , prox_val;
static int final_lux_val;

static int ltr559_ps_raw_data;
static int min_ltr559_ps_raw_data;

static int ps_data_overflow;


static DEFINE_MUTEX(read_lock);

static int ltr559_als_read(int gainrange);
static int ltr559_ps_read(void);




typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

struct ltr559_i2c_addr {    
    u8  write_addr;  
    u8  ps_thd;     
};


struct ltr559_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;
    struct mutex lock;
	
    struct ltr559_i2c_addr  addr;

     
    u16		    als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_debounce;   
    atomic_t    als_deb_on;     
    atomic_t    als_deb_end;    
    atomic_t    ps_mask;        
    atomic_t    ps_debounce;    
    atomic_t    ps_deb_on;      
    atomic_t    ps_deb_end;     
    atomic_t    ps_suspend;
    atomic_t    als_suspend;

    
    u16         als;
    u16          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;    
    atomic_t    ps_cmd_val;     
    atomic_t    ps_thd_val;     
	atomic_t    ps_thd_val_high;     
	atomic_t    ps_thd_val_low;     
    ulong       enable;         
    ulong       pending_intr;   

	int ps_enable;

	struct workqueue_struct *lp_wq;

	struct class *ltr559_class;
	struct device *ps_dev;

	unsigned long j_start;
	unsigned long j_end;
	int	ps_pocket_mode;

    
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};

 struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;

static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
static int intr_flag_value = -1;


static struct ltr559_priv *ltr559_obj = NULL;
static struct platform_driver ltr559_alsps_driver;

static struct i2c_driver ltr559_i2c_driver = {	
	.probe      = ltr559_i2c_probe,
	.remove     = ltr559_i2c_remove,
	.detect     = ltr559_i2c_detect,
	.suspend    = ltr559_i2c_suspend,
	.resume     = ltr559_i2c_resume,
	.id_table   = ltr559_i2c_id,
	
	.driver = {
		
		.name           = LTR559_DEV_NAME,
	},
};



static int ltr559_i2c_read_reg(u8 regnum)
{
    u8 buffer[1],reg_value[1];
	int res = 0;
	mutex_lock(&read_lock);
	
	buffer[0]= regnum;
	res = i2c_master_send(ltr559_obj->client, buffer, 0x1);
	if(res <= 0)	{
	   
	   APS_ERR("read reg send res = %d\n",res);
		return res;
	}
	res = i2c_master_recv(ltr559_obj->client, reg_value, 0x1);
	if(res <= 0)
	{
		APS_ERR("read reg recv res = %d\n",res);
		return res;
	}
	mutex_unlock(&read_lock);
	return reg_value[0];
}

static int ltr559_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;
   
	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr559_obj->client, databuf, 0x2);

	if (res < 0)
		{
			APS_ERR("wirte reg send res = %d\n",res);
		   	return res;
		}
		
	else
		return 0;
}

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static ssize_t ltr559_dynamic_calibrate(void)			
{																				
	int ret=0;
	int i=0;
	int data;
	int data_total=0;
	ssize_t len = 0;
	int noise = 0;
	int count = 5;
	int max = 0;
	struct ltr559_priv *obj = ltr559_obj;
	if(!ltr559_obj)
	{	
		APS_ERR("ltr559_obj is null!!\n");
		
		return -1;
	}

	
	msleep(15);

	
	atomic_set(&obj->ps_thd_val_high,  noise+200);
	atomic_set(&obj->ps_thd_val_low, noise+80);
	

	for (i = 0; i < count; i++) 
	{
		
		
		msleep(15);
		
		data=ltr559_ps_read();
		if (data < 0) {
			i--;
			continue;
		}
				
		if(data & 0x8000){
			noise = 0;
			break;
		}else {
			noise=data;
		}	
		
		data_total+=data;

		if (max++ > 100) {
			
			return len;
		}
	}

	
	noise=data_total/count;

	dynamic_calibrate = noise;
	if(noise < 100){

			atomic_set(&obj->ps_thd_val_high,  noise+150);
			atomic_set(&obj->ps_thd_val_low, noise+60);
	}else if(noise < 200){
			atomic_set(&obj->ps_thd_val_high,  noise+160);
			atomic_set(&obj->ps_thd_val_low, noise+70);
	}else if(noise < 300){
			atomic_set(&obj->ps_thd_val_high,  noise+160);
			atomic_set(&obj->ps_thd_val_low, noise+70);
	}else if(noise < 400){
			atomic_set(&obj->ps_thd_val_high,  noise+160);
			atomic_set(&obj->ps_thd_val_low, noise+80);
	}else if(noise < 600){
			atomic_set(&obj->ps_thd_val_high,  noise+180);
			atomic_set(&obj->ps_thd_val_low, noise+90);
	}else if(noise < 1000){
		atomic_set(&obj->ps_thd_val_high,  noise+300);
		atomic_set(&obj->ps_thd_val_low, noise+180);	
	}else if(noise < 1250){
			atomic_set(&obj->ps_thd_val_high,  noise+400);
			atomic_set(&obj->ps_thd_val_low, noise+300);
	}
	else{
			
			
			
		printk(KERN_ERR "ltr558 the proximity sensor structure is error\n");
	}
	
	int ps_thd_val_low,	ps_thd_val_high ;
	
	ps_thd_val_low = atomic_read(&obj->ps_thd_val_low);
	ps_thd_val_high = atomic_read(&obj->ps_thd_val_high);
	printk(" ltr559_dynamic_calibrate end:noise=%d, obj->ps_thd_val_low= %d , obj->ps_thd_val_high = %d\n", noise, ps_thd_val_low, ps_thd_val_high);
	return 0;
}
#endif

static ssize_t ps_adc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	uint16_t ps_adc = 0;
	int ret;
	int int_gpio = -1;

	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}

	if(0 == ltr559_obj->hw->polling_mode_ps){
		int_gpio = mt_get_gpio_in(GPIO_ALS_EINT_PIN);
	}
	ps_adc = ltr559_ps_read();
	APS_ERR("%s: PS_ADC=0x%04X\n" , __func__, ps_adc);

	ret = sprintf(buf, "ADC[0x%04X], ENABLE = %d, intr_pin = %d, ps_pocket_mode = %d\n",
			ps_adc, ltr559_obj->ps_enable, int_gpio, ltr559_obj->ps_pocket_mode);

	return ret;
}

static DEVICE_ATTR(ps_adc, 0664, ps_adc_show, NULL);

static ssize_t ltr559_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	u8 dat = 0;
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	res = ltr559_als_read(als_gainrange);
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);    
	
}
static ssize_t ltr559_show_ps(struct device_driver *ddri, char *buf)
{
	int  res;
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	res = ltr559_ps_read();
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);     
}


static ssize_t ltr559_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(ltr559_obj->hw)
	{
	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr559_obj->hw->i2c_num, ltr559_obj->hw->power_id, ltr559_obj->hw->power_vol);
		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr559_obj->als_suspend), atomic_read(&ltr559_obj->ps_suspend));

	return len;
}

static ssize_t ltr559_store_status(struct device_driver *ddri, char *buf, size_t count)
{
	int status1,ret;
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "%d ", &status1))
	{ 
	    ret=ltr559_ps_enable(ps_gainrange);
		APS_DBG("iret= %d, ps_gainrange = %d\n", ret, ps_gainrange);
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;    
}


static ssize_t ltr559_show_reg(struct device_driver *ddri, char *buf, size_t count)
{
	int i,len=0;
	int reg[]={0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
		0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x97,0x98,0x99,0x9a,0x9e};
	for(i=0;i<27;i++)
		{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr559_i2c_read_reg(reg[i]));	

	    }
	return len;
}
static ssize_t ltr559_store_reg(struct device_driver *ddri, char *buf, size_t count)
{
	int ret,value;
	u8 reg;
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(2 == sscanf(buf, "%c %x ", &reg,&value))
	{ 
		APS_DBG("before write reg: %x, reg_value = %x  write value=%x\n", reg,ltr559_i2c_read_reg(reg),value);
	    ret=ltr559_i2c_write_reg(reg,value);
		APS_DBG("after write reg: %x, reg_value = %x\n", reg,ltr559_i2c_read_reg(reg));
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;    
}

static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, ltr559_show_als,   NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, ltr559_show_ps,    NULL);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, ltr559_show_status,  ltr559_store_status);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, ltr559_show_reg,   ltr559_store_reg);
static struct driver_attribute *ltr559_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
   
   
   
   
    &driver_attr_status,
   
    &driver_attr_reg,
};
static int ltr559_create_attr(struct driver_attribute *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr559_attr_list)/sizeof(ltr559_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, ltr559_attr_list[idx]))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr559_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
	static int ltr559_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(ltr559_attr_list)/sizeof(ltr559_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr559_attr_list[idx]);
	}
	
	return err;
}



static int ltr559_ps_set_thres()
{
	APS_FUN();

	int res;
	u8 databuf[2];
	
		struct i2c_client *client = ltr559_obj->client;
		struct ltr559_priv *obj = ltr559_obj;
		APS_DBG("ps_cali.valid: %d\n", ps_cali.valid);
	if(1 == ps_cali.valid)
	{
		databuf[0] = LTR559_PS_THRES_LOW_0; 
		databuf[1] = (u8)(ps_cali.far_away & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_LOW_1; 
		databuf[1] = (u8)((ps_cali.far_away & 0xFF00) >> 8);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_UP_0;	
		databuf[1] = (u8)(ps_cali.close & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_UP_1;	
		databuf[1] = (u8)((ps_cali.close & 0xFF00) >> 8);;
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
	}
	else
	{
		APS_LOG("ltr559_ps_set_thres obj->ps_thd_val_low= %d , obj->ps_thd_val_high = %d\n", atomic_read(&obj->ps_thd_val_low),atomic_read(&obj->ps_thd_val_high));
		databuf[0] = LTR559_PS_THRES_LOW_0; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_LOW_1; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low )>> 8) & 0x00FF);
		
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_UP_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_UP_1;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high) >> 8) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
	
	}

	res = 0;
	return res;
	
	EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;

}

static void ltr559_dyna_polling_do_work(struct work_struct *work)
{
	struct ltr559_priv *obj = ltr559_obj;
	u8 databuf[2];
	int res = 0;
	obj->j_end = jiffies;
	if (time_after(obj->j_end, (obj->j_start + 3* HZ))){
		obj->ps_pocket_mode = 0;
	}
	if (obj->ps_enable == 1) {
		if(intr_flag_value == -1)
			goto NEXT_POLLING;
		ltr559_ps_raw_data=ltr559_ps_read();
		if(ps_data_overflow) {
			APS_LOG("ps_data_overflow:%d, ltr559_ps_raw_data:%d\n",ltr559_ps_raw_data,ltr559_ps_raw_data);
			ps_data_overflow = 0;
		} else {
			APS_LOG("ltr559_ps_raw_data:%d, min_ltr559_ps_raw_data:%d\n", ltr559_ps_raw_data, min_ltr559_ps_raw_data);
			if (ltr559_ps_raw_data != 0) {
				if (min_ltr559_ps_raw_data >= ltr559_ps_raw_data) {
					min_ltr559_ps_raw_data = ltr559_ps_raw_data;
					atomic_set(&obj->ps_thd_val_high,  min_ltr559_ps_raw_data+TH_ADD+1);
					atomic_set(&obj->ps_thd_val_low, min_ltr559_ps_raw_data+TH_ADD);
					if (atomic_read(&obj->ps_thd_val_low) >= 0x07fe)
						atomic_set(&obj->ps_thd_val_low, 0x07fe);
					if (atomic_read(&obj->ps_thd_val_high) >= 0x07ff)
						atomic_set(&obj->ps_thd_val_high, 0x07ff);
					APS_LOG("set low thd:%d\n", atomic_read(&obj->ps_thd_val_low));
				}
			}
		}
		
		APS_DBG("intr_flag_value=%d\n",intr_flag_value);
		if(intr_flag_value){
			APS_DBG(" interrupt value ps will < ps_threshold_low");

			databuf[0] = LTR559_PS_THRES_LOW_0;
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
				goto NEXT_POLLING;
			databuf[0] = LTR559_PS_THRES_LOW_1;
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
				goto NEXT_POLLING;
			databuf[0] = LTR559_PS_THRES_UP_0;
			databuf[1] = (u8)(0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
				goto NEXT_POLLING;
			databuf[0] = LTR559_PS_THRES_UP_1;
			databuf[1] = (u8)((0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
				goto NEXT_POLLING;
			APS_DBG("obj->ps_thd_val_low=%d !\n",atomic_read(&obj->ps_thd_val_low));
		} else {
			
			databuf[0] = LTR559_PS_THRES_LOW_0;
			databuf[1] = (u8)(0 & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
				goto NEXT_POLLING;
			databuf[0] = LTR559_PS_THRES_LOW_1;
			databuf[1] = (u8)((0 & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
				goto NEXT_POLLING;
			databuf[0] = LTR559_PS_THRES_UP_0;
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
				goto NEXT_POLLING;
			databuf[0] = LTR559_PS_THRES_UP_1;
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
				goto NEXT_POLLING;
			APS_DBG("obj->ps_thd_val_high=%d !\n",atomic_read(&obj->ps_thd_val_high));
		}
NEXT_POLLING:
		queue_delayed_work(obj->lp_wq, &ltr559_dyna_polling_work, msecs_to_jiffies(200));
	}
}


static int ltr559_ps_enable(int gainrange)
{
	struct i2c_client *client = ltr559_obj->client;
	struct ltr559_priv *obj = ltr559_obj;
	u8 databuf[2];	
	int res;

	int data;
	hwm_sensor_data sensor_data;	


	int error;
	int setgain;
    APS_LOG("ltr559_ps_enable() ...start!\n");

	switch (gainrange) {
		case PS_RANGE16:
			setgain = MODE_PS_ON_Gain16;
			break;

		case PS_RANGE32:
			setgain = MODE_PS_ON_Gain32;
			break;

		case PS_RANGE64:
			setgain = MODE_PS_ON_Gain64;
			break;


		default:
			setgain = MODE_PS_ON_Gain16;
			break;
	}

	setgain |= (1<<5);
	APS_LOG("LTR559_PS setgain = %d!\n",setgain);

	error = ltr559_i2c_write_reg(LTR559_PS_CONTR, setgain); 
	if(error<0)
	{
	    APS_LOG("ltr559_ps_enable() error1\n");
	    return error;
	}
	
	mdelay(WAKEUP_DELAY);
    
   error = ltr559_i2c_write_reg(LTR559_PS_N_PULSES, 9); 
	if(error<0)
    {
        APS_LOG("ltr559_ps_enable() error2\n");
	    return error;
	} 
	data = ltr559_i2c_read_reg(LTR559_PS_CONTR);
	mdelay(WAKEUP_DELAY);
	#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI  
	if (data & 0x02) {
		
		if(0 == obj->hw->polling_mode_ps){
			mt_eint_mask(CUST_EINT_ALS_NUM);
		}
		
		
			
	}
	#endif	
	obj->j_start = jiffies;
	
		if(0 == obj->hw->polling_mode_ps)
		{		

			ltr559_ps_set_thres();

			#if 1
			databuf[0] = LTR559_INTERRUPT;	
			databuf[1] = 0x01;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
			
			databuf[0] = LTR559_INTERRUPT_PERSIST;	
			databuf[1] = 0x20;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
			mt_eint_unmask(CUST_EINT_ALS_NUM);			
			#endif
	
		}
	min_ltr559_ps_raw_data = 0x07ff;
	queue_delayed_work(obj->lp_wq, &ltr559_dyna_polling_work, msecs_to_jiffies(200));
 	APS_LOG("ltr559_ps_enable ...OK!\n");
 	
	return error;

	EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;
}

static int ltr559_ps_disable(void)
{
	int error;
	struct ltr559_priv *obj = ltr559_obj;
		
	error = ltr559_i2c_write_reg(LTR559_PS_CONTR, MODE_PS_StdBy); 
	if(error<0)
 	    APS_LOG("ltr559_ps_disable ...ERROR\n");
 	else
        APS_LOG("ltr559_ps_disable ...OK\n");

	if(0 == obj->hw->polling_mode_ps)
	{
	    
		mt_eint_mask(CUST_EINT_ALS_NUM);
	}
	cancel_delayed_work(&ltr559_dyna_polling_work);

	if (atomic_read(&obj->ps_thd_val_low) >= 500) {
		atomic_set(&obj->ps_thd_val_low, 200);
		atomic_set(&obj->ps_thd_val_high, 500);
	}
	min_ltr559_ps_raw_data = 0x07ff;
	intr_flag_value = -1;
	obj->ps_pocket_mode = 0;
	
	return error;
}


static int ltr559_ps_read(void)
{
	int psval_lo, psval_hi, psdata;

	psval_lo = ltr559_i2c_read_reg(LTR559_PS_DATA_0);
	APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
	if (psval_lo < 0){
	    
	    APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}
	psval_hi = ltr559_i2c_read_reg(LTR559_PS_DATA_1);
    APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);

	if (psval_hi < 0){
	    APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}
	ps_data_overflow = psval_hi >> 7;
	APS_DBG("ps_data_overflow =%d\n",ps_data_overflow);
	psdata = ((psval_hi & 7)* 256) + psval_lo;
    
    APS_DBG("ps_rawdata = %d\n", psdata);

	prox_val = psdata;
    
	out:
	final_prox_val = psdata;
	
	
	return psdata;
}


static int ltr559_als_enable(int gainrange)
{
	int error;
	int err;
	
 	u8 databuf[2];
	struct i2c_client *client = ltr559_obj->client;
	APS_LOG("gainrange = %d\n",gainrange);
	switch (gainrange)
	{
		case ALS_RANGE_64K:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range1);
			break;

		case ALS_RANGE_32K:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range2);
			break;

		case ALS_RANGE_16K:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range3);
			break;
			
		case ALS_RANGE_8K:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range4);
			break;
			
		case ALS_RANGE_1300:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range5);
			break;

		case ALS_RANGE_600:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range6);
			break;
			
		default:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range1);			
			APS_ERR("proxmy sensor gainrange %d!\n", gainrange);
			break;
	}

	mdelay(WAKEUP_DELAY);

	databuf[0] = LTR559_ALS_MEAS_RATE;	
	databuf[1] = 0x02;
	i2c_master_send(client, databuf, 0x2);
	

 	if(error<0)
 	    APS_LOG("ltr559_als_enable ...ERROR\n");
 	else
        APS_LOG("ltr559_als_enable ...OK\n");
        
	return error;
}


static int ltr559_als_disable(void)
{
	int error;
	error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_StdBy); 
	if(error<0)
 	    APS_LOG("ltr559_als_disable ...ERROR\n");
 	else
        APS_LOG("ltr559_als_disable ...OK\n");
	return error;
}

static int ltr559_als_read(int gainrange)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int  luxdata_int = -1;
	int ratio;


	alsval_ch1_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);

	alsval_ch0_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
	APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);

    if((alsval_ch1==0)||(alsval_ch0==0))
    {
        luxdata_int = 0;
        goto err;
    }
	ratio = (alsval_ch1*100) /(alsval_ch0+alsval_ch1);
	APS_DBG("ratio = %d  gainrange = %d\n",ratio,gainrange);
	if (ratio < 45){
		luxdata_int = (((17743 * alsval_ch0)+(11059 * alsval_ch1)))/10000;
	}
	else if ((ratio < 64) && (ratio >= 45)){
		luxdata_int = (((42785 * alsval_ch0)-(19548 * alsval_ch1)))/10000;
	}
	else if ((ratio <= 100) && (ratio >= 64)) {
		luxdata_int = (((5926 * alsval_ch0)+(1185 * alsval_ch1)))/10000;
	}
	else {
		luxdata_int = 0;
		}
	
	APS_DBG("als_value_lux = %d\n", luxdata_int);
	return luxdata_int;

	
err:
	final_lux_val = luxdata_int;
	APS_DBG("err als_value_lux = 0x%x\n", luxdata_int);
	return luxdata_int;
}



int ltr559_get_addr(struct alsps_hw *hw, struct ltr559_i2c_addr *addr)
{
	return 0;
}


void ltr559_eint_func(void)
{
	APS_FUN();

	struct ltr559_priv *obj = ltr559_obj;
	if(!obj)
	{
		return;
	}
	queue_work(obj->lp_wq, &obj->eint_work);
	
	
}



int ltr559_setup_eint(struct i2c_client *client)
{
	APS_FUN();
	struct ltr559_priv *obj = (struct ltr559_priv *)i2c_get_clientdata(client);        

	ltr559_obj = obj;
	mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, TRUE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, ltr559_eint_func, 0);
	mt_eint_unmask(CUST_EINT_ALS_NUM);  
    return 0;
}


static void ltr559_power(struct alsps_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "LTR559")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "LTR559")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}

static int ltr559_check_and_clear_intr(struct i2c_client *client) 
{
	APS_FUN();

	int res,intp,intl;
	u8 buffer[2];	
	u8 temp;
		
		
	
		buffer[0] = LTR559_ALS_PS_STATUS;
		res = i2c_master_send(client, buffer, 0x1);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		res = i2c_master_recv(client, buffer, 0x1);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		temp = buffer[0];
		res = 1;
		intp = 0;
		intl = 0;
		if(0 != (buffer[0] & 0x02))
		{
			res = 0;
			intp = 1;
		}
		if(0 != (buffer[0] & 0x08))
		{
			res = 0;
			intl = 1;		
		}
	
		if(0 == res)
		{
			if((1 == intp) && (0 == intl))
			{
				buffer[1] = buffer[0] & 0xfD;
				
			}
			else if((0 == intp) && (1 == intl))
			{
				buffer[1] = buffer[0] & 0xf7;
			}
			else
			{
				buffer[1] = buffer[0] & 0xf5;
			}
			buffer[0] = LTR559_ALS_PS_STATUS	;
			res = i2c_master_send(client, buffer, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
			}
			else
			{
				res = 0;
			}
		}
	
		return res;
	
	EXIT_ERR:
		APS_ERR("ltr559_check_and_clear_intr fail\n");
		return 1;

}


static int ltr559_check_intr(struct i2c_client *client) 
{
	APS_FUN();

	int res,intp,intl;
	u8 buffer[2];

	
	

	buffer[0] = LTR559_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = 1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x02))
	{
		res = 0;
		intp = 1;
	}
	if(0 != (buffer[0] & 0x08))
	{
		res = 0;
		intl = 1;		
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr559_check_intr fail\n");
	return 1;
}

static int ltr559_clear_intr(struct i2c_client *client) 
{
	int res;
	u8 buffer[2];

	APS_FUN();
	
	buffer[0] = LTR559_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	APS_DBG("buffer[0] = %d \n",buffer[0]);
	buffer[1] = buffer[0] & 0x01;
	buffer[0] = LTR559_ALS_PS_STATUS	;

	res = i2c_master_send(client, buffer, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	else
	{
		res = 0;
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr559_check_and_clear_intr fail\n");
	return 1;
}




static int ltr559_devinit(void)
{
	int res;
	int init_ps_gain;
	int init_als_gain;
	u8 databuf[2];	

	struct i2c_client *client = ltr559_obj->client;

	struct ltr559_priv *obj = ltr559_obj;   
	
	mdelay(PON_DELAY);

	
	databuf[0] = LTR559_ALS_CONTR;	
	databuf[1] = 0x02;
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
		return ltr559_ERR_I2C;
	}

	

#if 0
	
	init_ps_gain = PS_RANGE32;
	ps_gainrange = init_ps_gain;

	
	if (res < 0)
		goto EXIT_ERR;


	
	init_als_gain = ALS_RANGE_600;
	als_gainrange = init_als_gain;

	
	
	if (res < 0)
		goto EXIT_ERR;
#endif
	databuf[0] = LTR559_ALS_MEAS_RATE;	
	databuf[1] = 0x02;
	res = i2c_master_send(client, databuf, 0x2);
	if (res <= 0)
		goto EXIT_ERR;

	
	if(0 == obj->hw->polling_mode_ps)
	{	
		APS_LOG("eint enable");
		ltr559_ps_set_thres();
		
		databuf[0] = LTR559_INTERRUPT;	
		databuf[1] = 0x01;
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}

		databuf[0] = LTR559_INTERRUPT_PERSIST;	
		databuf[1] = 0x20;
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}

	}

	if((res = ltr559_setup_eint(client))!=0)
	{
		APS_ERR("setup eint: %d\n", res);
		return res;
	}
	
	if((res = ltr559_check_and_clear_intr(client)))
	{
		APS_ERR("check/clear intr: %d\n", res);
		
	}

	res = 0;

	EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;

}


static int ltr559_get_als_value(struct ltr559_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	APS_DBG("als  = %d\n",als); 
	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	
	if(idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}
	
	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}
		
		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);	
		return obj->hw->als_value[idx];
	}
	else
	{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
}
static int ltr559_get_ps_value(struct ltr559_priv *obj, u16 ps)
{
	int val,  mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	static int val_temp = 1;
	if((ps > atomic_read(&obj->ps_thd_val_high)))
	{
		val = 0;  
		val_temp = 0;
		intr_flag_value = 1;
	}
			
	else if((ps < atomic_read(&obj->ps_thd_val_low)))
	{
		val = 1;  
		val_temp = 1;
		intr_flag_value = 0;
	}
	else
		val = val_temp;	
			
	
	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}
		
		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}
	else if (obj->als > 50000)
	{
		
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  
	}

	if(!invalid)
	{
		APS_DBG("PS:  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		return -1;
	}	
}



static void ltr559_eint_work(struct work_struct *work)
{
	struct ltr559_priv *obj = (struct ltr559_priv *)container_of(work, struct ltr559_priv, eint_work);
	int err;
	hwm_sensor_data sensor_data;
	int temp_noise;	 
	u8 databuf[2];
	int res = 0;

 	u8 i,j=0;
	int data = 0;
	unsigned long data_total = 0;

	obj->j_end = jiffies;

	APS_FUN();
	err = ltr559_check_intr(obj->client);
	if(err < 0)
	{
		APS_ERR("ltr559_eint_work check intrs: %d\n", err);
	}
	else
	{
		
		
#if 0
	for (i = 0; i < 3; i++) 	
	{		
		mdelay(10);
		data = ltr559_ps_read();
		if(data == 0){
		j++;
		}
		data_total += data;
	}
	obj->ps = data_total/(3- j);
#else
	obj->ps = ltr559_ps_read();
#endif

    	if(obj->ps < 0)
    	{
    		err = -1;
    		return;
    	}
				
		APS_DBG("ltr559_eint_work rawdata ps=%d als_ch0=%d!\n",obj->ps,obj->als);
		sensor_data.values[0] = ltr559_get_ps_value(obj, obj->ps);
		sensor_data.values[1] = obj->ps;
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		if(sensor_data.values[0]==0 && time_before(obj->j_end, (obj->j_start + NEAR_DELAY_TIME))){
			obj->ps_pocket_mode = 1;
			sensor_data.values[0] = 1;
		}
#if 0
		APS_DBG("intr_flag_value=%d\n",intr_flag_value);
		if(intr_flag_value){
			APS_DBG(" interrupt value ps will < ps_threshold_low");

			databuf[0] = LTR559_PS_THRES_LOW_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_LOW_1;	
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_0;	
			databuf[1] = (u8)(0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_1; 
			databuf[1] = (u8)((0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
			APS_DBG("obj->ps_thd_val_low=%d !\n",atomic_read(&obj->ps_thd_val_low));
			if(res <= 0)
			{
				return;
			}
		}
		else{	
				
			
			
			if(obj->ps < 100){			
				atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
				atomic_set(&obj->ps_thd_val_low, obj->ps+60);
			}else if(obj->ps < 200){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
				atomic_set(&obj->ps_thd_val_low, obj->ps+70);
			}else if(obj->ps < 300){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+160);
				atomic_set(&obj->ps_thd_val_low, obj->ps+70);
			}else if(obj->ps < 400){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+160);
				atomic_set(&obj->ps_thd_val_low, obj->ps+80);
			}else if(obj->ps < 600){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+180);
				atomic_set(&obj->ps_thd_val_low, obj->ps+90);
			}else if(obj->ps < 1000){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+300);
				atomic_set(&obj->ps_thd_val_low, obj->ps+180);	
			}else if(obj->ps < 1250){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+400);
				atomic_set(&obj->ps_thd_val_low, obj->ps+300);
			}
			else{
				atomic_set(&obj->ps_thd_val_high,  2047);
				atomic_set(&obj->ps_thd_val_low, 1000);
				printk(KERN_ERR "ltr559 the proximity sensor structure is error\n");
			}
		
			dynamic_calibrate = obj->ps;

			

			if(obj->ps	> 50){
				temp_noise = obj->ps - 50;
			}else{
				temp_noise = 0;
			}

			
			databuf[0] = LTR559_PS_THRES_LOW_0; 
			databuf[1] = (u8)(0 & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_LOW_1; 
			databuf[1] = (u8)((0 & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_1; 
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
			APS_DBG("obj->ps_thd_val_high=%d !\n",atomic_read(&obj->ps_thd_val_high));
			if(res <= 0)
			{
				return;
			}
		}
#endif
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		
		if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		{
		  APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
		}
	}
	ltr559_clear_intr(obj->client);
    mt_eint_unmask(CUST_EINT_ALS_NUM);       
}



static int ltr559_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr559_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
static int ltr559_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}


static int ltr559_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr559_priv *obj = i2c_get_clientdata(client);  
	int err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;
	APS_DBG("cmd= %d\n", cmd); 
	switch (cmd)
	{
		case ALSPS_SET_PS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr559_ps_enable(ps_gainrange);
				if(err < 0)
				{
					APS_ERR("enable ps fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
			    err = ltr559_ps_disable();
				if(err < 0)
				{
					APS_ERR("disable ps fail: %d\n", err); 
					goto err_out;
				}
				
				clear_bit(CMC_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:
			APS_DBG("ALSPS_GET_PS_DATA\n"); 
		    obj->ps = ltr559_ps_read();
			if(obj->ps < 0)
			{
				goto err_out;
			}
			
			dat = ltr559_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:
			
			#if 1
			obj->ps = ltr559_ps_read();
			if(obj->ps < 0)
			{
				goto err_out;
			}
			dat = obj->ps;
			#endif

			
			
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_SET_ALS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr559_als_enable(als_gainrange);
				if(err < 0)
				{
					APS_ERR("enable als fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
			    err = ltr559_als_disable();
				if(err < 0)
				{
					APS_ERR("disable als fail: %d\n", err); 
					goto err_out;
				}
				clear_bit(CMC_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
		    obj->als = ltr559_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = ltr559_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			obj->als = ltr559_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = obj->als;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		default:
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}

static struct file_operations ltr559_fops = {
	
	.open = ltr559_open,
	.release = ltr559_release,
	.unlocked_ioctl = ltr559_unlocked_ioctl,
};
static struct miscdevice ltr559_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr559_fops,
};

static int ltr559_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct ltr559_priv *obj = i2c_get_clientdata(client);    
	int err;
	APS_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		atomic_set(&obj->als_suspend, 1);
		err = ltr559_als_disable();
		if(err < 0)
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		#if 0		
		atomic_set(&obj->ps_suspend, 1);
		err = ltr559_ps_disable();
		if(err < 0)
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}
				
		ltr559_power(obj->hw, 0);

		#endif
		if (obj->ps_enable == 1)
			cancel_delayed_work(&ltr559_dyna_polling_work);

	}
	return 0;
}
static int ltr559_i2c_resume(struct i2c_client *client)
{
	struct ltr559_priv *obj = i2c_get_clientdata(client);        
	int err;
	err = 0;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ltr559_power(obj->hw, 1);
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr559_als_enable(als_gainrange);
	    if (err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}
	
	if(test_bit(CMC_BIT_PS,  &obj->enable))
	{
		
	    if (err < 0)
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}
	if (obj->ps_enable == 1)
		queue_delayed_work(obj->lp_wq, &ltr559_dyna_polling_work, msecs_to_jiffies(200));
	return 0;
}

static void ltr559_early_suspend(struct early_suspend *h) 
{   
	struct ltr559_priv *obj = container_of(h, struct ltr559_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}
	
	atomic_set(&obj->als_suspend, 1); 
	err = ltr559_als_disable();
	if(err < 0)
	{
		APS_ERR("disable als fail: %d\n", err); 
	}
	if (obj->ps_enable == 1)
		cancel_delayed_work(&ltr559_dyna_polling_work);
}

static void ltr559_late_resume(struct early_suspend *h)
{   
	struct ltr559_priv *obj = container_of(h, struct ltr559_priv, early_drv);         
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr559_als_enable(als_gainrange);
		if(err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
	if (obj->ps_enable == 1)
		queue_delayed_work(obj->lp_wq, &ltr559_dyna_polling_work, msecs_to_jiffies(200));
}

int ltr559_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr559_priv *obj = (struct ltr559_priv *)self;
	
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{				
				value = *(int *)buff_in;
				if(value)
				{
				    err = ltr559_ps_enable(ps_gainrange);
					if(err < 0)
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);
					obj->ps_enable=1;
				}
				else
				{
				    err = ltr559_ps_disable();
					if(err < 0)
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
					obj->ps_enable=0;
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				APS_ERR("get sensor ps data !\n");
				sensor_data = (hwm_sensor_data *)buff_out;
				obj->ps = ltr559_ps_read();
    			if(obj->ps < 0)
    			{
    				err = -1;
    				break;
    			}
				sensor_data->values[0] = ltr559_get_ps_value(obj, obj->ps);
				sensor_data->values[1] = obj->ps;		
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

int ltr559_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr559_priv *obj = (struct ltr559_priv *)self;

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;				
				if(value)
				{
				    err = ltr559_als_enable(als_gainrange);
					if(err < 0)
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
				    err = ltr559_als_disable();
					if(err < 0)
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
				}
				
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				APS_ERR("get sensor als data !\n");
				sensor_data = (hwm_sensor_data *)buff_out;
				obj->als = ltr559_als_read(als_gainrange);
                #if 0
				sensor_data->values[0] = obj->als;
				#else
				sensor_data->values[0] = ltr559_get_als_value(obj, obj->als);
				#endif
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
			}
			break;
		default:
			APS_ERR("light sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}


static int ltr559_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, LTR559_DEV_NAME);
	return 0;
}

static int ltr559_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr559_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr559_obj = obj;

	obj->hw = get_cust_alsps_hw();
	ltr559_get_addr(obj->hw, &obj->addr);

	INIT_WORK(&obj->eint_work, ltr559_eint_work);
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	
	
	atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	obj->als_modulus = (400*100)/(16*150);
										
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	APS_LOG("ltr559_devinit() start...!\n");
	ltr559_i2c_client = client;

	if(err = ltr559_devinit())
	{
		goto exit_init_failed;
	}
	APS_LOG("ltr559_devinit() ...OK!\n");

	

	if(err = misc_register(&ltr559_device))
	{
		APS_ERR("ltr559_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	obj->lp_wq = create_singlethread_workqueue("ltr559_wq");
	if (!obj->lp_wq) {
		APS_ERR("lrt599 can't create workqueue\n");
		err = -ENOMEM;
		goto err_create_singlethread_workqueue;
	}

	obj->ltr559_class = class_create(THIS_MODULE, "optical_sensors");
	if (IS_ERR(obj->ltr559_class)) {
		err = PTR_ERR(obj->ltr559_class);
		obj->ltr559_class = NULL;
		goto err_create_class;
	}

	obj->ps_dev = device_create(obj->ltr559_class,	NULL, 0, "%s", "proximity");
	if (unlikely(IS_ERR(obj->ps_dev))) {
		err = PTR_ERR(obj->ps_dev);
		obj->ps_dev = NULL;
		goto err_create_ps_device;
	}

	err = device_create_file(obj->ps_dev, &dev_attr_ps_adc);
	if (err)
		goto err_create_ps_device_file;

	
	if(err = ltr559_create_attr(&ltr559_alsps_driver.driver))
	{
		printk(KERN_ERR "create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}


	obj_ps.self = ltr559_obj;
	
	if(1 == obj->hw->polling_mode_ps)
	{
		obj_ps.polling = 1;
	}
	else
	{
		obj_ps.polling = 0;
	}
	obj_ps.sensor_operate = ltr559_ps_operate;
	if(err = hwmsen_attach(ID_PROXIMITY, &obj_ps))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	
	obj_als.self = ltr559_obj;
	obj_als.polling = 1;
	obj_als.sensor_operate = ltr559_als_operate;
	if(err = hwmsen_attach(ID_LIGHT, &obj_als))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}


#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = ltr559_early_suspend,
	obj->early_drv.resume   = ltr559_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	misc_deregister(&ltr559_device);
	err_create_ps_device_file:
	device_unregister(obj->ps_dev);
	err_create_ps_device:
	class_destroy(obj->ltr559_class);
	err_create_class:
	err_create_singlethread_workqueue:
	exit_misc_device_register_failed:
	exit_init_failed:
	
	exit_kfree:
	kfree(obj);
	exit:
	ltr559_i2c_client = NULL;           
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}


static int ltr559_i2c_remove(struct i2c_client *client)
{
	int err;	
	if(err = ltr559_delete_attr(&ltr559_i2c_driver.driver))
	{
		APS_ERR("ltr559_delete_attr fail: %d\n", err);
	} 

	if(err = misc_deregister(&ltr559_device))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	ltr559_i2c_client = NULL;
	destroy_workqueue(ltr559_obj->lp_wq);
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
static int ltr559_probe(struct platform_device *pdev) 
{
	struct alsps_hw *hw = get_cust_alsps_hw();

	ltr559_power(hw, 1);
	
	
	
	if(i2c_add_driver(&ltr559_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
static int ltr559_remove(struct platform_device *pdev)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();    
	ltr559_power(hw, 0);    
	i2c_del_driver(&ltr559_i2c_driver);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{ .compatible = "mediatek,als_ps", },
	{},
};
#endif

static struct platform_driver ltr559_alsps_driver =
{
	.probe      = ltr559_probe,
	.remove     = ltr559_remove,    
	.driver     = 
	{
		.name = "als_ps",
        #ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
		#endif
	}
};

#ifdef CONFIG_OF
static struct platform_device ltr559_alsps_device={
	.name="als_ps",
	.id=-1
};
#endif

static int __init ltr559_init(void)
{
       struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();
	
	i2c_register_board_info(hw->i2c_num, &i2c_ltr559, 1);
	
#ifdef CONFIG_OF
	if(platform_device_register(&ltr559_alsps_device))
	{
		APS_ERR("failed to register device");
		return -ENODEV;
	}
#endif

	if(platform_driver_register(&ltr559_alsps_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;
}
static void __exit ltr559_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&ltr559_alsps_driver);
}
module_init(ltr559_init);
module_exit(ltr559_exit);
MODULE_AUTHOR("XX Xx");
MODULE_DESCRIPTION("LTR-559ALS Driver");
MODULE_LICENSE("GPL");

