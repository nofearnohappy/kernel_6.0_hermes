/*
 * Verifiy on sensorhub F/W 102(SPI), 019(I2C)
*/ 

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include "CwMcuSensor.h"
#include <linux/gpio.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>
#include <mach/eint.h>
#include <cust_cwmcu.h>
#include <mach/mt_pm_ldo.h>
#include <linux/wakelock.h>
#include <linux/spi/spi.h>
#include <mach/mt_spi.h>


#define ACK		0x79
#define NACK		0x1F

#define DPS_MAX			(1 << (16 - 1))

/* Input poll interval in milliseconds */
#define CWMCU_POLL_INTERVAL	10
#define CWMCU_POLL_MAX		200
#define CWMCU_POLL_MIN		10

#define CWMCU_MAX_OUTPUT_ID		(CW_SNAP+1)
#define CWMCU_MAX_OUTPUT_BYTE		(CWMCU_MAX_OUTPUT_ID * 7)
#define CWMCU_MAX_DRIVER_OUTPUT_BYTE		256
static struct wake_lock cwmcu_wakelock;
static CWMCU_send_dummy_data(uint32_t timestamp);

int cw_fw_upgrade_status = -1;
module_param(cw_fw_upgrade_status, int, 0660);
int cw_tilt_wakeup_flag = 1;
module_param(cw_tilt_wakeup_flag, int, 0660);
int cw_data_log = 0;
module_param(cw_data_log, int, 0660);

static struct early_suspend cw_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB+1,
	.suspend = NULL,
	.resume = NULL,
};

/* turn on gpio interrupt if defined */
#define CWMCU_INTERRUPT

struct CWMCU_data *sensor;

static int CWMCU_Set_Mcu_Mode(int mode)
{
	switch (sensor->mcu_mode) {
	case CW_NORMAL:
		sensor->mcu_mode = mode;
		break;
	case CW_SLEEP:
		sensor->mcu_mode = mode;
		break;
	case CW_NO_SLEEP:
		sensor->mcu_mode = mode;
		break;
	case CW_BOOT:
		sensor->mcu_mode = mode;
		break;
	default:
		return -EAGAIN;
	}
	return 0;
}

static void cwmcu_powermode_switch(SWITCH_POWER_ID id, int onoff)
{
	if (onoff) {
		if (sensor->power_on_list == 0) {
			mt_set_gpio_out(GPIO_CW_MCU_WAKE_UP, onoff);
		}
		sensor->power_on_list |= ((uint32_t)(1) << id);
		usleep_range(200, 250);
	} else {
		sensor->power_on_list &= ~(1 << id);
		if (sensor->power_on_list == 0) {
			mt_set_gpio_out(GPIO_CW_MCU_WAKE_UP, onoff);
		}
	}
	/* CW_DEBUG("%s id = %d, onoff = %d", __func__, id, onoff); */
}

static void cwmcu_kernel_status(uint8_t status)
{
	sensor->kernel_status = status;
	CWMCU_bus_write(sensor, CwRegMapW_KERNEL_STATUS, &sensor->kernel_status, 1);
}

static void cwmcu_reinit(void)
{
	uint8_t data[10];
	int i =0;
	int j =0;
	for(i = 0;i<HANDLE_ID_END;i ++){
		for(j = 0;j<SENSORS_ID_END;j ++){
			if(sensor->sensors_info[i][j].en){
				data[0] = i;
				data[1] = j;
				data[2] = (sensor->sensors_info[i][j].rate ==0)?200:sensor->sensors_info[i][j].rate;
				data[3] = (uint8_t)sensor->sensors_info[i][j].timeout;
				data[4] = (uint8_t)(sensor->sensors_info[i][j].timeout >>8);
				if(CWMCU_bus_write(sensor, CwRegMapW_SET_ENABLE, data, 5)< 0){
					printk("%s:%s:(flush:bus write fail)\n",LOG_TAG_KERNEL ,__FUNCTION__);
				}
				printk("%s:%s:(id:%d enable)\n",LOG_TAG_KERNEL ,__FUNCTION__,j);
				
				if(CWMCU_bus_write(sensor, CwRegMapW_SET_FLUSH, data, 2)< 0){
					printk("%s:%s:(flush:bus write fail)\n",LOG_TAG_KERNEL ,__FUNCTION__);
			}
				printk("%s:%s:(flush:id:%d)\n",LOG_TAG_KERNEL ,__FUNCTION__, j);
				msleep(1);				
		}
	}
	}
    
	if(sensor->initial_hw_config){
		for(i = 0;i<HW_ID_END;i++){
			if(sensor->hw_info[i].hw_id !=HW_ID_END){
				data[0] = i;
				data[1] = sensor->hw_info[i].hw_id;
				data[2] = sensor->hw_info[i].deviceaddr;
				data[3] = sensor->hw_info[i].rate;
				data[4] = sensor->hw_info[i].mode;
				data[5] = sensor->hw_info[i].position;
				data[6] = sensor->hw_info[i].private_setting[0];
				data[7] = sensor->hw_info[i].private_setting[1];
				data[8] = sensor->hw_info[i].private_setting[2];
				data[9] = sensor->hw_info[i].private_setting[3];
				CWMCU_bus_write(sensor, CwRegMapRW_HW_SENSORS_CONFIG_START+i, data, 10);
				printk(KERN_DEBUG "CwMcu:%s id:%d, HwId:%u, deviceaddr:%u, mode:%u, position:%u\n"
					,__FUNCTION__
					, i
					,sensor->hw_info[i].hw_id
					,sensor->hw_info[i].deviceaddr
					,sensor->hw_info[i].mode
					,sensor->hw_info[i].position
				);
			}
		}
	}
		
    cwmcu_kernel_status(KERNEL_RESUND);
}
    
static void cwmcu_send_event_to_hal(uint8_t event, uint8_t data0, uint8_t data1)
{
    uint32_t data_event = 0;
    sensor->report_count++;
    data_event = ((u32)sensor->report_count << 24) |((u32)event << 16) | ((u32)data1 << 8) | (u32)data0;
    input_report_abs(sensor->input, CW_ABS_X, data_event);
    input_sync(sensor->input);
}

static void cwmcu_read_batch_buff(struct CWMCU_data *sensor)
{
	uint8_t data[20];
	uint16_t count = 0;
	uint32_t data_event[4] = {0};
	int i = 0;

	if (CWMCU_bus_read(sensor, CwRegMapR_BatchCount, data, 2) >= 0) {
		count = ((uint16_t)data[1] << 8) | (uint16_t)data[0];
//		printk("CwMcu:%s count %u\n",__FUNCTION__, count);
	} else {
		printk("CwMcu:%s check count failed~!!\n",__FUNCTION__);
        return;
	}
	for (i = 0; i < count; i++) {
		if (CWMCU_bus_read(sensor, CwRegMapR_BatchEvent, data, 9) >= 0) {
			/* check if there are no data from queue */
			if (data[0] != CWMCU_NODATA) {
				sensor->report_count++;
				if (data[0] == META_DATA) {
					data_event[1] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[2] << 8) | (u32)data[1];
					printk("CwMcu:%s META_DATA ,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",__FUNCTION__, 
						data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]
						);
					input_report_abs(sensor->input, CW_ABS_Z, data_event[1]);
					input_sync(sensor->input);
				} else if (data[0] == TimestampSync) {
					data_event[1] = ((u32)data[4] << 24) |  ((u32)data[3] << 16) | ((u32)data[2] << 8) | (u32)data[1];
					printk( "CwMcu:TimestampSync :%u\n",data_event[1]);
					input_report_abs(sensor->input, CW_ABS_TIMEBASE_WAKE_UP, data_event[1]);
					input_sync(sensor->input);
				} else if (data[0] == ACCURACY_UPDATE) {
					data_event[1] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[2] << 8) | (u32)data[1];
					printk( "CwMcu:TimestampSync :%u\n",data_event[1]);
					input_report_abs(sensor->input, CW_ABS_ACCURACY, data_event[1]);
					input_sync(sensor->input);
				} else {
					data_event[0] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[2] << 8) | (u32)data[1];
					data_event[1] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[4] << 8) | (u32)data[3];
					data_event[2] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[6] << 8) | (u32)data[5];
					data_event[3] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[8] << 8) | (u32)data[7];
                    
                    /* Calibration */
                    if (data[0] == ACCELERATION || data[0] == GYRO) {
                        data_event[0] += sensor->cali_data[data[0]].x;
                        data_event[1] += sensor->cali_data[data[0]].y;
                        data_event[2] += sensor->cali_data[data[0]].z;
                    }
                    
                    if (cw_data_log)
                        CW_DEBUG("read_batch_buff: 0x%x,ox%x,0x%x,0x%x", data_event[0], data_event[1], data_event[2], data_event[3]);
					/* check flush event */
					input_report_abs(sensor->input, CW_ABS_X, data_event[0]);
					input_report_abs(sensor->input, CW_ABS_Y, data_event[1]);
					input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
					input_report_abs(sensor->input, CW_ABS_TIMEDIFF_WAKE_UP, data_event[3]);
					input_sync(sensor->input);
				}
			}
		}
	}
	sensor->interrupt_check_count++;
}
static void cwmcu_read_stream_buff(struct CWMCU_data *sensor)
{
	uint8_t data[20];
	uint16_t count = 0;
	uint32_t data_event[4] = {0};
	int i = 0;
    static int pre_tilt = 0;

	if (CWMCU_bus_read(sensor, CwRegMapR_StreamCount, data, 2) >= 0) {
		count = ((uint16_t)data[1] << 8) | (uint16_t)data[0];
//		printk("CwMcu:%s count %u\n",__FUNCTION__, count);
	} else {
		printk("CwMcu:%s check count failed~!!\n",__FUNCTION__);
        return;
	}
	for (i = 0; i < count; i++) {
		if (CWMCU_bus_read(sensor, CwRegMapR_StreamEvent, data, 9) >= 0) {
            if (data[0] != TILT) pre_tilt = 0;
			/* check if there are no data from queue */
			if (data[0] != CWMCU_NODATA) {
				sensor->report_count++;
				if (data[0] == META_DATA) {
					data_event[1] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[2] << 8) | (u32)data[1];
					printk("CwMcu:%s META_DATA ,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",__FUNCTION__, 
						data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]
						);
					input_report_abs(sensor->input, CW_ABS_Z, data_event[1]);
					input_sync(sensor->input);
                    CWMCU_send_dummy_data(0);
				} else if (data[0] == TimestampSync) {
					data_event[1] = ((u32)data[4] << 24) |  ((u32)data[3] << 16) | ((u32)data[2] << 8) | (u32)data[1];
					printk( "CwMcu:TimestampSync :%u\n",data_event[1]);
					input_report_abs(sensor->input, CW_ABS_TIMEBASE, data_event[1]);
					input_sync(sensor->input);
                } else if (data[0] == ACCURACY_UPDATE) {
                    data_event[1] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[2] << 8) | (u32)data[1];
                    printk( "CwMcu:AccuracySync :%u\n",data_event[1]);					
                    input_report_abs(sensor->input, CW_ABS_ACCURACY, data_event[1]);
                    input_sync(sensor->input);
                }
                else if (data[0] == TILT) {
                    if (cw_tilt_wakeup_flag) {
                        if (data[5] == 1) {
                            pre_tilt = 1;
                        
                        }else if (pre_tilt == 1 && data[5] == 0){
                            pre_tilt = 0;
                            //TODO check data_event[2] bit 0
                            input_report_key(sensor->input, KEY_POWER, 1);
                            input_sync(sensor->input);
                            input_report_key(sensor->input, KEY_POWER, 0);
                            input_sync(sensor->input);
                        }    
                        input_report_abs(sensor->input, CW_ABS_X, 700);
					    input_report_abs(sensor->input, CW_ABS_Y, 650);
					    input_report_abs(sensor->input, CW_ABS_Z, 650);
					    input_sync(sensor->input);
        					/* check flush event */
                        CW_INFO("tilt wakeup(%d) %x,%x,%x,%x",pre_tilt, data_event[0], data_event[1], data_event[2], data_event[3]);
                    }
                }    
				else {
					data_event[0] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[2] << 8) | (u32)data[1];
					data_event[1] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[4] << 8) | (u32)data[3];
					data_event[2] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[6] << 8) | (u32)data[5];
					data_event[3] = ((u32)sensor->report_count << 24) |((u32)data[0] << 16) | ((u32)data[8] << 8) | (u32)data[7];

                    /* Calibration */
                    data_event[0] += sensor->cali_data[data[0]].x/10;
                    data_event[1] += sensor->cali_data[data[0]].y/10;
                    data_event[2] += sensor->cali_data[data[0]].z/10;
                    
                    if (cw_data_log)
                        CW_DEBUG("read_stream_buff: 0x%x,ox%x,0x%x,0x%x", data_event[0], data_event[1], data_event[2], data_event[3]);
					/* check flush event */
					input_report_abs(sensor->input, CW_ABS_X, data_event[0]);
					input_report_abs(sensor->input, CW_ABS_Y, data_event[1]);
					input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
					input_report_abs(sensor->input, CW_ABS_TIMEDIFF, data_event[3]);
					input_sync(sensor->input);
				}
			}
		}
	}
	sensor->interrupt_check_count++;
}

