/* CWMCU.h - header file for CyWee digital 3-axis gyroscope
 *
 * Copyright (C) 2010 CyWee Group Ltd.
 * Author: Joe Wei <joewei@cywee.com>
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
#ifndef __CWMCUSENSOR_H__
#define __CWMCUSENSOR_H__
#include <linux/ioctl.h>
#include <linux/sensors_io.h>

#define CWMCU_NAME "CwMcuSensor"
#define LOG_TAG_KERNEL "CwMcuKernel"
#define LOG_TAG_MCU "CwMcu"

typedef enum {
	NonWakeUpHandle= 0,
	WakeUpHandle=1,
	HANDLE_ID_END
} HANDLE_ID;

typedef enum {
	ACCELERATION			=0
	,MAGNETIC
	,GYRO
	,LIGHT
	,PROXIMITY
	,PRESSURE
	,HEARTBEAT
	,ORIENTATION
	,ROTATIONVECTOR
	,LINEARACCELERATION
	,GRAVITY
	,MAGNETIC_UNCALIBRATED
	,GYROSCOPE_UNCALIBRATED
	,GAME_ROTATION_VECTOR
	,GEOMAGNETIC_ROTATION_VECTOR
	,STEP_DETECTOR
	,STEP_COUNTER
	,SIGNIFICANT_MOTION
	,TILT
	,SENSORS_ID_END
} SENSORS_ID;

#define 	DRIVER_ID_END 	HEARTBEAT + 1

typedef enum {
	TimestampSync = SENSORS_ID_END+1
	,FLASH_DATA
	,META_DATA
	,MAGNETIC_UNCALIBRATED_BIAS
	,GYRO_UNCALIBRATED_BIAS
	,ERROR_MSG
	,BATCH_TIMEOUT
	,BATCH_FULL
	,ACCURACY_UPDATE
	,CALIBRATOR_UPDATE	
	,MCU_REINITIAL
	,MCU_ENABLE_LIST	
	,MCU_HW_ENABLE_LIST
}MCU_TO_CPU_EVENT_TYPE;

typedef enum {
	L3GD20					= 1,		//gyro
	LSM303DLHC				= 2,		//acc + mag
	LSM330					= 3,		//acc + gyro
	LPS331AP				= 4,		//pressure
	BMP280					= 5,		//pressure
	AKM8963				= 6,		//mag
	YAS53x					= 7,		//mag
	BMI055					= 8,		//acc + gyro
	AGD						= 9,		//acc + gyro
	AMI						= 10,	//mag
	LSM303D				= 11,	//acc + mag
	CM36283				= 12,
	APDS9960				= 13,
	LSM6DS0				= 14,  	 //acc + gyro
	AKM09911				= 15, 	//mag
	BMX055					= 16,	//acc + gyro + mag
	BMA250E				= 17, 	//acc
	BMI160					= 18, 	//acc + gyro
	CUSTOMDRIVER			= 19, 	//custom driver
	PAH8001				= 20, 	//HEARTBEAT
	CM36671				= 21, 	//Proximity
	HW_ID_END,
} HW_ID;

enum ABS_status {
	CW_ABS_X = 0x01,
	CW_ABS_Y,
	CW_ABS_Z,
	CW_ABS_X1,
	CW_ABS_Y1,
	CW_ABS_Z1,
	CW_ABS_TIMEDIFF,
	CW_ABS_TIMEDIFF_WAKE_UP,	
	CW_ABS_ACCURACY,
	CW_ABS_TIMEBASE,
	CW_ABS_TIMEBASE_WAKE_UP
};

enum MCU_mode {
	CW_NORMAL = 0x00,
	CW_SLEEP,
	CW_NO_SLEEP,
	CW_BOOT
};

/* power manager status */
typedef enum {
	SWITCH_POWER_ENABLE     = 0,
	SWITCH_POWER_DELAY,
	SWITCH_POWER_BATCH,
	SWITCH_POWER_NORMAL,
	SWITCH_POWER_CALIB,
	SWITCH_POWER_INTERRUPT,
	SWITCH_POWER_PROBE,
	SWITCH_POWER_LOG,
	SWITCH_POWER_FIRMWARE_COMMAND,	
	SWITCH_POWER_TIME	
} SWITCH_POWER_ID;

