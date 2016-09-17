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
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <linux/sensors_io.h>
#include "CwMcuSensor.h"
#include <cust_cwmcu.h>

#define SAVE_PATH_ACC       "/data/system/cw_calibrator_acc.ini"
#define SAVE_PATH_MAG       "/data/system/cw_calibrator_mag.ini"
#define SAVE_PATH_GYR       "/data/system/cw_calibrator_gyr.ini"
#define SAVE_PATH_LIGHT     "/data/system/cw_calibrator_light.ini"
#define SAVE_PATH_PROXIMITY "/data/system/cw_calibrator_proximity.ini"

//===============================================================
// gsensor
static int cw_accel_open(struct inode *inode, struct file *file)
{
	factory_active_sensor(ACCELERATION, 1);
	return nonseekable_open(inode, file);
}

static int cw_accel_release(struct inode *inode, struct file *file)
{
	factory_active_sensor(ACCELERATION, 0);
	file->private_data = NULL;
	return 0;
}
//========================
// gyroscope
static int cw_gyro_open(struct inode *inode, struct file *file)
{
	factory_active_sensor(GYRO, 1);
	return nonseekable_open(inode, file);
}

static int cw_gyro_release(struct inode *inode, struct file *file)
{
	factory_active_sensor(GYRO, 0);
	file->private_data = NULL;
	return 0;
}
//========================
// msensor
static int cw_mag_open(struct inode *inode, struct file *file)
{
	factory_active_sensor(MAGNETIC, 1);
	return nonseekable_open(inode, file);
}

static int cw_mag_release(struct inode *inode, struct file *file)
{
	factory_active_sensor(MAGNETIC, 0);
	file->private_data = NULL;
	return 0;
}

//========================
// alsps
static int cw_alsps_open(struct inode *inode, struct file *file)
{
	factory_active_sensor(LIGHT, 1);
	factory_active_sensor(PROXIMITY, 1);
	return nonseekable_open(inode, file);
}

static int cw_alsps_release(struct inode *inode, struct file *file)
{
	factory_active_sensor(LIGHT, 0);
	factory_active_sensor(PROXIMITY, 0);;
	file->private_data = NULL;
	return 0;
}

//========================
// barometer
static int cw_barometer_open(struct inode *inode, struct file *file)
{
	factory_active_sensor(PRESSURE, 1);
	return nonseekable_open(inode, file);
}

static int cw_barometer_release(struct inode *inode, struct file *file)
{
	factory_active_sensor(PRESSURE, 0);;
	file->private_data = NULL;
	return 0;
}
//========================
// heart rate monitor
static int cw_hrm_open(struct inode *inode, struct file *file)
{
	factory_active_sensor(HEARTBEAT, 1);
	return nonseekable_open(inode, file);
}

static int cw_hrm_release(struct inode *inode, struct file *file)
{
	factory_active_sensor(HEARTBEAT, 0);;
	file->private_data = NULL;
	return 0;
}