#define QueueSystemInfoMsgSize          30

static void ParserInfoFormQueue(char *data){
	static unsigned char loge_bufftemp[255];
	static unsigned char buff_counttemp = 0;
	int i;
	for(i=0;i<QueueSystemInfoMsgSize;i++){
		loge_bufftemp[buff_counttemp] = (unsigned char)data[i];
		buff_counttemp++;
		if(data[i] == '\n'){
			printk("CwMcuInfo:%s",loge_bufftemp);
			memset(loge_bufftemp,0x00,QueueSystemInfoMsgSize*2);
			buff_counttemp = 0;
		}
	}
}
static void cwmcu_read_mcu_info(struct CWMCU_data *sensor)
{
	uint8_t data[40];
	uint16_t count = 0;
	int i = 0;

	if (CWMCU_bus_read(sensor, CwRegMapR_SystemInfoMsgCount, data, 1) >= 0) {
		count = (uint16_t)data[0];
	} else {
		printk("CwMcu:%s check count failed~!!\n",__FUNCTION__);
        return;
	}
	for (i = 0; i < count; i++) {
		if (CWMCU_bus_read(sensor, CwRegMapR_SystemInfoMsgEvent, data, 30) >= 0) {
			ParserInfoFormQueue(data);
		}
	}

}