typedef enum {
	DRIVER_ENABLE_FAIL		= -7,
	DRIVER_DISABLE_FAIL		= -6,
	DRIVER_GETDATA_FAIL		= -5,
	I2C_FAIL				= -4,
	DRIVER_NO_USE			= -3,
	SENSORS_NO_INITIAL	= -2,
	FAIL					= -1,
	NO_ERROR				= 0,
	NO_DATA				= 1
} ERR_MSG;

/* interrupt status */
typedef enum {
	INTERRUPT_NON   = 0,
	INTERRUPT_INIT  = 1,
	INTERRUPT_GESTURE = 2,
	INTERRUPT_BATCHTIMEOUT   = 3,
	INTERRUPT_BATCHFULL   = 4,
	INTERRUPT_INFO = 5,
	INTERRUPT_DATAREADY   = 6,
	INTERRUPT_TIMESYNC   = 7,	
	INTERRUPT_LOGE   = 8
} INTERRUPT_STATUS_LIST;

typedef enum {
	KERNEL_NON   = 0
	,KERNEL_PROBE
	,KERNEL_SUPEND
	,KERNEL_RESUND
} KERNEL_STATUS;

typedef enum {
	CALIBRATOR_TYPE_NON = 0,
	CALIBRATOR_TYPE_DEFAULT = 1,
	CALIBRATOR_TYPE_SELFTEST = 2
} CALIBRATOR_TYPE;

typedef enum {
	CALIBRATOR_STATUS_OUT_OF_RANGE= -2,
	CALIBRATOR_STATUS_FAIL= -1,
	CALIBRATOR_STATUS_NON = 0,
	CALIBRATOR_STATUS_INPROCESS = 1,
	CALIBRATOR_STATUS_PASS = 2,
} CALIBRATOR_STATUS;

/* calibrator command */
typedef enum {
	CWMCU_ACCELERATION_CALIBRATOR = 0,
	CWMCU_MAGNETIC_CALIBRATOR,
	CWMCU_GYRO_CALIBRATOR,
	CWMCU_LIGHT_CALIBRATOR,
	CWMCU_PROXIMITY_CALIBRATOR,
	CWMCU_PRESSURE_CALIBRATOR,
	CWMCU_CALIBRATOR_STATUS = 6,
	CWMCU_CALIBRATOR_INFO = 7
} CALIBRATOR_CMD;

/* firmware update command */
typedef enum {
	CHANGE_TO_BOOTLOADER_MODE	= 1,
	ERASE_MCU_MEMORY,
	WRITE_MCU_MEMORY,
	MCU_GO,
	CHANGE_TO_NORMAL_MODE		= 5,
	CHECK_FIRMWAVE_VERSION,
	SET_DEBUG_LOG,
    SET_SYSTEM_COMMAND,
    GET_SYSTEM_TIMESTAMP,
    GET_FUNCTION_ENTRY_POINT = 10,
    GET_MCU_INFO,
	SET_HW_INITIAL_CONFIG_FLAG,	
	SET_SENSORS_POSITION,
	SHOW_LOG_INFO,
    SET_TILT_WAKEUP,
    GET_SENSORS_INDEX0 = 20,
	GET_SENSORS_INDEX1,
	GET_SENSORS_INDEX2,
	GET_SENSORS_INDEX3,
	GET_SENSORS_INDEX4,
	GET_SENSORS_INDEX5,	
	GET_SENSORS_INDEX6,
} FIRMWARE_CMD;

typedef enum {	
	NonSystemComand= 0
	,WatchDogReset=1
	,SystemNoSleep=2
	,ShowMcuInfo=3	
	,SystemShowLoge=4
	,SystemCommandIndexEnd
} SYSTEM_COMMAND_INDEX;