//========================
static long cw_factory_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
	int data_event[3] = {0};
	char strbuf[256];
	void __user *data;
	int err = 0;
    SENSOR_DATA sensor_data = {0};

	//CW_DEBUG("[FACTORY]factory ioctl, cmd=0x%8x", cmd);
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
		CW_ERROR("[FACTORY]access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	data = (void __user *) arg;
	if(data == NULL)
	{		
		return -EINVAL;	  
	}

	switch(cmd)
	{
	    //GSENSOR
		case GSENSOR_IOCTL_READ_SENSORDATA:
            CW_DEBUG("cw_factory_ioctl: GSENSOR_IOCTL_READ_SENSORDATA");
			if (factory_data_read(ACCELERATION, data_event) < 0)
			{
				CW_ERROR("[FACTORY] accel read data failed");	
			}
			CW_DEBUG("[FACTORY] accel data[]={%d %d %d}", data_event[0]*10, data_event[1]*10, data_event[2]*10);
			sprintf(strbuf, "%4x %4x %4x", data_event[0]*10, data_event[1]*10, data_event[2]*10);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;
        case GSENSOR_IOCTL_CLR_CALI:
            CW_INFO("[FACTORY] clear calibration data");
            factory_clear_calibrator_data(ACCELERATION);
            break;
        case GSENSOR_IOCTL_SET_CALI:
            CW_INFO("[FACTORY] GSENSOR_IOCTL_SET_CALI");
            if (copy_from_user(&sensor_data, data, sizeof(sensor_data)))
            {
                CW_INFO("[FACTORY] ACCEL calibration data failed");
                err = -EFAULT;
                break;    
            }

            CW_INFO("GSENSOR_IOCTL_SET_CALI data : (%d, %d, %d)!\n", sensor_data.x, sensor_data.y, sensor_data.z);           
            
            factory_set_calibrator_data(ACCELERATION, &sensor_data);
            break;    
        case GSENSOR_IOCTL_GET_CALI:
            CW_INFO("[FACTORY] GSENSOR_IOCTL_GET_CALI");           
            factory_get_calibrator_data(ACCELERATION, &sensor_data);
 
            copy_to_user(data, &sensor_data, sizeof(sensor_data));
            break;
       
        //GYROSCOPE    
		case GYROSCOPE_IOCTL_READ_SENSORDATA:
            CW_DEBUG("cw_factory_ioctl: GYROSCOPE_IOCTL_READ_SENSORDATA");
			if (factory_data_read(GYRO, data_event) < 0)
			{
				CW_ERROR("[FACTORY] gyro read data failed");	
			}
			CW_DEBUG("[FACTORY] gyro data[]={%d %d %d}", data_event[0]*10, data_event[1]*10, data_event[2]*10);
			sprintf(strbuf, "%4x %4x %4x", data_event[0]*10, data_event[1]*10, data_event[2]*10);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;		
        case GYROSCOPE_IOCTL_CLR_CALI:
            CW_INFO("[FACTORY] clear calibration data");
            factory_clear_calibrator_data(GYRO);
            break;
        case GYROSCOPE_IOCTL_SET_CALI:
            CW_INFO("[FACTORY] GYROSCOPE_IOCTL_SET_CALI");
            if (copy_from_user(&sensor_data, data, sizeof(sensor_data)))
            {
                CW_INFO("[FACTORY] Gyro calibration data failed");
                err = -EFAULT;
                break;    
            }

            CW_INFO("GYROSCOPE_IOCTL_SET_CALI data : (%d, %d, %d)!\n", sensor_data.x, sensor_data.y, sensor_data.z);           
            
            factory_set_calibrator_data(GYRO, &sensor_data);
            break;
            
        case GYROSCOPE_IOCTL_GET_CALI:
            CW_INFO("[FACTORY] GYROSCOPE_IOCTL_GET_CALI");
            factory_get_calibrator_data(GYRO, &sensor_data);

            copy_to_user(data, &sensor_data, sizeof(sensor_data));
            break;

        //MSENSOR    
		case MSENSOR_IOCTL_READ_FACTORY_SENSORDATA	:
            CW_DEBUG("cw_factory_ioctl: MSENSOR_IOCTL_READ_FACTORY_SENSORDATA");
			break;
		case MSENSOR_IOCTL_READ_SENSORDATA:
            CW_DEBUG("cw_factory_ioctl: MSENSOR_IOCTL_READ_SENSORDATA");
			if (factory_data_read(MAGNETIC, data_event) < 0)
			{
				CW_ERROR("[FACTORY] msensor read data failed");	
			}
			CW_DEBUG("[FACTORY] CW_MAGNETIC data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
			sprintf(strbuf, "%4x %4x %4x", data_event[0]/100, data_event[1]/100, data_event[2]/100);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;			

        //ALSPS
		case ALSPS_SET_PS_MODE:
		case ALSPS_SET_ALS_MODE:
		case ALSPS_GET_ALS_RAW_DATA:	
			break;
		case ALSPS_GET_PS_RAW_DATA:
            CW_DEBUG("cw_factory_ioctl: ALSPS_GET_PS_RAW_DATA");
			if (factory_data_read(PROXIMITY, data_event) < 0)
			{
				CW_ERROR("[FACTORY] PS read data failed");	
			}
			CW_DEBUG("[FACTORY] CW_PROXIMITY data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
			if(copy_to_user(data, &data_event[0], sizeof(int)))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;
		case ALSPS_GET_PS_THRESHOLD_HIGH:
			CW_DEBUG("cw_factory_ioctl: ALSPS_GET_PS_THRESHOLD_HIGH");
			data_event[0] = PS_HIGH_THRESHOLD;
			if(copy_to_user(data, &data_event[0], sizeof(int)))
            {
            	err = -EFAULT;
                break;
            }
			break;
		case ALSPS_GET_PS_THRESHOLD_LOW:
			CW_DEBUG("cw_factory_ioctl: ALSPS_GET_PS_THRESHOLD_LOW");
			data_event[0] = PS_LOW_THRESHOLD;
			if(copy_to_user(data, &data_event[0], sizeof(int)))
            {
            	err = -EFAULT;
                break;
            }
			break;	
		case ALSPS_GET_PS_TEST_RESULT:
			CW_DEBUG("cw_factory_ioctl: ALSPS_GET_PS_TEST_RESULT");
			factory_data_read(PROXIMITY, data_event);
			data_event[0] = (data_event[0] > PS_HIGH_THRESHOLD) ? 0 : 1; 
			if(copy_to_user(data, &data_event[0], sizeof(int)))
            {
            	err = -EFAULT;
                break;
            }
			break;			
        //BAROMETER
		case BAROMETER_GET_TEMP_DATA:
            CW_DEBUG("cw_factory_ioctl: BAROMETER_GET_TEMP_DATA");
			break;
		case BAROMETER_GET_PRESS_DATA:
            CW_DEBUG("cw_factory_ioctl: BAROMETER_GET_PRESS_DATA");
			if (factory_data_read(PRESSURE, data_event) < 0)
			{
				CW_ERROR("[FACTORY] PS read data failed");	
			}
			data_event[0] *= 10;
			CW_DEBUG("[FACTORY] CW_PROXIMITY data[]={%d %d %d}", data_event[0]*10, data_event[1], data_event[2]);
			if(copy_to_user(data, &data_event[0], sizeof(int)))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;

        //HEARTRATE
		case HRM_READ_SENSOR_DATA:
            CW_DEBUG("cw_factory_ioctl: HRM_READ_SENSOR_DATA");
			if (factory_data_read(HEARTBEAT, data_event) < 0)
			{
				CW_ERROR("[FACTORY] HRM read data failed");	
			}
			
			CW_DEBUG("[FACTORY] CW_HEARTRATE_MONITOR data[]={%d %d %d}", data_event[0], data_event[1], data_event[2]);
            sprintf(strbuf, "%d %d %d", data_event[0], data_event[1], data_event[2]);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;

		default:
			CW_INFO("[FACTORY]unknown IOCTL: 0x%08x\n", cmd);
			break;
			
	}

	return err;
}

//======================== 
//accelerometer
static struct file_operations cw_accel_fops = {
	.owner = THIS_MODULE,
	.open = cw_accel_open,
	.release = cw_accel_release,
	.unlocked_ioctl = cw_factory_ioctl,
};

static struct miscdevice cw_accel_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &cw_accel_fops,
};
//======================== 
//gyroscope
static struct file_operations cw_gyro_fops = {
	.owner = THIS_MODULE,
	.open = cw_gyro_open,
	.release = cw_gyro_release,
	.unlocked_ioctl = cw_factory_ioctl,
};

static struct miscdevice cw_gyro_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gyroscope",
	.fops = &cw_gyro_fops,
};
//======================== 
//msensor
static struct file_operations cw_mag_fops = {
	.owner = THIS_MODULE,
	.open = cw_mag_open,
	.release = cw_mag_release,
	.unlocked_ioctl = cw_factory_ioctl,
};