static void cwmcu_read_Interrupt_buff(struct CWMCU_data *sensor)
{
	uint8_t data[4];
	uint16_t count = 0;
	int i = 0;
	if (CWMCU_bus_read(sensor, CwRegMapR_InteruptCount, data, 1) >= 0) {
		count = (uint16_t)data[0];
	} else {
		printk("CwMcu:%s check count failed~!!\n",__FUNCTION__);
        return;
	}
	for (i = 0; i < count; i++) {
		if (CWMCU_bus_read(sensor, CwRegMapR_InteruptEvent, data, 4) >= 0) {
			if(data[0] == ERROR_MSG){
				switch ((int8_t)data[2] ) {
				case DRIVER_ENABLE_FAIL:
					printk("CwMcu:DRIVER_ENABLE_FAIL:ID:%u:Status:%u\n", data[1],data[3]);
					break;
				case DRIVER_DISABLE_FAIL:
					printk("CwMcu:DRIVER_DISABLE_FAIL:ID:%u:Status:%u\n", data[1],data[3]);
					break;
				case DRIVER_GETDATA_FAIL:
					printk("CwMcu:DRIVER_GETDATA_FAIL:ID:%u:Status:%u\n", data[1],data[3]);
					break;
				}
            }else if(data[0] == CALIBRATOR_UPDATE){
                #if 0
                if(data[1] == CALIBRATOR_TYPE_DEFAULT){
                    printk("CwMcu:Calibrator Id:%u,Status:%u\n", data[2],data[3]);
                    if(data[3] == CALIBRATOR_STATUS_PASS){                      
                        cwmcu_send_event_to_hal(CALIBRATOR_UPDATE,data[2],data[3]);
                    }
                #endif    
                }else if(data[1] == CALIBRATOR_TYPE_SELFTEST){                  
                        printk("CwMcu:Selftest Id:%u,Status:%u\n", data[2],data[3]);
                }        
            }else if(data[0] == MCU_ENABLE_LIST){
                if(data[1] == 0){
                    sensor->mcu_enable_list = (sensor->mcu_enable_list&0xFFFF0000) |(((uint32_t)data[3])<<8)|((uint32_t)data[2]);
                }else if(data[1] == 1){
                    sensor->mcu_enable_list = (sensor->mcu_enable_list&0x0000FFFF) |(((uint32_t)data[3])<<24)|(((uint32_t)data[2])<<16);
                }
                printk("CwMcu:%s:(McuEnableList:%u)\n",__FUNCTION__,sensor->mcu_enable_list);
            }else if(data[0] == MCU_HW_ENABLE_LIST){
                if(data[1] == 0){
                    sensor->mcu_hw_enable_list = (sensor->mcu_hw_enable_list&0xFFFF0000) |(((uint32_t)data[3])<<8)|((uint32_t)data[2]);
                }else if(data[1] == 1){
                    sensor->mcu_hw_enable_list = (sensor->mcu_hw_enable_list&0x0000FFFF) |(((uint32_t)data[3])<<24)|(((uint32_t)data[2])<<16);
                }
                printk("CwMcu:%s:(McuHwEnableList:%u)\n",__FUNCTION__,sensor->mcu_hw_enable_list);    
			}		
		}
	sensor->interrupt_check_count++;
}

static void cwmcu_interrupt_trigger(struct CWMCU_data *sensor)
{
	uint8_t temp[2] = {0};
	
	if (sensor->mcu_mode == CW_BOOT) {
		printk("--CWMCU--%s sensor->mcu_mode = %d\n", __func__, sensor->mcu_mode);
		return;
	}
	/* check mcu interrupt status */
	if (CWMCU_bus_read(sensor, CwRegMapR_InterruptStatus, temp, 2) >= 0) {
		sensor->interrupt_status = (u32)temp[1] << 8 | (u32)temp[0];
        if (cw_data_log)
        CW_DEBUG("==>INT:0x%x (en_list=0x%x)",sensor->interrupt_status, sensor->enabled_list);
	} else {
		printk("--CWMCU-- check interrupt_status failed~!!\n");
        return;
	}

	if (sensor->interrupt_status & (1<<INTERRUPT_INIT)) {
		cwmcu_reinit();
        cwmcu_send_event_to_hal(MCU_REINITIAL,0,0);
	}
	if (sensor->interrupt_status & (1<<INTERRUPT_TIMESYNC)) {
		cwmcu_send_event_to_hal(TimestampSync,0,0);
	}
	if ((sensor->interrupt_status & (1<<INTERRUPT_BATCHTIMEOUT)) || (sensor->interrupt_status & (1<<INTERRUPT_BATCHFULL)) ) {
        cwmcu_read_batch_buff(sensor);
    }
	if (sensor->interrupt_status & (1<<INTERRUPT_INFO)) {
		cwmcu_read_Interrupt_buff(sensor);
	}
	if (sensor->interrupt_status & (1<<INTERRUPT_DATAREADY)) {		
        cwmcu_read_stream_buff(sensor); 
    }
    if (sensor->interrupt_status & (1<<INTERRUPT_LOGE)) {       
        cwmcu_read_mcu_info(sensor);    
    }
}

#if 0

static int CWMCU_read(struct CWMCU_data *sensor)
{
	#ifndef CWMCU_INTERRUPT
	cwmcu_powermode_switch(SWITCH_POWER_NORMAL, 1);
	if(sensor->enabled_list)
		cwmcu_read_stream_buff(sensor);

	if(sensor->debug_log)
		cwmcu_read_mcu_info(sensor);	

	cwmcu_interrupt_trigger(sensor);
	
	cwmcu_powermode_switch(SWITCH_POWER_NORMAL, 0);
	#else
	if(sensor->debug_log){
		cwmcu_powermode_switch(SWITCH_POWER_NORMAL, 1);
		cwmcu_read_mcu_info(sensor);
		cwmcu_powermode_switch(SWITCH_POWER_NORMAL, 0);
	}
	#endif
		return 0;
	}
#else
    
static int CWMCU_read(struct CWMCU_data *sensor)
{
	if (sensor->mcu_mode == CW_BOOT) {
		return 0;
	}
#ifndef CWMCU_INTERRUPT
	cwmcu_powermode_switch(SWITCH_POWER_NORMAL, 1);
	if(sensor->kernel_status !=KERNEL_SUPEND) {
		cwmcu_kernel_status(KERNEL_RESUND);
		cwmcu_interrupt_trigger(sensor);
	}
	if(sensor->enabled_list)
		cwmcu_read_stream_buff(sensor);
	
	if(sensor->debug_log){
		cwmcu_powermode_switch(SWITCH_POWER_LOG, 1);
		cwmcu_read_mcu_info(sensor);
		cwmcu_powermode_switch(SWITCH_POWER_LOG, 0);
	}
	cwmcu_powermode_switch(SWITCH_POWER_NORMAL, 0);
#else
	if(sensor->debug_log){
		cwmcu_powermode_switch(SWITCH_POWER_LOG, 1);
		cwmcu_read_mcu_info(sensor);
		cwmcu_powermode_switch(SWITCH_POWER_LOG, 0);
	}
#endif

	return 0;
}
#endif
/*==========sysfs node=====================*/

static int active_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int enabled = 0;
	int id = 0;
	int handle = 0;
	uint32_t enabled_list = 0;
	uint32_t enabled_list_temp = 0;
	uint8_t data[5];
	int i = 0;

	sscanf(buf, "%d %d\n", &id, &enabled);
   
	handle = NonWakeUpHandle;
	sensor->sensors_info[handle][id].en = enabled;
	data[0] = handle;
	data[1] = id;
	if(enabled){
		sensor->enabled_list |= 1<<id;
		data[2] = (sensor->sensors_info[handle][id].rate ==0)?200:sensor->sensors_info[handle][id].rate;
		data[3] = (uint8_t)sensor->sensors_info[handle][id].timeout;
		data[4] = (uint8_t)(sensor->sensors_info[handle][id].timeout >>8);
	}else{
		sensor->enabled_list &= ~(1<<id);
		sensor->sensors_info[handle][id].rate = 0;
		sensor->sensors_info[handle][id].timeout = 0;
		data[2] = 0;
		data[3] = 0;
		data[4] = 0;
	}
	if (sensor->mcu_mode == CW_BOOT) {
		return count;
	}
	
	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 1);
	if(enabled){
		if(CWMCU_bus_write(sensor, CwRegMapW_SET_ENABLE, data, 5)< 0){
			printk("%s:%s:(bus Write Fail)\n",LOG_TAG_KERNEL ,__FUNCTION__);
		}
	}else{
		if(CWMCU_bus_write(sensor, CwRegMapW_SET_DISABLE, data, 5)< 0){
			printk("%s:%s:(bus Write Fail)\n",LOG_TAG_KERNEL ,__FUNCTION__);
		}
	}
	msleep(5);
	
	if (CWMCU_bus_read(sensor, CwRegMapR_EnableList, data, 4) < 0)
        return count;
	enabled_list = (uint32_t)data[3]<<24 |(uint32_t)data[2]<<16 |(uint32_t)data[1]<<8 |(uint32_t)data[0];
	if(enabled_list !=sensor->enabled_list){
		printk("%s:%s:(Enable not match AP:0x%x,MCU:0x%x)\n",LOG_TAG_KERNEL ,__FUNCTION__, sensor->enabled_list,enabled_list);
		enabled_list_temp = sensor->enabled_list ^ enabled_list;
		for(i=0;i<SENSORS_ID_END;i++){
			if (enabled_list_temp & (1<<i)) {
				data[0] = NonWakeUpHandle;
				data[1] = i;
				if(sensor->sensors_info[NonWakeUpHandle][i].en){
					data[2] = (sensor->sensors_info[NonWakeUpHandle][i].rate ==0)?200:sensor->sensors_info[NonWakeUpHandle][i].rate;
					data[3] = (uint8_t)sensor->sensors_info[NonWakeUpHandle][i].timeout;
					data[4] = (uint8_t)(sensor->sensors_info[NonWakeUpHandle][i].timeout >>8);
					CWMCU_bus_write(sensor, CwRegMapW_SET_ENABLE, data, 5);
					printk("%s:%s:(id:%d enable)\n",LOG_TAG_KERNEL ,__FUNCTION__,i);
        }else{
					data[2] = 0;
					data[3] = 0;
					data[4] = 0;
					if(CWMCU_bus_write(sensor, CwRegMapW_SET_DISABLE, data, 5)< 0){
						printk("%s:%s:(bus Write Fail)\n",LOG_TAG_KERNEL ,__FUNCTION__);
					}
				}
				msleep(5);
			}
        }
	}
	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 0);
	printk("%s:%s:(id:%d, en:%d)\n",LOG_TAG_KERNEL ,__FUNCTION__, id,enabled);
	return count;
}

static int active_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint8_t data[5];
    uint32_t mcu_enabled_list;
    
    cwmcu_powermode_switch(SWITCH_POWER_NORMAL, 1);
    CWMCU_bus_read(sensor, CwRegMapR_EnableList, data, 4);
    cwmcu_powermode_switch(SWITCH_POWER_NORMAL, 0);
	mcu_enabled_list = (uint32_t)data[3]<<24 |(uint32_t)data[2]<<16 |(uint32_t)data[1]<<8 |(uint32_t)data[0];

	return sprintf(buf, "0x%x (mcu:0x%x)\n", sensor->enabled_list, mcu_enabled_list);
}

static int interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(CWMCU_POLL_INTERVAL), "%d\n", CWMCU_POLL_INTERVAL);
}

static int interval_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static int batch_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int id = 0;
	int handle = 0;	
	int mode = -1;
	int rate = 0;
	int64_t timeout = 0;
	uint8_t data[5];

//	if(mode ==0)
	{
	sscanf(buf, "%d %d %d %lld\n", &id, &mode, &rate, &timeout);

	handle = NonWakeUpHandle;
		sensor->sensors_info[handle][id].rate = rate;
		sensor->sensors_info[handle][id].timeout = timeout;
		data[0] = handle;
		data[1] = id;
		data[2] = sensor->sensors_info[handle][id].rate;
		data[3] = (uint8_t)sensor->sensors_info[handle][id].timeout;
		data[4] = (uint8_t)(sensor->sensors_info[handle][id].timeout >>8);

		if (sensor->mcu_mode == CW_BOOT) {
			return count;
		}
		cwmcu_powermode_switch(SWITCH_POWER_BATCH, 1);
		if (CWMCU_bus_write(sensor, CwRegMapW_SET_ENABLE, data, 5)<0){
			printk("%s:%s:(Write Fail:id:%d, mode:%d, rate:%d, timeout:%lld)\n",LOG_TAG_KERNEL ,__FUNCTION__, id,mode, rate, timeout);
		}
		cwmcu_powermode_switch(SWITCH_POWER_BATCH, 0);
		printk("%s:%s:(id:%d, mode:%d, rate:%d, timeout:%lld)\n",LOG_TAG_KERNEL ,__FUNCTION__, id,mode, rate, timeout);
	}
	return count;
}

static int batch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(buf), "\n");
}

static int flush_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int id = 0;
	int error_msg = 0;
	uint8_t data[2];
	if (sensor->mcu_mode == CW_BOOT) {
		return count;
	}

	cwmcu_powermode_switch(SWITCH_POWER_BATCH, 1);

	sscanf(buf, "%d\n", &id);
	data[0] = NonWakeUpHandle;
	data[1] = (uint8_t)id;

	printk("CwMcu: flush id = %d~!!\n", id);

	error_msg = CWMCU_bus_write(sensor, CwRegMapW_SET_FLUSH, data, 2);

	if (error_msg < 0)
		printk("CwMcu: flush bus error~!!\n");
	cwmcu_powermode_switch(SWITCH_POWER_BATCH, 0);

	return count;
}

static int flush_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	CW_INFO("%s in", __func__);
	return  0;
}

