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
#include <linux/fs.h>
#include <asm/atomic.h>
#include "icm30628.h"
#include "icm30628_factory.h"

static int invensense_accel_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int invensense_accel_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int invensense_gyro_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int invensense_gyro_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int invensense_mag_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int invensense_mag_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int invensense_alsps_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int invensense_alsps_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int invensense_barometer_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int invensense_barometer_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int invensense_hrm_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int invensense_hrm_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long invensense_factory_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
	int ret = 0;
	int data_event[3] = {0};
	char strbuf[256];
	void __user *data;
	int sensor_data[3]= {0};

	INV_DBG_FUNC_NAME;
	
	if(_IOC_DIR(cmd) & _IOC_READ){
		ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	} else if(_IOC_DIR(cmd) & _IOC_WRITE){
		ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(UNLIKELY(ret)){
		INV_ERR;
		return ret;
	}

	data = (void __user *) arg;
	
	if(UNLIKELY(data == NULL)){
		ret = -EINVAL;
		INV_ERR;
		return ret;	  
	}

	switch(cmd)
	{
		case GSENSOR_IOCTL_READ_SENSORDATA:
			printk("icm30628_factory_ioctl : GSENSOR_IOCTL_READ_SENSORDATA\n");
			icm30628_factory_sensor_enable(INV_ACCELEROMETER_NUM, true);
			msleep(100);
			ret = icm30628_factory_get_sensor_data(INV_ACCELEROMETER_NUM, data_event);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			data_event[0] = (data_event[0] * 1000)/HAL_DIV_ACCELEROMETER;
			data_event[1] = (data_event[1] * 1000)/HAL_DIV_ACCELEROMETER;
			data_event[2] = (data_event[2] * 1000)/HAL_DIV_ACCELEROMETER;
			printk("icm30628_factory_ioctl : accel data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
			sprintf(strbuf, "%4x %4x %4x", data_event[0], data_event[1], data_event[2]);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1)){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}				 
			break;
		case GSENSOR_IOCTL_CLR_CALI:
			printk("icm30628_factory_ioctl : GSENSOR_IOCTL_CLR_CALI\n");
			ret = icm30628_factory_clear_calibrator_data(INV_ACCELEROMETER_NUM);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			break;
		case GSENSOR_IOCTL_SET_CALI:
			printk("icm30628_factory_ioctl : GSENSOR_IOCTL_SET_CALI\n");
			if (copy_from_user(&sensor_data, data, sizeof(sensor_data))){
				ret = -EFAULT;
				INV_ERR;
				break;    
			}
			printk("icm30628_factory_ioctl : GSENSOR_IOCTL_SET_CALI data : (%d, %d, %d)\n", sensor_data[0], sensor_data[1], sensor_data[2]);           
			ret =icm30628_factory_set_calibrator_data(INV_ACCELEROMETER_NUM, sensor_data);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			break;    
		case GSENSOR_IOCTL_GET_CALI:
			printk("icm30628_factory_ioctl : GSENSOR_IOCTL_GET_CALI\n");
			ret = icm30628_factory_get_calibrator_data(INV_ACCELEROMETER_NUM, sensor_data);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			printk("icm30628_factory_ioctl : GSENSOR_IOCTL_GET_CALI data : (%d, %d, %d)\n", sensor_data[0], sensor_data[1], sensor_data[2]);           
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data))){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}				 
			break;
		case GYROSCOPE_IOCTL_READ_SENSORDATA:
			printk("icm30628_factory_ioctl : GYROSCOPE_IOCTL_READ_SENSORDATA\n");
			icm30628_factory_sensor_enable(INV_GYROSCOPE_NUM, true);
			msleep(100);
			ret = icm30628_factory_get_sensor_data(INV_GYROSCOPE_NUM, data_event);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			data_event[0] = (data_event[0] * 1000)/HAL_DIV_GYROSCOPE;
			data_event[1] = (data_event[1] * 1000)/HAL_DIV_GYROSCOPE;
			data_event[2] = (data_event[2] * 1000)/HAL_DIV_GYROSCOPE;
			printk("icm30628_factory_ioctl : gyro data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
			sprintf(strbuf, "%4x %4x %4x", data_event[0], data_event[1], data_event[2]);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1)){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}				 
			break;		
		case GYROSCOPE_IOCTL_CLR_CALI:
			printk("icm30628_factory_ioctl : GYROSCOPE_IOCTL_CLR_CALI\n");
			ret = icm30628_factory_clear_calibrator_data(INV_GYROSCOPE_NUM);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			break;
		case GYROSCOPE_IOCTL_SET_CALI:
			printk("icm30628_factory_ioctl : GYROSCOPE_IOCTL_SET_CALI\n");
			if (copy_from_user(&sensor_data, data, sizeof(sensor_data))){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}
			printk("icm30628_factory_ioctl : GYROSCOPE_IOCTL_SET_CALI data : (%d, %d, %d)\n", sensor_data[0], sensor_data[1], sensor_data[2]);           
			ret =icm30628_factory_set_calibrator_data(INV_GYROSCOPE_NUM, sensor_data);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			break;
		case GYROSCOPE_IOCTL_GET_CALI:
			printk("icm30628_factory_ioctl : GYROSCOPE_IOCTL_GET_CALI\n");
			ret = icm30628_factory_get_calibrator_data(INV_GYROSCOPE_NUM, sensor_data);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data))){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}				 
			break;
		case MSENSOR_IOCTL_READ_FACTORY_SENSORDATA	:
			printk("icm30628_factory_ioctl : MSENSOR_IOCTL_READ_FACTORY_SENSORDATA\n");
			break;
		case MSENSOR_IOCTL_READ_SENSORDATA:
			printk("icm30628_factory_ioctl : MSENSOR_IOCTL_READ_SENSORDATA\n");
			icm30628_factory_sensor_enable(INV_MAGNETIC_FIELD_NUM, true);
			msleep(100);
			ret = icm30628_factory_get_sensor_data(INV_MAGNETIC_FIELD_NUM, data_event);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			//data_event[0] = data_event[0] / HAL_DIV_GEOMAGNETIC_FIELD;
			//data_event[1] = data_event[1] / HAL_DIV_GEOMAGNETIC_FIELD;
			//data_event[2] = data_event[2] / HAL_DIV_GEOMAGNETIC_FIELD;
			printk("icm30628_factory_ioctl : magnetic data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
			sprintf(strbuf, "%4x %4x %4x", data_event[0], data_event[1], data_event[2]);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1)){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}				 
			break;			
		case ALSPS_SET_PS_MODE:
		case ALSPS_SET_ALS_MODE:
		case ALSPS_GET_ALS_RAW_DATA:	
			break;
		case ALSPS_GET_PS_RAW_DATA:
			printk("icm30628_factory_ioctl : ALSPS_GET_PS_RAW_DATA\n");
			icm30628_factory_sensor_enable(INV_PROXIMITY_NUM, true);
			msleep(100);
			ret = icm30628_factory_get_sensor_data(INV_PROXIMITY_NUM, data_event);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			printk("icm30628_factory_ioctl : proximity data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
			if(copy_to_user(data, &data_event[0], sizeof(int))){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}				 
			break;
		case BAROMETER_GET_TEMP_DATA:
			printk("icm30628_factory_ioctl : BAROMETER_GET_TEMP_DATA\n");
			break;
		case BAROMETER_GET_PRESS_DATA:
			printk("icm30628_factory_ioctl : BAROMETER_GET_PRESS_DATA\n");
			icm30628_factory_sensor_enable(INV_PRESSURE_NUM, true);
			msleep(100);
			ret = icm30628_factory_get_sensor_data(INV_PRESSURE_NUM, data_event);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			data_event[0] = (data_event[0]*10) / HAL_DIV_PRESSURE; //ftm_barometer.c divid 10,
			printk("icm30628_factory_ioctl : pressure data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
			if(copy_to_user(data, &data_event[0], sizeof(int))){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}				 
			break;
		case HRM_READ_SENSOR_DATA:
			printk("icm30628_factory_ioctl : HRM_READ_SENSOR_DATA\n");
			icm30628_factory_sensor_enable(INV_HEART_RATE_NUM, true);
			msleep(100);
			ret = icm30628_factory_get_sensor_data(INV_HEART_RATE_NUM, data_event);
			if(UNLIKELY(ret)){
				INV_ERR;
				break;	  
			}
			printk("icm30628_factory_ioctl : hrm data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
			sprintf(strbuf, "%d %d %d", data_event[0], data_event[1], data_event[2]);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1)){
				ret = -EFAULT;
				INV_ERR;
				break;	  
			}				 
			break;
		default:
			printk("icm30628_factory_ioctl : unkown command 0x%08x\n", cmd);
			break;
	}

	return ret;
}

