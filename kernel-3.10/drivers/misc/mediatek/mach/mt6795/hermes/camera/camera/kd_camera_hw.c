#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         xlog_printk(ANDROID_LOG_ERR, PFX , fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
                do {    \
                   xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg); \
                } while(0)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#define PK_XLOG_INFO(fmt, args...)
#endif

extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);
extern void ISP_MCLK3_EN(BOOL En);

int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{

u32 pinSetIdx = 0;//default main sensor

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4
#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3


//CAMERA_CMRST_PIN = 10
//CAMERA_CMPDN_PIN = 11

//CAMERA_CMRST1_PIN = 14
//CAMERA_CMPDN1_PIN = 15


u32 pinSet[3][8] = {
                        //for main sensor
                     {  CAMERA_CMRST_PIN,
                        CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        CAMERA_CMPDN_PIN,
                        CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     },
                     //for sub sensor
                     {  CAMERA_CMRST1_PIN,
                        CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        CAMERA_CMPDN1_PIN,
                        CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     },
                     //for main_2 sensor
                     {  CAMERA_CMRST2_PIN,
                        CAMERA_CMRST2_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,               /* ON state */
                        GPIO_OUT_ZERO,              /* OFF state */
                        CAMERA_CMPDN2_PIN,
                        CAMERA_CMPDN2_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     }
                   };

    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx){
        pinSetIdx = 0;

    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        pinSetIdx = 1;
    }
    else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
        pinSetIdx = 2;
    }
                     

   
    //power ON
    if (On) {
        if(pinSetIdx == 0 ) {
            ISP_MCLK1_EN(1);
        }
        else if (pinSetIdx == 1) {
            ISP_MCLK3_EN(1);
        }
        else if (pinSetIdx == 2) {
            ISP_MCLK2_EN(1);
        }
 
		if (currSensorName){
			if( (0 == strcmp("ov5670mipi", currSensorName))	|| (0 == strcmp("ov5670_2nd_mipi", currSensorName)) || (0 == strcmp("ov5670_flt_2nd_mipi", currSensorName))	|| (0 == strcmp("s5k5e8yxmipiraw", currSensorName))){
					
				if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d \n", CAMERA_POWER_VCAM_A);
					goto _kdCISModulePowerOn_exit_;
				}
				
				if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D2), power id = %d \n", CAMERA_POWER_VCAM_D2);
					goto _kdCISModulePowerOn_exit_;
				}
				
				mdelay(1);
				
				if(TRUE != hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1200,mode_name))
				{
					PK_DBG("[SUB CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
			
				mdelay(1);
				
				if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!!\n");}
				if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!!\n");}
				if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!!\n");}
				
			}else if( (0 == strcmp("s5k3m2mipiraw", currSensorName)) || (0 == strcmp("s5k3m2_2ndmipiraw", currSensorName)) || (0 == strcmp("s5k3l8mipiraw", currSensorName)) || (0 == strcmp("ov13853mipiraw", currSensorName))){
				if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
				if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
				if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
					
				if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d \n", CAMERA_POWER_VCAM_A);
					goto _kdCISModulePowerOn_exit_;
				}
				if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D2), power id = %d \n", CAMERA_POWER_VCAM_D2);
					goto _kdCISModulePowerOn_exit_;
				}
				if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1100,mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
				mdelay(1);
				
				if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
				
				if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A2), power id = %d \n", CAMERA_POWER_VCAM_A2);
					goto _kdCISModulePowerOn_exit_;
				}
			}		
			return 0;
		}	