static int CWMCU_Erase_Mcu_Memory(void)
{
	/* page should be 1~N */
	uint8_t send[300];
	uint8_t received[10];
	uint8_t XOR = 0;
	uint8_t page;
	uint16_t i = 0;
	page = 128;

	send[0] = 0x44;
	send[1] = 0xBB;
	if (CWMCU_bus_write_serial((uint8_t *)send, 2) < 0) {
		return -EAGAIN;
		}
	if (CWMCU_bus_read_serial((uint8_t *)received, 1) < 0) {
		return -EAGAIN;
		}
	if (received[0] != ACK) {
		return -EAGAIN;
		}

	send[0] = (uint8_t) ((page-1)>>8);
	send[1] = (uint8_t) (page-1);
	send[2] = send[0] ^ send[1];
	if (CWMCU_bus_write_serial((uint8_t *)send, 3) < 0) {
		return -EAGAIN;
		}
	if (CWMCU_bus_read_serial((uint8_t *)received, 1) < 0) {
		return -EAGAIN;
		}
	if (received[0] != ACK) {
		return -EAGAIN;
		}

	for (i = 0; i < page; i++) {
		send[2*i] = (uint8_t)(i>>8);
		send[(2*i)+1] = (uint8_t)i;
		XOR = XOR ^ send[2*i] ^ send[(2*i)+1];
	}
	send[(2*page)] = XOR;
	if (CWMCU_bus_write_serial((uint8_t *)send, ((2*page)+1)) < 0) {
		return -EAGAIN;
		}
	return 0;

}
static int CWMCU_Write_Mcu_Memory(const char *buf)
{
	uint8_t WriteMemoryCommand[2];
	uint8_t data[300];
	uint8_t received[10];
	uint8_t XOR = 0;
	uint16_t i = 0;
	WriteMemoryCommand[0] = 0x31;
	WriteMemoryCommand[1] = 0xCE;
	if (CWMCU_bus_write_serial((uint8_t *)WriteMemoryCommand, 2) < 0) {
		return -EAGAIN;
		}
	if (CWMCU_bus_read_serial((uint8_t *)received, 1) < 0) {
		return -EAGAIN;
		}
	if (received[0] != ACK) {
		return -EAGAIN;
		}

	/* Set Address + Checksum */
	data[0] = (uint8_t) (sensor->addr >> 24);
	data[1] = (uint8_t) (sensor->addr >> 16);
	data[2] = (uint8_t) (sensor->addr >> 8);
	data[3] = (uint8_t) sensor->addr;
	data[4] = data[0] ^ data[1] ^ data[2] ^ data[3];
	if (CWMCU_bus_write_serial((uint8_t *)data, 5) < 0) {
		return -EAGAIN;
		}
	if (CWMCU_bus_read_serial((uint8_t *)received, 1) < 0) {
		return -EAGAIN;
		}
	if (received[0] != ACK) {
		return -EAGAIN;
		}

	/* send data */
	data[0] = sensor->len - 1;
	XOR = sensor->len - 1;
	for (i = 0; i < sensor->len; i++) {
		data[i+1] = buf[i];
		XOR ^= buf[i];
	}
	data[sensor->len+1] = XOR;

	if (CWMCU_bus_write_serial((uint8_t *)data, (sensor->len + 2)) < 0) {
		return -EAGAIN;
		}
	return 0;
}

static CWMCU_send_dummy_data(uint32_t timestamp)
{
    uint32_t data_event[4] = {0x00100000, 0x00100000, 0x00100000, 0x00100000};    
    static int first_in = 1;

    if (first_in) {
    // for show step card
    //data_event[2] |= timestamp;
    input_report_abs(sensor->input, CW_ABS_X, data_event[0]);
    input_report_abs(sensor->input, CW_ABS_Y, data_event[1]);
    input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
    input_report_abs(sensor->input, CW_ABS_TIMEDIFF, data_event[3]);
    input_sync(sensor->input);
        first_in = 0;
    CW_DEBUG("CWMCU_send_dummy_data : send dummy pedometer data");
}
}

static int set_firmware_update_cmd(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u8 data[40] = {0};
	s16 data_buff[3] = {0};
	s16  _bData[8];
	s16  _bData2[9];
	s16  m_hdata[3];
	s16  m_asa[3];
	u8 test = 0x01;

	sscanf(buf, "%d %d %d\n", &sensor->cmd, &sensor->addr, &sensor->len);
	CW_INFO("CWMCU cmd=%d addr=%d len=%d", sensor->cmd, sensor->addr, sensor->len);

	cwmcu_powermode_switch(SWITCH_POWER_FIRMWARE_COMMAND, 1);

	switch (sensor->cmd) {
	case CHANGE_TO_BOOTLOADER_MODE:
			CW_INFO("CWMCU CHANGE_TO_BOOTLOADER_MODE");
            wake_lock(&cwmcu_wakelock);
            cw_fw_upgrade_status = 0;
			sensor->mcu_mode = CW_BOOT;
			/* boot enable : put high , reset: put low */
			mt_set_gpio_out(GPIO_CW_MCU_RESET, 1);
			mt_set_gpio_out(GPIO_CW_MCU_BOOT, 1);
			msleep(500);
			mt_set_gpio_out(GPIO_CW_MCU_RESET, 1);
			msleep(500);
			mt_set_gpio_out(GPIO_CW_MCU_RESET, 0);
			msleep(500);
			mt_set_gpio_out(GPIO_CW_MCU_RESET, 1);
			msleep(1000);

#if defined(CWMCU_I2C_INTERFACE)
			sensor->mcu_slave_addr = sensor->client->addr;
			sensor->client->addr = 0x72 >> 1;
#elif defined(CWMCU_SPI_INTERFACE)
            {
                int count = 20;
                u8 tbuf[3] = {0x0};
                u8 rbuf[10] = {0x0};

                tbuf[0] = 0x5A;
                if (CWMCU_bus_write_serial(tbuf, 1) < 0)
                {
                    CW_ERROR("F/W bootloader mode wrtie 0x5A failed");
                }
                    
                tbuf[0] = 0x0;

                while(count-- > 0) {
                    if (spi_rw_bytes_serial(tbuf, rbuf, 1) < 0) {
                        CW_ERROR("F/W bootloader mode read ACK failed");
                        continue;
                    }
                    
                    if (rbuf[0] == 0x79)
                    {
                        CW_INFO("F/W bootloader ACK is 0x79");
                        tbuf[0] = 0x79;
                        CWMCU_bus_write_serial(tbuf, 1);                            
			            break;
                    }

                    CW_INFO("F/W bootloader polling ACK... ");
                }    
                if (count <= 0)
                    CW_INFO("F/W bootloader ACK failed... %d", count);
            }
#endif
			break;

	case CHANGE_TO_NORMAL_MODE:
			CW_INFO("CWMCU CHANGE_TO_NORMAL_MODE");

			sensor->firmwave_update_status = 1;
#if defined(CWMCU_I2C_INTERFACE)
			sensor->client->addr = 0x74 >> 1;
#endif
			/* boot low  reset high */
			mt_set_gpio_out(GPIO_CW_MCU_BOOT, 0);
			mt_set_gpio_out(GPIO_CW_MCU_RESET, 1);
			msleep(500);
			mt_set_gpio_out(GPIO_CW_MCU_RESET, 0);
			msleep(500);
			mt_set_gpio_out(GPIO_CW_MCU_RESET, 1);
			msleep(1000);

			//mt_set_gpio_dir(GPIO_CW_MCU_RESET, GPIO_DIR_IN);

			sensor->mcu_mode = CW_NORMAL;
            cwmcu_reinit();
            cw_fw_upgrade_status = 1;
            wake_unlock(&cwmcu_wakelock);
			break;

	case ERASE_MCU_MEMORY:
			CW_INFO("CWMCU ERASE_MCU_MEMORY");
			sensor->firmwave_update_status = 1;
			sensor->firmwave_update_status = CWMCU_Erase_Mcu_Memory();
			break;

	case WRITE_MCU_MEMORY:
			CW_INFO("CWMCU Set Addr=%d\tlen=%d", sensor->addr, sensor->len);
			break;

	case MCU_GO:
			CW_INFO("CWMCU MCU_GO");
			break;

	case CHECK_FIRMWAVE_VERSION:
            CW_INFO("READ F/W VERSION, cw_fw_upgrade_status=%d", cw_fw_upgrade_status);
            if (CWMCU_bus_read(sensor, CwRegMapR_REPORT_CHIP_ID, data, 4) >= 0) 
            {
                CW_INFO( "CwMcu:CHECK_FIRMWAVE_VERSION:%u,%u,%u,%u\n", data[0],data[1],data[2],data[3]);
            }
            break;

   	case SET_DEBUG_LOG:			
            sensor->debug_log = sensor->addr;           
            break;        
	case SET_SYSTEM_COMMAND:
			data[0] = sensor->addr;
			data[1] = sensor->len;
			CWMCU_bus_write(sensor, CwRegMapW_SystemCommand, data, 2);
			break;
	case GET_SYSTEM_TIMESTAMP:
			if (CWMCU_bus_read(sensor, CwRegMapR_REPORT_SYSTEM_TIMESTAMP, data, 4) >= 0) {
				printk("Timestamp:%u\n",
					(((uint32_t)data[3])<<24) |(((uint32_t)data[2])<<16) |(((uint32_t)data[1])<<8) | ((uint32_t)data[0])
					);
			}
			break;
	case GET_FUNCTION_ENTRY_POINT:
			if (CWMCU_bus_read(sensor, CwRegMapR_FunctionEntryPoint, data, 4) >= 0) {
				printk("GET_FUNCTION_ENTRY_POINT:%u\n",
					(((uint32_t)data[3])<<24) |(((uint32_t)data[2])<<16) |(((uint32_t)data[1])<<8) | ((uint32_t)data[0])
					);
			}
			break;
	case GET_MCU_INFO:
			cwmcu_read_mcu_info(sensor);
			break;    
    case SET_TILT_WAKEUP:
            cw_tilt_wakeup_flag = 1;
            break;
	case SET_HW_INITIAL_CONFIG_FLAG:			
        sensor->initial_hw_config = sensor->addr;           
        break;  
    case SET_SENSORS_POSITION:          
        data[0] = sensor->addr;         
        data[1] = sensor->len;          
        CWMCU_bus_write(sensor, CwRegMapW_HW_SENSORS_POSITION, data, 2);            
        break;
	case SHOW_LOG_INFO:					
        printk("CwDbg:initial_hw_config%d\n",sensor->initial_hw_config);            
        printk("CwDbg:kernel_status%d\n",sensor->kernel_status);            
        printk("CwDbg:enabled_list%d\n",sensor->enabled_list);          
        printk("CwDbg:interrupt_status%d\n",sensor->interrupt_status);          
        printk("CwDbg:power_on_list%d\n",sensor->power_on_list);            
        printk("CwDbg:%d\n",sensor->power_on_list);         
        printk("CwDbg:%d\n",sensor->power_on_list);         
        break;        
    case GET_SENSORS_INDEX0:
    case GET_SENSORS_INDEX1:
    case GET_SENSORS_INDEX2:
    case GET_SENSORS_INDEX3:
    case GET_SENSORS_INDEX4:
    case GET_SENSORS_INDEX5:
    case GET_SENSORS_INDEX6:    
            CW_INFO( "CWMCU CHECK_ACC_DATA\n");
            if (CWMCU_bus_read(sensor, CwRegMapR_REPORT_DATA_START +sensor->cmd - GET_SENSORS_INDEX0, data, 6) >= 0) {
                data_buff[0] = (s16)(((u16)data[1] << 8) | (u16)data[0]);
                data_buff[1] = (s16)(((u16)data[3] << 8) | (u16)data[2]);
                data_buff[2] = (s16)(((u16)data[5] << 8) | (u16)data[4]);
        
                CW_INFO( "x = %d, y = %d, z = %d\n",
                data_buff[0], data_buff[1], data_buff[2]);
            }    
			break;
	default:
			break;
	}
	cwmcu_powermode_switch(SWITCH_POWER_FIRMWARE_COMMAND, 0);
	return count;
}