typedef enum {
	//5Byte: 0:handle;1:id;2:Rate;[3:4]:timeout ms;
	CwRegMapW_SET_ENABLE=0x01,
	//5Byte: 0:handle;1:id;2:Rate;[3:4]:timeout ms;
	CwRegMapW_SET_DISABLE=0x02,
	//2Byte: 0:handle;1:id;
	CwRegMapW_SET_FLUSH=0x04,
	//2Byte(uint16_t)
	//4Byte: enable list;	
	CwRegMapR_EnableList=0x05,	
	CwRegMapR_InterruptStatus=0x0F,
	/**
	*	10byte
	*	byte 0: sensors id
	*	byte 1: HwId
	*	byte 2: deviceaddr
	*	byte 3: rate
	*	byte 4: mode
	*	byte 5: position
	*	byte 6: private_setting[0]
	*	byte 7: private_setting[1]
	*	byte 8: private_setting[2]
	*	byte 9: private_setting[3]
	*/
	CwRegMapRW_HW_SENSORS_CONFIG_START	= 0x10 +ACCELERATION,
	CwRegMapRW_HW_SENSORS_CONFIG_END		= 0x10 +DRIVER_ID_END,
	/**
	*	2byte
	*	byte 0: sensors id
	*	byte 1: position
	*/
	CwRegMapW_HW_SENSORS_POSITION			= 0x1F,
	/**	
	*	2byte
	*	count
        *   byte 0: count   L
        *   byte 1: count   H
	*
	*	9byte
	*	event
	*	byte 0: sensors id
        *   byte 1: sensors X   L
        *   byte 2: sensors X   H
        *   byte 3: sensors Y   L
        *   byte 4: sensors Y   H
        *   byte 5: sensors Z   L
        *   byte 6: sensors Z   H
        *   byte 7: timestamp   L
        *   byte 8: timestamp   H
	*/

	CwRegMapR_StreamCount=0x20,
	CwRegMapR_StreamEvent=0x21,
	CwRegMapR_BatchCount=0x22,
	CwRegMapR_BatchEvent=0x23,
	
	/**
	*	1byte
	*	count
	*
	*	4byte
	*	byte 0: Type
	*	byte 1: id
	*	byte 2: error message (int8_t)
	*	byte 3: error status (int8_t)
	*/
	CwRegMapR_InteruptCount=0x24,
	CwRegMapR_InteruptEvent=0x25,
	
	/**
	*	2byte
	*	count
	*
	*	30byte
	*	Log
	*/
	CwRegMapR_SystemInfoMsgCount			= 0x26,
	CwRegMapR_SystemInfoMsgEvent			= 0x27,

	/**
	*	2byte
	*	byte 0: Sensors Id
	*	byte 1: Sensors Type
	*
	*	wbyte
	*	Calibrator Status
	*	byte 0: Sensors Id
	*	byte 1: Sensors Status
	*/
	CwRegMapW_CalibratorEnable			= 0x40,
	CwRegMapR_CalibratorStatus				= 0x41,

	CwRegMapRW_Calibrator_Data_Acceleration 	=0x43,
	CwRegMapRW_Calibrator_Data_Magnetioc 		=0x44,
	CwRegMapRW_Calibrator_Data_Gyro 			=0x45,
	CwRegMapRW_Calibrator_Data_Light 			=0x46,
	CwRegMapRW_Calibrator_Data_Proximity 		=0x47,
	CwRegMapRW_Calibrator_Data_Pressure 		=0x48,

	/**
	*	1byte
	*/
	CwRegMapW_SystemCommand			            = 0x5A,
	CwRegMapW_KERNEL_STATUS				= 0x5B,

	/**
	*	6byte
	*	data
	*/
	CwRegMapR_REPORT_DATA_START	= 0x60 +ACCELERATION,		//ACCELERATION
	CwRegMapR_REPORT_DATA_END	= 0x60 +SENSORS_ID_END,		//ACCELERATION
	
	
	/**
	*	4byte
	*	data[0] = BuildSubVersion;	
	*	data[1] = BuildVersion;	
	*	data[2] = BuildDay;	
	*	data[3] = BuildMonth;
	*	data
	*/
	CwRegMapR_REPORT_CHIP_ID						= 0x80,
	CwRegMapR_REPORT_SYSTEM_TIMESTAMP			= 0x81,

    /** *   64byte  *   data    */  
    CwRegMapR_REPORT_MAG_LOG                    = 0x8A,

	CwRegMapW_TIMESTAMP_SYNC					= 0xA0,
	CwRegMapR_FunctionEntryPoint					= 0xA1,
	CW_MAX_REG
} CwRegisterMapIndex;
/* check data of queue if queue is empty */