static struct file_operations invensense_accel_fops = {
	.owner = THIS_MODULE,
	.open = invensense_accel_open,
	.release = invensense_accel_release,
	.unlocked_ioctl = invensense_factory_ioctl,
};

static struct miscdevice invensense_accel_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &invensense_accel_fops,
};

static struct file_operations invensense_gyro_fops = {
	.owner = THIS_MODULE,
	.open = invensense_gyro_open,
	.release = invensense_gyro_release,
	.unlocked_ioctl = invensense_factory_ioctl,
};

static struct miscdevice invensense_gyro_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gyroscope",
	.fops = &invensense_gyro_fops,
};

static struct file_operations invensense_mag_fops = {
	.owner = THIS_MODULE,
	.open = invensense_mag_open,
	.release = invensense_mag_release,
	.unlocked_ioctl = invensense_factory_ioctl,
};

static struct miscdevice invensense_mag_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "msensor",
	.fops = &invensense_mag_fops,
};

static struct file_operations invensense_alsps_fops = {
	.owner = THIS_MODULE,
	.open = invensense_alsps_open,
	.release = invensense_alsps_release,
	.unlocked_ioctl = invensense_factory_ioctl,
};

static struct miscdevice invensense_alsps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &invensense_alsps_fops,
};

static struct file_operations invensense_barometer_fops = {
	.owner = THIS_MODULE,
	.open = invensense_barometer_open,
	.release = invensense_barometer_release,
	.unlocked_ioctl = invensense_factory_ioctl,
};