static int set_firmware_update_data(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	CW_INFO("CWMCU Write Data");
	CW_INFO("%s",buf);
	sensor->firmwave_update_status = 1;
	sensor->firmwave_update_status = CWMCU_Write_Mcu_Memory(buf);
	return count;
}

static int get_firmware_update_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	CW_INFO("CWMCU firmwave_update_status = %d", sensor->firmwave_update_status);
	return snprintf(buf, sizeof(buf), "%d\n", sensor->firmwave_update_status);
}

static int set_firmware_update(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
#if defined(CWMCU_I2C_INTERFACE)
	int intsize = sizeof(int);
	memcpy(&sensor->cw_bus_rw, buf, intsize);
	memcpy(&sensor->cw_bus_len, &buf[4], intsize);
	memcpy(sensor->cw_bus_data, &buf[8], sensor->cw_bus_len);

    //CW_DEBUG("[set_firmware_update_i2] RW=%d, LEN=%d, DATA=0x%x,0x%x", sensor->cw_bus_rw, sensor->cw_bus_len, sensor->cw_bus_data[0], sensor->cw_bus_data[1]);    
#elif defined(CWMCU_SPI_INTERFACE)
    int intsize = sizeof(int);
    memcpy(&sensor->cw_bus_rw, buf, intsize);
    memcpy(&sensor->cw_bus_len, &buf[4], intsize);

    if (sensor->cw_bus_rw)
    {
        if (sensor->cw_bus_len == 2)
        {
            sensor->cw_bus_len += 1;
            sensor->cw_bus_data[0] = 0x5A;
            memcpy(&sensor->cw_bus_data[1], &buf[8], sensor->cw_bus_len);
        }
        else
        {
            memcpy(sensor->cw_bus_data, &buf[8], sensor->cw_bus_len);
        }
    }    
    else
    {
        sensor->cw_bus_len = 3;
        sensor->cw_bus_data[0] = 0x00;
        sensor->cw_bus_data[1] = 0x00;
        sensor->cw_bus_data[2] = 0x00;
    }
    //CW_DEBUG("SET_FIRMWARE_UPDATE : rw=%d, len=%d, data=%x,%x,%x ", sensor->cw_bus_rw, sensor->cw_bus_len, sensor->cw_bus_data[0], sensor->cw_bus_data[1], sensor->cw_bus_data[2]);
#endif
	return count;
}

static int get_firmware_update(struct device *dev, struct device_attribute *attr, char *buf)
{
#if defined(CWMCU_I2C_INTERFACE)

	int status = 0;
	if (sensor->cw_bus_rw) {
		if (CWMCU_bus_write_serial(sensor->cw_bus_data, sensor->cw_bus_len) < 0) {
			status = -1;
		}
		memcpy(buf, &status, sizeof(int));
		return 4;
	} else {
		if (CWMCU_bus_read_serial(sensor->cw_bus_data, sensor->cw_bus_len) < 0) {
			status = -1;
			memcpy(buf, &status, sizeof(int));
			return 4;
		}
		memcpy(buf, &status, sizeof(int));
		memcpy(&buf[4], sensor->cw_bus_data, sensor->cw_bus_len);
		return 4+sensor->cw_bus_len;
	}
#elif defined(CWMCU_SPI_INTERFACE)

    int status = -1;
    if (sensor->cw_bus_rw) {
        if (CWMCU_bus_write_serial(sensor->cw_bus_data, sensor->cw_bus_len) < 0) {
			status = -1;
        }
        else
            status = 0;
			memcpy(buf, &status, sizeof(int));
			return 4;
    } else {
        u8 rbuf[5] = {0};
        int count = 30;

        while(count-- >= 0) {
            sensor->cw_bus_data[0] = 0;
            if (spi_rw_bytes_serial(sensor->cw_bus_data, rbuf, 1) < 0) {
                CW_ERROR("get_firmware_update:Read failed");
                continue;
		}
            if (rbuf[0] == 0x79) {
                //CW_INFO("get_firmware_update ACK is 0x79");
                buf[4] = 0x79;
                status = 0;
                   
		memcpy(buf, &status, sizeof(int));
                rbuf[0] = 0x79;
                CWMCU_bus_write_serial(rbuf, 1);  
                return 4+sensor->cw_bus_len;                 
            }
            //CW_DEBUG("get_firmware_update polling ACK... (0x%x)", rbuf[0]);
        }            

        status = -1;
        return 4;
	}
#endif    
	return  0;
}

static int mcu_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(buf), "%d\n", sensor->mcu_mode);
}

static int mcu_model_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int mode = 0;
	sscanf(buf, "%d\n", &mode);
	CWMCU_Set_Mcu_Mode(mode);
	return count;
}

/* get calibrator data */
static int get_calibrator_data(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t data[2] = {0};
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);
			printk("--CWMCU-- CWMCU_CALIBRATOR_STATUS\n");
	if (CWMCU_bus_read(sensor, CwRegMapR_CalibratorStatus, data, 2) >= 0) {
		printk("--CWMCU-- calibrator status = %d\n", data[1]);
			} else {
		printk("--CWMCU--  bus calibrator status read fail\n");
			}
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
	return sprintf(buf, "%d\n",data[1]);

}

static int set_calibrator_data(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	uint8_t data[33] = {0};
	int temp[33] = {0};

    if (sensor->mcu_mode == CW_BOOT) {
        return ;
    }

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);

	printk("--CWMCU-- buf = %s\n", buf);

		sscanf(buf, "%d %d %d",&temp[0], &temp[1], &temp[2]);
		sensor->cal_cmd = (uint8_t)temp[0];
		sensor->cal_type = (uint8_t)temp[1];
		sensor->cal_id = (uint8_t)temp[2];
		printk("--CWMCU-- cmd:%d type:%d id:%d\n", sensor->cal_cmd, sensor->cal_type, sensor->cal_id);
		if (sensor->cal_cmd == CWMCU_CALIBRATOR_INFO) {
			printk("--CWMCU-- set calibrator info\n");
			data[0] = sensor->cal_id;
			data[1] = sensor->cal_type;
			CWMCU_bus_write(sensor, CwRegMapW_CalibratorEnable, data, 2);
		}
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
	return count;
}

/* get calibrator data */
static int get_calibrator_data0(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	uint8_t data[33] = {0};

    if (sensor->mcu_mode == CW_BOOT) {
        return ;
    }    
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);
	printk("--CWMCU-- CWMCU_ACCELERATION_CALIBRATOR read data\n");
	if (CWMCU_bus_read(sensor, CwRegMapRW_Calibrator_Data_Acceleration , &data[3], 30) < 0) {
		printk(KERN_ERR "--CWMCU-- bus calibrator read fail!!! [ACC]\n");
		data[0] = 255;
	}else{
		for (i = 0; i < 33; i++) {
			printk("--CWMCU-- read data[%d] = %u\n", i, data[i]);
		}
	}
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
	return sprintf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]);

}

static int set_calibrator_data0(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int i = 0;
	uint8_t data[33] = {0};
	int temp[33] = {0};

    if (sensor->mcu_mode == CW_BOOT) {
        return ;
    }

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);

	sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			&temp[0], &temp[1], &temp[2],
			&temp[3], &temp[4], &temp[5], &temp[6], &temp[7], &temp[8], &temp[9], &temp[10], &temp[11], &temp[12],
			&temp[13], &temp[14], &temp[15], &temp[16], &temp[17], &temp[18], &temp[19], &temp[20], &temp[21], &temp[22],
			&temp[23], &temp[24], &temp[25], &temp[26], &temp[27], &temp[28], &temp[29], &temp[30], &temp[31], &temp[32]);

	for (i = 0; i < 33; i++) {
		data[i] = (uint8_t)temp[i];
	}
	printk("--CWMCU-- CW_ACCELERATION_CALIBRATOR write data\n");
	if (CWMCU_bus_write(sensor, CwRegMapRW_Calibrator_Data_Acceleration , &data[3], 30) >= 0) {
		printk("CwMcu:%s:(%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d)\n",__FUNCTION__,
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]
		);
	}else{
		printk("CwMcu:%s:(bus write fail)\n",__FUNCTION__);
	}

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
		return count;
	}