//__empty_name			
		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))
		{
			PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D2), power id = %d \n", CAMERA_POWER_VCAM_D2);
			goto _kdCISModulePowerOn_exit_;
		}
		
		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
		{
			PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d \n", CAMERA_POWER_VCAM_A);
			goto _kdCISModulePowerOn_exit_;
		}
		
		if (currSensorName){
		// Это условие никогда не выполняется. Непонятно, почему компилятор это пропустил
			if ((0 == strcmp("imx135mipiraw", currSensorName)) 
			|| (0 == strcmp("imx220mipiraw", currSensorName)) 
			|| (0 == strcmp("imx214mipiraw", currSensorName)) ){
				if(TRUE != hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1000,mode_name))
				{
					PK_DBG("[SUB CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
			}else if (0 == strcmp("ov5648mipi", currSensorName)){
				if(TRUE != hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1500,mode_name))
				{
					PK_DBG("[SUB CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
			}else if (0 == strcmp("s5k2p8mipiraw", currSensorName)){
				if(TRUE != hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1200,mode_name))
				{
					PK_DBG("[SUB CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
			}
		}else{
//empty_name			
			if(TRUE != hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1800,mode_name))
			{
				PK_DBG("[SUB CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}
		}
			
		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
		{
			PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A2), power id = %d \n", CAMERA_POWER_VCAM_A2);
			goto _kdCISModulePowerOn_exit_;
		}
			
		if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
		if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
		if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
		mdelay(10);
		if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
		mdelay(1);
		
		if (currSensorName){
			if ((0 == strcmp("ov16825mipiraw", currSensorName))){
				if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!!\n");}
				if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!!\n");}
				if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!!\n");}
			}else if ((0 == strcmp("s5k3m2mipiraw", currSensorName))){
				return 0;
			}
			
		}
		if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!!\n");}
		if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!!\n");}
		if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!!\n");}
		
    } else {//power OFF
        if(pinSetIdx == 0 ) {
            ISP_MCLK1_EN(0);
        }
        else if (pinSetIdx == 1) {
            ISP_MCLK3_EN(0);
        }
        else if (pinSetIdx == 2) {
            ISP_MCLK2_EN(0);
        }
		if (currSensorName){
			if( (0 == strcmp("ov5670mipi", currSensorName))	|| (0 == strcmp("ov5670_2nd_mipi", currSensorName)) || (0 == strcmp("ov5670_flt_2nd_mipi", currSensorName))	|| (0 == strcmp("s5k5e8yxmipiraw", currSensorName))){
					
				if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!!\n");}
				if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!!\n");}
				if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!!\n");}

				if(TRUE != hwPowerDown(SUB_CAMERA_POWER_VCAM_D, mode_name))
				{
					PK_DBG("[SUB CAMERA SENSOR] Fail to disable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
				
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A, mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to disable analog power (VCAM_A), power id = %d \n", CAMERA_POWER_VCAM_A);
					goto _kdCISModulePowerOn_exit_;
				}
				
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2, mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to disable digital power (VCAM_D2), power id = %d \n", CAMERA_POWER_VCAM_D2);
					goto _kdCISModulePowerOn_exit_;
				}
				return 0;
				
			}else if( (0 == strcmp("s5k3m2mipiraw", currSensorName)) || (0 == strcmp("s5k3m2_2ndmipiraw", currSensorName)) || (0 == strcmp("s5k3l8mipiraw", currSensorName)) || (0 == strcmp("ov13853mipiraw", currSensorName))){
				if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
				if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
				if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
					
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A, mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to disable analog power (VCAM_A), power id = %d \n", CAMERA_POWER_VCAM_A);
					goto _kdCISModulePowerOn_exit_;
				}
				
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2, mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to disable analog power (VCAM_A2), power id = %d \n", CAMERA_POWER_VCAM_A2);
					goto _kdCISModulePowerOn_exit_;
				}
				
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2, mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to disable digital power (VCAM_D2), power id = %d \n", CAMERA_POWER_VCAM_D2);
					goto _kdCISModulePowerOn_exit_;
				}
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name))
				{
					PK_DBG("[CAMERA SENSOR] Fail to disable digital power (VCAM_D), power id = %d \n", CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
				return 0;
			}		
		}	
//__empty_name			
		if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
		if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!!\n");}
		
		if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
		if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!!\n");}
		
		if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
		
		if ((currSensorName) && ((0 == strcmp("ov16825mipiraw", currSensorName)))){
			if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], 1)){PK_DBG("[CAMERA SENSOR] set gpio failed!!\n");}
		}else{
  		   if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!!\n");}
  		}

		if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A, mode_name))
		{
			PK_DBG("[CAMERA SENSOR] Fail to disable analog power (VCAM_A), power id = %d \n", CAMERA_POWER_VCAM_A);
			goto _kdCISModulePowerOn_exit_;
		}

		if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2, mode_name))
		{
			PK_DBG("[CAMERA SENSOR] Fail to disable analog power (VCAM_A2), power id = %d \n", CAMERA_POWER_VCAM_A2);
			goto _kdCISModulePowerOn_exit_;
		}

		if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2, mode_name))
		{
			PK_DBG("[CAMERA SENSOR] Fail to disable digital power (VCAM_D2), power id = %d \n", CAMERA_POWER_VCAM_D2);
			goto _kdCISModulePowerOn_exit_;
		}
		
		if (currSensorName){
			if ((0 == strcmp("imx135mipiraw", currSensorName))){ 
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name))
				{
					PK_DBG("[SUB CAMERA SENSOR] Fail to disable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
			}else if (0 == strcmp("ov5648mipi", currSensorName)){
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name))
				{
					PK_DBG("[SUB CAMERA SENSOR] Fail to disable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
		}else{
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))
			{
				PK_DBG("[SUB CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", SUB_CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
                        }
        }

            
        }

    }

    return 0;

_kdCISModulePowerOn_exit_:
    return -EIO;
    
}

EXPORT_SYMBOL(kdCISModulePowerOn);

//!--
//