static struct miscdevice invensense_barometer_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "barometer",
	.fops = &invensense_barometer_fops,
};

static struct file_operations invensense_hrm_fops = {
	.owner = THIS_MODULE,
	.open = invensense_hrm_open,
	.release = invensense_hrm_release,
	.unlocked_ioctl = invensense_factory_ioctl,
};

static struct miscdevice invensense_hrm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hrm",
	.fops = &invensense_hrm_fops,
};

void init_factory_node(void)
{
	int ret = 0;

	ret = misc_register(&invensense_accel_device);
	if(UNLIKELY(ret)){
		INV_ERR;
	}

	ret |= misc_register(&invensense_gyro_device);
	if(UNLIKELY(ret)){
		INV_ERR;
	}

	ret |= misc_register(&invensense_mag_device);
	if(UNLIKELY(ret)){
		INV_ERR;
	}

	ret |= misc_register(&invensense_alsps_device);
	if(UNLIKELY(ret)){
		INV_ERR;
	}

	ret |= misc_register(&invensense_barometer_device);
	if(UNLIKELY(ret)){
		INV_ERR;
	}

	ret |= misc_register(&invensense_hrm_device);
	if(UNLIKELY(ret)){
		INV_ERR;
	}

	if(ret){
		printk("icm30628 factory mode initialization error\n");		
		return;
	}

	printk("icm30628 factory mode initialization done\n");
}