/* get calibrator data */
static int get_calibrator_data1(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	uint8_t data[33] = {0};

    if (sensor->mcu_mode == CW_BOOT) {
        return ;
    }
    
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);
	printk("--CWMCU-- CWMCU_MAGNETIC_CALIBRATOR read data\n");
	if (CWMCU_bus_read(sensor, CwRegMapRW_Calibrator_Data_Magnetioc , &data[3], 30) < 0) {
		printk(KERN_ERR "--CWMCU-- bus calibrator read fail!!! [MAG]\n");
		data[0] = 255;
	}else{
		printk("CWMCU:%s:(%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d)\n",__FUNCTION__,
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]
		);
	}
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
	return sprintf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]);
}

static int set_calibrator_data1(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int i = 0;
	uint8_t data[33] = {0};
	int temp[33] = {0};

    if (sensor->mcu_mode == CW_BOOT) {
        return ;
    }

    //TODO
    return;

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);

	sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			&temp[0], &temp[1], &temp[2],
			&temp[3], &temp[4], &temp[5], &temp[6], &temp[7], &temp[8], &temp[9], &temp[10], &temp[11], &temp[12],
			&temp[13], &temp[14], &temp[15], &temp[16], &temp[17], &temp[18], &temp[19], &temp[20], &temp[21], &temp[22],
			&temp[23], &temp[24], &temp[25], &temp[26], &temp[27], &temp[28], &temp[29], &temp[30], &temp[31], &temp[32]);

	for (i = 0; i < 33; i++) {
		data[i] = (uint8_t)temp[i];
	}
	printk("--CWMCU-- CWMCU_MAGNETIC_CALIBRATOR write data\n");
	if (CWMCU_bus_write(sensor, CwRegMapRW_Calibrator_Data_Magnetioc , &data[3], 30) >= 0) {
		printk("CwMcu:%s:(%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d)\n",__FUNCTION__,
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]
		);
	}else{
		printk("CwMcu:%s:(bus write fail)\n",__FUNCTION__);
	}

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
	return count;
}

/* get calibrator data */

static int get_calibrator_data2(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t data[33] = {0};

    if (sensor->mcu_mode == CW_BOOT) {
        return ;
    }
    
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);
	if (CWMCU_bus_read(sensor, CwRegMapRW_Calibrator_Data_Gyro , &data[3], 30) < 0) {
		printk("CwMcu:%s:(bus read fail)\n",__FUNCTION__);
		data[0] = 255;
	}else{
		printk("CwMcu:%s:(%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d)\n",__FUNCTION__,
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]
		);
	}
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
	return sprintf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]);

}

static int set_calibrator_data2(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int i = 0;
	uint8_t data[33] = {0};
	int temp[33] = {0};

    if (sensor->mcu_mode == CW_BOOT) {
        return ;
    }
    
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);

	sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			&temp[0], &temp[1], &temp[2],
			&temp[3], &temp[4], &temp[5], &temp[6], &temp[7], &temp[8], &temp[9], &temp[10], &temp[11], &temp[12],
			&temp[13], &temp[14], &temp[15], &temp[16], &temp[17], &temp[18], &temp[19], &temp[20], &temp[21], &temp[22],
			&temp[23], &temp[24], &temp[25], &temp[26], &temp[27], &temp[28], &temp[29], &temp[30], &temp[31], &temp[32]);

	for (i = 0; i < 33; i++) {
		data[i] = (uint8_t)temp[i];
	}
	if (CWMCU_bus_write(sensor, CwRegMapRW_Calibrator_Data_Gyro , &data[3], 30) >= 0) {
		printk("CwMcu:%s:(%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d)\n",__FUNCTION__,
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]
		);
	}else{
		printk("CwMcu:%s:(bus write fail)\n",__FUNCTION__);
	}
	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
	return count;
}



/* get calibrator data */
static int get_calibrator_data3(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static int set_calibrator_data3(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	return 0;
}

/* get calibrator data */
static int get_calibrator_data4(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static int set_calibrator_data4(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	return 0;
}
/* get calibrator data */
static int get_calibrator_data5(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static int set_calibrator_data5(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	return 0;
}

/* get calibrator data */
static int get_calibrator_data6(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static int set_calibrator_data6(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	return 0;
}

static int version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t data[4];
	int16_t version = -1;
	printk("--CWMCU-- %s\n", __func__);
	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 1);
	if (CWMCU_bus_read(sensor, CwRegMapR_REPORT_CHIP_ID, data, 4) >= 0) {
		printk(KERN_DEBUG "CHECK_FIRMWAVE_VERSION : M:%u,D:%u,V:%u,SV:%u\n", data[3], data[2], data[1], data[0]);
		version = (int16_t)( ((uint16_t)data[1])<<8 | (uint16_t)data[0]);
	}else{
		printk(KERN_DEBUG "CHECK_FIRMWAVE_VERSION Fail\n");
	}
	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 0);
	return snprintf(buf, sizeof(buf), "%d\n", version);
}

static int timestamp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t data[20];
	uint32_t timestamp = 0, timebase = 0, timebasew = 0;
	cwmcu_powermode_switch(SWITCH_POWER_TIME, 1);
	if (CWMCU_bus_read(sensor, CwRegMapR_REPORT_SYSTEM_TIMESTAMP, data, 12) >= 0) {
		timestamp = ((uint32_t)data[3])<<24 | ((uint32_t)data[2])<<16 | ((uint32_t)data[1])<<8 | ((uint32_t)data[0]);
		timebase = ((uint32_t)data[7])<<24 | ((uint32_t)data[6])<<16 | ((uint32_t)data[5])<<8 | ((uint32_t)data[4]);
		timebasew = ((uint32_t)data[11])<<24 | ((uint32_t)data[10])<<16 | ((uint32_t)data[9])<<8 | ((uint32_t)data[8]);
		CW_INFO("%s:( Time:%d,Base:%d,Basew:%d)\n",__FUNCTION__, timestamp,timebase,timebasew);
		buf[12] = 0;
		memcpy(buf, data, 12);
	}else{
		buf[12] = 255;
	}
	cwmcu_powermode_switch(SWITCH_POWER_TIME, 0);
	return 13;
}


static DEVICE_ATTR(enable, 0660, active_show, active_set);
static DEVICE_ATTR(delay_ms, 0660, interval_show, interval_set);
/* static DEVICE_ATTR(poll, 0660, poll_show, NULL); */
static DEVICE_ATTR(batch, 0660, batch_show, batch_set);
static DEVICE_ATTR(flush, 0660, flush_show, flush_set);
static DEVICE_ATTR(mcu_mode, 0660, mcu_mode_show, mcu_model_set);
static DEVICE_ATTR(calibrator_cmd, 0660, get_calibrator_data, set_calibrator_data);
static DEVICE_ATTR(calibrator_cmd0, 0660, get_calibrator_data0, set_calibrator_data0);
static DEVICE_ATTR(calibrator_cmd1, 0660, get_calibrator_data1, set_calibrator_data1);
static DEVICE_ATTR(calibrator_cmd2, 0660, get_calibrator_data2, set_calibrator_data2);
static DEVICE_ATTR(calibrator_cmd3, 0660, get_calibrator_data3, set_calibrator_data3);
static DEVICE_ATTR(calibrator_cmd4, 0660, get_calibrator_data4, set_calibrator_data4);
static DEVICE_ATTR(calibrator_cmd5, 0660, get_calibrator_data5, set_calibrator_data5);
static DEVICE_ATTR(calibrator_cmd6, 0660, get_calibrator_data6, set_calibrator_data6);
static DEVICE_ATTR(firmware_update_i2c, 0660, get_firmware_update, set_firmware_update);
static DEVICE_ATTR(firmware_update_cmd, 0660, NULL, set_firmware_update_cmd);
static DEVICE_ATTR(firmware_update_data, 0660, NULL, set_firmware_update_data);
static DEVICE_ATTR(firmware_update_status, 0660, get_firmware_update_status, NULL);
static DEVICE_ATTR(version, 0660, version_show, NULL);
static DEVICE_ATTR(timestamp, 0660, timestamp_show, NULL);


static struct attribute *sysfs_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay_ms.attr,
	/* &dev_attr_poll.attr, */
	&dev_attr_batch.attr,
	&dev_attr_flush.attr,
	&dev_attr_calibrator_cmd.attr,
	&dev_attr_calibrator_cmd0.attr,
	&dev_attr_calibrator_cmd1.attr,
	&dev_attr_calibrator_cmd2.attr,
	&dev_attr_calibrator_cmd3.attr,
	&dev_attr_calibrator_cmd4.attr,
	&dev_attr_calibrator_cmd5.attr,
	&dev_attr_calibrator_cmd6.attr,
	&dev_attr_mcu_mode.attr,
	&dev_attr_firmware_update_i2c.attr,
	&dev_attr_firmware_update_cmd.attr,
	&dev_attr_firmware_update_data.attr,
	&dev_attr_firmware_update_status.attr,
	&dev_attr_version.attr,	
	&dev_attr_timestamp.attr,
	NULL
};

static struct attribute_group sysfs_attribute_group = {
	.attrs = sysfs_attributes
};
/*=======factory mode=========*/
void factory_active_sensor(int sensor_id, int enabled)
{	
    int error_msg = 0;
    uint8_t data[5];
    int handle = NonWakeUpHandle;
    static u32 fac_enabled_list = 0;

#if 0
    if (sensor_id == HEARTBEAT)
    {
        if (enabled)
        {
            hwPowerOn(MT6323_POWER_LDO_VGP3, VOL_1800, "sensorhub");
            hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_3300, "sensorhub");
        }
        else
        {
            hwPowerDown(MT6323_POWER_LDO_VGP3, "sensorhub");
            hwPowerDown(MT6323_POWER_LDO_VGP1, "sensorhub");
        }
    }