struct CWMCU_SENSORS_INFO{
	uint8_t en;    
	uint8_t mode;
	uint8_t rate;
	uint16_t timeout;
};

typedef struct {
        uint8_t hw_id;
        uint8_t deviceaddr;
        uint8_t     rate;
        uint8_t     mode;                       //default:  MODE_BYPASS 
        uint8_t position;   
        uint8_t private_setting[4];         //private_setting[2] = INTERRUPT_SETTING
    }SensorsInit_T;


struct CWMCU_data {
	struct i2c_client *client;
    struct spi_device *spi;
	struct regulator *vdd;
	struct regulator *vcc_i2c;
	struct input_polled_dev *input_polled;
	struct input_dev *input;
	struct workqueue_struct *driver_wq;
	struct work_struct work;
    struct work_struct resume_work;
	struct CWMCU_SENSORS_INFO sensors_info[HANDLE_ID_END][SENSORS_ID_END];
    SensorsInit_T	hw_info[DRIVER_ID_END];	
    uint8_t initial_hw_config;	
    
	int mcu_mode;
    uint8_t kernel_status;

	/* enable & batch list */
	uint32_t enabled_list;
    uint32_t interrupt_status;

	/* Mcu site enable list*/	
    uint32_t mcu_enable_list;	
    uint32_t mcu_hw_enable_list;

	/* power status */
    volatile    uint32_t power_on_list;
    int interrupt_check_count;

	/* Calibrator status */
	uint8_t cal_cmd;
	uint8_t cal_type;
	uint8_t cal_id;

	/* gpio */
	int irq_gpio;
	int wakeup_gpio;

    uint32_t debug_log;
    
	int cmd;
	uint32_t addr;
	int len;
	int mcu_slave_addr;
	int firmwave_update_status;
	int cw_bus_rw;	/* r = 0 , w = 1 */
	int cw_bus_len;
	uint8_t cw_bus_data[300];
    uint8_t report_count;
    int cw_suspend_flag;

    /*calibration*/
    SENSOR_DATA cali_data[SENSORS_ID_END];
};
#define CWMCU_NODATA	0xff

#define DPS_MAX			(1 << (16 - 1))
#ifdef __KERNEL__

#endif /* __KERNEL */

#define CW_INFO(fmt, arg...)         pr_warn("-CWMCU_INF- "fmt"\n", ##arg)
#define CW_ERROR(fmt, arg...)        pr_error("-CWMCU_ERR- "fmt"\n", ##arg)
#define CW_DEBUG(fmt, arg...)		pr_debug("-CWMCU_DBG- "fmt"\n", ##arg)

extern int CWMCU_bus_register(void);
extern void CWMCU_bus_unregister(void);
extern int CWMCU_bus_write_serial(u8 *data, int len);
extern int CWMCU_bus_read_serial(u8 *data, int len);
extern int CWMCU_bus_read(struct CWMCU_data *sensor, u8 reg_addr, u8 *data, u8 len);
extern int CWMCU_bus_write(struct CWMCU_data *sensor, u8 reg_addr, u8 *data, u8 len);
extern void CWMCU_bus_init(void *bus_dev);
extern int CWMCU_probe(void *bus_dev);
extern void CWMCU_suspend(struct device *dev);
extern void CWMCU_resume(struct device *dev);
extern void factory_active_sensor(int sensor_id, int enabled);
extern int factory_data_read(int sensor_id, int data_event[]);
extern int factory_set_calibrator_data(int sensor_id, SENSOR_DATA *cali_data);
extern void factory_clear_calibrator_data(int sensor_id);
extern void factory_get_calibrator_data(int sensor_id, SENSOR_DATA *cali_data);
extern void init_factory_node();
extern int spi_rw_bytes_serial(u8 *wbuf, u8 *rbuf, u8 len);
#endif /* __CWMCUSENSOR_H__ */