static struct miscdevice cw_mag_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "msensor",
	.fops = &cw_mag_fops,
};
//======================== 
//ALSPS
static struct file_operations cw_alsps_fops = {
	.owner = THIS_MODULE,
	.open = cw_alsps_open,
	.release = cw_alsps_release,
	.unlocked_ioctl = cw_factory_ioctl,
};

static struct miscdevice cw_alsps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &cw_alsps_fops,
};
//======================== 
//Barometer
static struct file_operations cw_barometer_fops = {
	.owner = THIS_MODULE,
	.open = cw_barometer_open,
	.release = cw_barometer_release,
	.unlocked_ioctl = cw_factory_ioctl,
};

static struct miscdevice cw_barometer_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "barometer",
	.fops = &cw_barometer_fops,
};
//======================== 
//Heart Rate Monitor
static struct file_operations cw_hrm_fops = {
	.owner = THIS_MODULE,
	.open = cw_hrm_open,
	.release = cw_hrm_release,
	.unlocked_ioctl = cw_factory_ioctl,
};

static struct miscdevice cw_hrm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hrm",
	.fops = &cw_hrm_fops,
};

//===============================================================
void init_factory_node(void)
{
	int err;

	//accel
	err = misc_register(&cw_accel_device);
	if (0 != err)
	{
		CW_ERROR("[FACTORY]init accel factory device node error");
	}

	//gyroscope
	err = misc_register(&cw_gyro_device);
	if (0 != err)
	{
		CW_ERROR("[FACTORY]init gyroscope factory device node error");
	}

	//msensor
	err = misc_register(&cw_mag_device);
	if (0 != err)
	{
		CW_ERROR("[FACTORY]init msensor factory device node error");
	}

	//alsps
	err = misc_register(&cw_alsps_device);
	if (0 != err)
	{
		CW_ERROR("[FACTORY]init alsps factory device node error");
	}

	//barometer
	err = misc_register(&cw_barometer_device);
	if (0 != err)
	{
		CW_ERROR("[FACTORY]init barometer factory device node error");
	}

	//heart rate monitor
	err = misc_register(&cw_hrm_device);
	if (0 != err)
	{
		CW_ERROR("[FACTORY]init hrm factory device node error");
	}

	CW_INFO("init factory node done!!");
}