#endif
    printk("factory_active_sensor ==> entry %d, %d, %d\n", sensor->mcu_mode, sensor_id, enabled);
    if (sensor->mcu_mode == CW_BOOT) {
        return ;
    }
    cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 1);
    if(enabled){
        mt_eint_mask(CUST_EINT_SENSORHUB_NUM);
        fac_enabled_list |= 1<<sensor_id;
        sensor->enabled_list |= 1<<sensor_id;
        data[0] = NonWakeUpHandle;
        data[1] = sensor_id;
        data[2] = 40;
        data[3] = 0;
        data[4] = 0;
        error_msg = CWMCU_bus_write(sensor, CwRegMapW_SET_ENABLE, data, 5);
        if (error_msg < 0)
            CW_ERROR("CwMcu: active bus error (%d)~!!\n", sensor_id);
    }else{
        fac_enabled_list &= ~(1<<sensor_id);
        sensor->enabled_list &= ~(1<<sensor_id);
        sensor->sensors_info[handle][sensor_id].rate = 0;
        sensor->sensors_info[handle][sensor_id].timeout = 0;
        data[0] = NonWakeUpHandle;
        data[1] = sensor_id;
        data[2] = 0;
        data[3] = 0;
        data[4] = 0;
        error_msg = CWMCU_bus_write(sensor, CwRegMapW_SET_DISABLE, data, 5);
        if (error_msg < 0)
            CW_ERROR("CwMcu: de-active bus error (%d)~!!\n", sensor_id);
        if (!fac_enabled_list)
            mt_eint_unmask(CUST_EINT_SENSORHUB_NUM);
    }
    cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 0);
    printk("CwMcu:%s id:%d, en:%d\n",__FUNCTION__, sensor_id, enabled);


}

int factory_data_read(int sensor_id, int data_event[])
{	
	int id_check = 0;
    int ret = 0;;
	uint8_t data[9] = {0};
    int retry = 0;

    CW_DEBUG("factory_data_read ==> entry %d, %d\n", sensor->mcu_mode, sensor_id);

	if (sensor->mcu_mode == CW_BOOT) {
		/* it will not get data if status is bootloader mode */
		return -1;
	}

    cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);

	if (CWMCU_bus_read(sensor, CwRegMapR_REPORT_DATA_START+sensor_id, data, 6) >= 0) 
	{
    
		data_event[0] = (s16)(data[1] << 8 | data[0] + sensor->cali_data[sensor_id].x/10);
		data_event[1] = (s16)(data[3] << 8 | data[2] + sensor->cali_data[sensor_id].y/10);
		data_event[2] = (s16)(data[5] << 8 | data[4] + sensor->cali_data[sensor_id].z/10);
		CW_DEBUG("factory_data_read: sensor(%d) -> x = %d, y = %d, z = %d"
							, sensor_id	, data_event[0], data_event[1], data_event[2]);
	} 
	else 
	{
		CW_ERROR("factory read error 0x%x~!!!", sensor_id);
		ret = -1;
	}	

    cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
	return ret;
}

void factory_get_calibrator_data(int sensor_id, SENSOR_DATA *cali_data)
{
    cali_data->x = sensor->cali_data[sensor_id].x;
    cali_data->y = sensor->cali_data[sensor_id].y;
    cali_data->z = sensor->cali_data[sensor_id].z;
    CW_DEBUG("[factory_get_calibrator_data]cali[%d]=(%d,%d,%d\)", sensor_id, cali_data->x, cali_data->y, cali_data->z);
	}

void factory_clear_calibrator_data(int sensor_id)
{
    sensor->cali_data[sensor_id].x = 0;
    sensor->cali_data[sensor_id].y = 0;
    sensor->cali_data[sensor_id].z = 0;
			}

int factory_set_calibrator_data(int sensor_id, SENSOR_DATA *cali_data)
{
    if (!cali_data){
        CW_ERROR("[%s] no sensor data", __FUNCTION__);
		return -1;
	}

    if (sensor_id == ACCELERATION)
    {   
        // Pre-process accel data, correct reversed orientation on facotry calibration.
        if (cali_data->z > 10*1000){
            #define LIBHWM_GRAVITY_EARTH            (9806) 
            int abs_orig_caliz, new_caliz;
            CW_DEBUG("[factory_set_calibrator_data] original calibrator is (%d,%d,%d)", cali_data->x, cali_data->y, cali_data->z);
            abs_orig_caliz = cali_data->z - LIBHWM_GRAVITY_EARTH;
            cali_data->z = abs_orig_caliz - LIBHWM_GRAVITY_EARTH;            
	}
		}

    sensor->cali_data[sensor_id].x += cali_data->x;
    sensor->cali_data[sensor_id].y += cali_data->y;
    sensor->cali_data[sensor_id].z += cali_data->z;

    CW_DEBUG("[factory_set_calibrator_data] final calibrator id=%d(%d,%d,%d)", sensor_id, cali_data->x, cali_data->y, cali_data->z);

    return 0;
				}

/*=======input device==========*/

static void CWMCU_init_input_device(struct CWMCU_data *sensor, struct input_dev *idev)
{
	idev->name = CWMCU_NAME;
#if defined(CWMCU_I2C_INTERFACE)
	idev->id.bustype = BUS_I2C;
	idev->dev.parent = &sensor->client->dev;
#elif defined(CWMCU_SPI_INTERFACE)
    idev->id.bustype = BUS_SPI;
    idev->dev.parent = &sensor->spi->dev;
#endif        
	idev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_ABS);
	set_bit(EV_KEY, idev->evbit);

	/* send mouse event */
	set_bit(BTN_MOUSE, idev->keybit);
	set_bit(EV_REL, idev->evbit);
	set_bit(REL_X, idev->relbit);
	set_bit(REL_Y, idev->relbit);
	set_bit(EV_MSC, idev->evbit);
	set_bit(MSC_SCAN, idev->mscbit);
	set_bit(BTN_LEFT, idev->keybit);
	set_bit(BTN_RIGHT, idev->keybit);

	input_set_capability(idev, EV_KEY, 116);
	input_set_capability(idev, EV_KEY, 102);
    input_set_capability(idev, EV_KEY, KEY_POWER);

	/*
	input_set_capability(idev, EV_KEY, 88);
	*/
	set_bit(EV_ABS, idev->evbit);
	input_set_abs_params(idev, CW_ABS_X, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_Y, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_Z, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_X1, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_Y1, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_Z1, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_TIMEDIFF, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_TIMEDIFF_WAKE_UP, -DPS_MAX, DPS_MAX, 0, 0);    
	input_set_abs_params(idev, CW_ABS_ACCURACY, -DPS_MAX, DPS_MAX, 0, 0);
    input_set_abs_params(idev, CW_ABS_TIMEBASE, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_TIMEBASE_WAKE_UP, -DPS_MAX, DPS_MAX, 0, 0);	
	input_set_abs_params(idev, REL_X, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, REL_Y, -DPS_MAX, DPS_MAX, 0, 0);

}

/*=======polling device=========*/
static void CWMCU_poll(struct input_polled_dev *dev)
{
	CWMCU_read(dev->private);
}

static int CWMCU_open(struct CWMCU_data *sensor)
{
	int error;
#if defined(CWMCU_I2C_INTERFACE)     
	error = pm_runtime_get_sync(&sensor->client->dev);
#elif defined(CWMCU_SPI_INTERFACE)
    error = pm_runtime_get_sync(&sensor->spi->dev);
#endif

	if (error && error != -ENOSYS)
		return error;
	return 0;
}

static void CWMCU_close(struct CWMCU_data *sensor)
{
#if defined(CWMCU_I2C_INTERFACE)
	pm_runtime_put_sync(&sensor->client->dev);
#elif defined(CWMCU_SPI_INTERFACE)
	pm_runtime_put_sync(&sensor->spi->dev);
#endif
}

static void CWMCU_poll_open(struct input_polled_dev *ipoll_dev)
{
	struct CWMCU_data *sensor = ipoll_dev->private;
	CWMCU_open(sensor);
}

static void CWMCU_poll_close(struct input_polled_dev *ipoll_dev)
{
	struct CWMCU_data *sensor = ipoll_dev->private;
	CWMCU_close(sensor);
}

static int CWMCU_register_input_device(struct CWMCU_data *sensor)
{
#if 0 //input Poll device 
	int error = -1;
  
	struct input_polled_dev *ipoll_dev;

	/* poll device */
	ipoll_dev = input_allocate_polled_device();
	if (!ipoll_dev)
		return -ENOMEM;

	ipoll_dev->private = sensor;
	ipoll_dev->open = CWMCU_poll_open;
	ipoll_dev->close = CWMCU_poll_close;
	ipoll_dev->poll = CWMCU_poll;
	ipoll_dev->poll_interval = CWMCU_POLL_INTERVAL;
	ipoll_dev->poll_interval_min = CWMCU_POLL_MIN;
	ipoll_dev->poll_interval_max = CWMCU_POLL_MAX;

	CWMCU_init_input_device(sensor, ipoll_dev->input);

	error = input_register_polled_device(ipoll_dev);
	if (error) {
		input_free_polled_device(ipoll_dev);
		return error;
	}

	sensor->input_polled = ipoll_dev;
	sensor->input = ipoll_dev->input;
#else
    sensor->input = input_allocate_device();
    if(!sensor->input)
        CW_ERROR("alloc input_dev error!");

    CWMCU_init_input_device(sensor, sensor->input);
    if (input_register_device(sensor->input))
        CW_ERROR("input_register_device failed.(cwmcu)\n");
        
#endif
	return 0;
}

static void cwmcu_resume_work(struct work_struct *work)
{
    char cmd[10];

	cwmcu_powermode_switch(SWITCH_POWER_PROBE, 1);
	cwmcu_kernel_status(KERNEL_RESUND);

    if (cw_tilt_wakeup_flag) {
        //de-activate tilt  wakeup
        sprintf(cmd, "%d %d\n", TILT, 0);
        active_set(NULL, NULL, cmd, sizeof(cmd));    
        CW_INFO("resume => disable tilt");
    }
	cwmcu_powermode_switch(SWITCH_POWER_PROBE, 0);

    CW_DEBUG("%s:end", __FUNCTION__);
}

void CWMCU_suspend(struct device *dev)
{
	return 0;
}

static int CWMCU_early_suspend(struct early_suspend *h)
{
    char cmd[10];

    if (cw_tilt_wakeup_flag) {
        //activate tilt  wakeup
        sprintf(cmd, "%d %d\n", TILT, 1);
        active_set(NULL, NULL, cmd, sizeof(cmd));
        CW_INFO("suspend => enable tilt");
    }        

    cwmcu_powermode_switch(SWITCH_POWER_PROBE, 1);
	cwmcu_kernel_status(KERNEL_SUPEND);
	cwmcu_powermode_switch(SWITCH_POWER_PROBE, 0);

    sensor->cw_suspend_flag= 1;
	return 0;
}

void CWMCU_resume(struct device *dev)
{
	return 0;
}

static int CWMCU_late_resume(struct early_suspend *h)
{
    schedule_work(&sensor->resume_work);
   	return 0;
}

#ifdef CWMCU_INTERRUPT
static void CWMCU_interrupt_thread(void)
{
	//mt_eint_mask(CUST_EINT_SENSORHUB_NUM);

	if (sensor->mcu_mode == CW_BOOT) {
		//CW_INFO("%s sensor->mcu_mode = %d", __func__, sensor->mcu_mode);
		mt_eint_unmask(CUST_EINT_SENSORHUB_NUM);
		return;
	}
	schedule_work(&sensor->work);

    //mt_eint_unmask(CUST_EINT_SENSORHUB_NUM);
	return;
}

static void cwmcu_work_report(struct work_struct *work)
{
	if (sensor->mcu_mode == CW_BOOT) {
		printk("CwMcu:%s sensor->mcu_mode = %d\n", __func__, sensor->mcu_mode);
        mt_eint_unmask(CUST_EINT_SENSORHUB_NUM);
		return;
	}
	cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 1);
	cwmcu_interrupt_trigger(sensor);
	cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 0);
	//printk("CwMcu:%s\n", __func__);
	mt_eint_unmask(CUST_EINT_SENSORHUB_NUM);
}

#endif

static void cwmcu_hw_config_init(struct CWMCU_data *sensor)
{
	int i = 0;
    SensorsInit_T	accel_hw_info = ACCEL_HW_INFO;
    SensorsInit_T	gyro_hw_info = GYRO_HW_INFO;
    
    sensor->initial_hw_config = 0;  
    sensor->enabled_list = 0;   
    sensor->interrupt_status = 0;   
    sensor->power_on_list = 0;  
    sensor->cal_cmd = 0;    
    sensor->cal_type = 0;   
    sensor->cal_id = 0; 
    sensor->mcu_enable_list = 0;	
    sensor->mcu_hw_enable_list = 0;
    for(i = 0;i<DRIVER_ID_END;i++){
		sensor->hw_info[i].hw_id=HW_ID_END;
	}
#if 0
	i = ACCELERATION;
	sensor->hw_info[i].hw_id 			= BMI055;
	sensor->hw_info[i].deviceaddr		= 0x30;
	sensor->hw_info[i].rate 			= 20;
	sensor->hw_info[i].mode 			= 7;
	sensor->hw_info[i].position 		= 6;
	sensor->hw_info[i].private_setting[0]	= 2;
	sensor->hw_info[i].private_setting[1]	= 4;
	sensor->hw_info[i].private_setting[2]	= 1;
	sensor->hw_info[i].private_setting[3]	= 0x0B;

	i = GYRO;
	sensor->hw_info[i].hw_id 			= BMI055;
	sensor->hw_info[i].deviceaddr		= 0xD0;
	sensor->hw_info[i].rate 			= 20;
	sensor->hw_info[i].mode 			= 8;
	sensor->hw_info[i].position 		= 6;
	sensor->hw_info[i].private_setting[0]	= 0;
	sensor->hw_info[i].private_setting[1]	= 0;
	sensor->hw_info[i].private_setting[2]	= 0;
	sensor->hw_info[i].private_setting[3]	= 0;
#else
    sensor->hw_info[ACCELERATION] = accel_hw_info;
    sensor->hw_info[GYRO] = gyro_hw_info;
#endif
}

int CWMCU_probe(void *bus_dev)
{
	int error;
	int i = 0;

	CW_INFO("%s", __func__);

    //VGP1
    //hwPowerOn(MT6323_POWER_LDO_VGP3, VOL_1800, "sensorhub");
    //hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_3300, "sensorhub");


	//initialize
	mt_set_gpio_mode(GPIO_CW_MCU_BOOT, GPIO_SENSORHUB_HOST_BOOT_ROM_M_GPIO);
	mt_set_gpio_dir(GPIO_CW_MCU_BOOT, GPIO_DIR_OUT);
	mt_set_gpio_mode(GPIO_CW_MCU_RESET, GPIO_SENSORHUB_HOST_RESET_M_GPIO);
	mt_set_gpio_dir(GPIO_CW_MCU_RESET, GPIO_DIR_OUT);
	mt_set_gpio_mode(GPIO_CW_MCU_WAKE_UP, GPIO_SENSORHUB_WAKE_UP_M_GPIO);
	mt_set_gpio_dir(GPIO_CW_MCU_WAKE_UP, GPIO_DIR_OUT);

	/* mcu reset */
	mt_set_gpio_out(GPIO_CW_MCU_RESET, 1);
	mt_set_gpio_out(GPIO_CW_MCU_BOOT, 1);
	msleep(100);
	mt_set_gpio_out(GPIO_CW_MCU_BOOT, 0);
	mt_set_gpio_out(GPIO_CW_MCU_RESET, 1);
	msleep(100);
	mt_set_gpio_out(GPIO_CW_MCU_RESET, 0);
	msleep(100);
	mt_set_gpio_out(GPIO_CW_MCU_RESET, 1);
	msleep(100);

	wake_lock_init(&cwmcu_wakelock, WAKE_LOCK_SUSPEND, "cwmcu suspend wakelock");

	//mt_set_gpio_dir(GPIO_CW_MCU_RESET, GPIO_DIR_IN);

	sensor = kzalloc(sizeof(struct CWMCU_data), GFP_KERNEL);
	if (!sensor) {
		CW_INFO("kzalloc error");
		return -ENOMEM;
	}


    CWMCU_bus_init(bus_dev);

	error = CWMCU_register_input_device(sensor);
	if (error) {
		CW_ERROR("CWMCU_register_input_device error");
		goto err_free_mem;
	}

	error = sysfs_create_group(&sensor->input->dev.kobj,
					&sysfs_attribute_group);
	if (error)
		goto exit_free_input;

	cwmcu_hw_config_init(sensor);

	cwmcu_powermode_switch(SWITCH_POWER_PROBE, 1);
	cwmcu_kernel_status(KERNEL_PROBE);
	cwmcu_powermode_switch(SWITCH_POWER_PROBE, 0);    
#ifdef CWMCU_INTERRUPT

	CW_INFO("--CWMCU--sensor->irq  =%d~!!", CUST_EINT_SENSORHUB_NUM);

	if (CUST_EINT_SENSORHUB_NUM > 0) {
		mt_set_gpio_mode(GPIO_SENSORHUB_EINT_PIN, GPIO_SENSORHUB_EINT_PIN_M_EINT);
		mt_set_gpio_dir(GPIO_SENSORHUB_EINT_PIN, GPIO_DIR_IN);
		mt_set_gpio_pull_enable(GPIO_SENSORHUB_EINT_PIN, GPIO_PULL_ENABLE);
		mt_set_gpio_pull_select(GPIO_SENSORHUB_EINT_PIN, 1);
		
		//mt_eint_set_sens(GPIO_SENSORHUB_EINT, MT_EDGE_SENSITIVE);
		//mt_eint_set_hw_debounce(GPIO_SENSORHUB_EINT, CUST_EINT_SENSORHUB_DEBOUNCE_CN);
		mt_eint_registration(CUST_EINT_SENSORHUB_NUM, EINTF_TRIGGER_FALLING, CWMCU_interrupt_thread, 0);

		mt_eint_mask(CUST_EINT_SENSORHUB_NUM);
		INIT_WORK(&sensor->work, cwmcu_work_report);
		mt_eint_unmask(CUST_EINT_SENSORHUB_NUM);
	}
#endif

    INIT_WORK(&sensor->resume_work, cwmcu_resume_work);
	//factory
	init_factory_node();
    for(i = 0; i<SENSORS_ID_END; i++) 
        factory_clear_calibrator_data(i);


    sensor->mcu_mode = CW_NORMAL;

	CW_INFO("CWMCU_probe success!");

	return 0;

exit_free_input:
	input_free_device(sensor->input);
err_free_mem:
exit_destroy_mutex:
	free_irq(CUST_EINT_SENSORHUB_NUM, sensor);
	kfree(sensor);
	return error;
}

static int __init CWMCU_init(void){

    int ret = 0;
	CW_INFO("CWMCU_init");

    cw_early_suspend_handler.suspend = CWMCU_early_suspend;
    cw_early_suspend_handler.resume = CWMCU_late_resume;
    register_early_suspend(&cw_early_suspend_handler);
    
	ret = CWMCU_bus_register();
    if (ret <0) {
        CW_ERROR("CWMCU_init bus register error");
	}	
    return ret;
}

static void __exit CWMCU_exit(void){
    //unregister_early_suspend(&cw_early_suspend_handler);
	 CWMCU_bus_unregister();
}

module_init(CWMCU_init);
module_exit(CWMCU_exit);

MODULE_DESCRIPTION("CWMCU Bus Driver");
MODULE_AUTHOR("CyWee Group Ltd.");
MODULE_LICENSE("GPL");
