/*
 * Copyright (C) 2012 Invensense, Inc.
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

#ifndef ICM30628_H
#define ICM30628_H

#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>	
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/crc32.h>
#include <linux/crypto.h>
#include <linux/fs.h>
#include <linux/kfifo.h>
#include <linux/hwmsensor.h>
#include <asm/siginfo.h>
#include <linux/rcupdate.h>
//#include <asm/gpio.h>
#include <linux/wakelock.h>

#define MTK_PLATFORM //MUST define before header file 
#include "icm30628_common.h"
#include "icm30628_math.h"
#include "icm30628_i2c.h"
#include "icm30628_spi.h"
#include "icm30628_debug.h"
#include "icm30628_interrupt.h"
#include "icm30628_power.h"
#include "icm30628_factory.h"
#include "garnet_definitions.h"
#include "mtkSensorPlatform.h"
//#define MTK_TESTTEST


#define FIRMWARE_VERSION_2_2_0 220
#define FIRMWARE_VERSION_2_4_0 240
#define FIRMWARE_VERSION_3_0_0 300
#define FIRMWARE_VERSION_3_2_0 320
#define FIRMWARE_VERSION_3_2_2 322
#define FIRMWARE_VERSION FIRMWARE_VERSION_3_2_2

#define CHIP_REV_2_1 3063027
#define CHIP_REV_2_7 3063027
#define CHIP_REV_3_1 3063031
#define CHIP_REV_3_2 3063032
#define CHIP_REV CHIP_REV_3_2

#define MTK_EINT_ENABLE
//#define MTK_EINT_WAKE_ENABLE
//define POLLING_MODE

#define ENABLE_LP_EN
#define HOST_LINEAR_GRAVITY_ORIENTATION
#define USING_WORKQUEUE_FOR_SENDING_COMMAND
#if (FIRMWARE_VERSION >= FIRMWARE_VERSION_2_2_0)
#define PIXART_LIBRARY_IN_FIRMWARE
#endif
#if (FIRMWARE_VERSION >= FIRMWARE_VERSION_3_0_0)
#define DMP_DOWNLOAD
#define ENABLE_DEEP_SLEEP
#endif
#if (FIRMWARE_VERSION >= FIRMWARE_VERSION_3_2_0)
#define M0_ORIENTATION_MATRIX
#endif

//#define I2C_INTERFACE
#define SPI_INTERFACE
#if !defined(I2C_INTERFACE) && !defined(SPI_INTERFACE)
#error at least one of interfaces required
#endif
//#define PERFORMACE
#ifdef PERFORMACE
#define LIKELY likely
#define UNLIKELY unlikely
#else
#define LIKELY 
#define UNLIKELY 
#endif

#define MODULE_NAME	"icm30628"
#define ICM30628_DRV	"/dev/"MODULE_NAME
#define ICM30628_I2C_ADDRESS 	0x6B

#if (FIRMWARE_VERSION >= FIRMWARE_VERSION_3_0_0)
#define COMMAND_DELAY 1
#else
#ifdef USING_WORKQUEUE_FOR_SENDING_COMMAND
#define COMMAND_DELAY 50
#else
#define COMMAND_DELAY 10
#endif
#endif

#define MAX_READ_SIZE 128
#define INV_DUMMY_SENSOR_ID 0x00
#define INV_WAKEUP_SENSOR_FLAG 0x80

#define GARNET_SYSTEM_CLOCK_UNIT 3051757812ULL

enum sensor_id
{
	INV_META_DATA = 0,
	INV_ACCELEROMETER = 1,
	INV_MAGNETIC_FIELD = 2,
	INV_ORIENTATION = 3,
	INV_GYROSCOPE = 4,
	INV_LIGHT = 5,
	INV_PRESSURE =6,
	INV_TEMPERATURE = 7, 
	INV_PROXIMITY = 8,
	INV_GRAVITY = 9,
	INV_LINEAR_ACCELERATION = 10,
	INV_ROTATION_VECTOR = 11,
	INV_RELATIVE_HUMIDITY = 12,
	INV_AMBIENT_TEMPERATURE = 13,
	INV_MAGNETIC_FIELD_UNCALIBRATED = 14,
	INV_GAME_ROTATION_VECTOR = 15,
	INV_GYROSCOPE_UNCALIBRATED = 16,
	INV_SIGNIFICANT_MOTION = 17,
	INV_STEP_DETECTOR = 18,
	INV_STEP_COUNTER = 19,
	INV_GEOMAGNETIC_ROTATION_VECTOR = 20,
	INV_HEART_RATE = 21,
	INV_TILT_DETECTOR = 22, 
	INV_WAKE_GESTURE = 23,
	INV_GLANCE_GESTURE = 24,
	INV_PICKUP_GESTURE = 25,
	INV_ACTIVITY_CLASSIFICATION = 31,
	INV_BRING_TO_SEE = 39,
	INV_SCREEN_ROTATION = 37, //TBD
	INV_PERFORM_SELFTEST = 254,
	INV_PLATFORM_SETUP = 255,
	INV_SENSOR_INVALID = 256
};

#define INV_SHAKE INV_WAKE_GESTURE

enum sensor_num
{
	INV_META_DATA_NUM = 0,
	INV_ACCELEROMETER_NUM,
	INV_MAGNETIC_FIELD_NUM,
	INV_ORIENTATION_NUM,
	INV_GYROSCOPE_NUM,
	INV_LIGHT_NUM,
	INV_PRESSURE_NUM,
	INV_TEMPERATURE_NUM, 
	INV_PROXIMITY_NUM,
	INV_GRAVITY_NUM,
	INV_LINEAR_ACCELERATION_NUM,
	INV_ROTATION_VECTOR_NUM,
	INV_RELATIVE_HUMIDITY_NUM,
	INV_AMBIENT_TEMPERATURE_NUM,
	INV_MAGNETIC_FIELD_UNCALIBRATED_NUM,
	INV_GAME_ROTATION_VECTOR_NUM,
	INV_GYROSCOPE_UNCALIBRATED_NUM,
	INV_SIGNIFICANT_MOTION_NUM,
	INV_STEP_DETECTOR_NUM,
	INV_STEP_COUNTER_NUM,
	INV_GEOMAGNETIC_ROTATION_VECTOR_NUM,
	INV_HEART_RATE_NUM,
	INV_SHAKE_NUM,
	INV_BRING_TO_SEE_NUM,
	INV_ACTIVITY_CLASSIFICATION_NUM,
	INV_SCREEN_ROTATION_NUM,
	INV_PERFORM_SELFTEST_NUM,
	INV_PLATFORM_SETUP_NUM,
	INV_SENSOR_MAX_NUM,
	INV_SENSOR_INVALID_NUM,
};

enum batch_sensor_num
{
	INV_BATCH_META_DATA_NUM = INV_SENSOR_INVALID_NUM + 1,
	INV_BATCH_ACCELEROMETER_NUM,
	INV_BATCH_GEOMAGNETIC_FIELD_NUM,
	INV_BATCH_ORIENTATION_NUM,
	INV_BATCH_GYROSCOPE_NUM,
	INV_BATCH_LIGHT_NUM,
	INV_BATCH_PRESSURE_NUM,
	INV_BATCH_TEMPERATURE_NUM, 
	INV_BATCH_PROXIMITY_NUM,
	INV_BATCH_GRAVITY_NUM,
	INV_BATCH_LINEAR_ACCELERATION_NUM,
	INV_BATCH_ROTATION_VECTOR_NUM,
	INV_BATCH_RELATIVE_HUMIDITY_NUM,
	INV_BATCH_AMBIENT_TEMPERATURE_NUM,
	INV_BATCH_MAGNETIC_FIELD_UNCALIBRATED_NUM,
	INV_BATCH_GAME_ROTATION_VECTOR_NUM,
	INV_BATCH_GYROSCOPE_UNCALIBRATED_NUM,
	INV_BATCH_SIGNIFICANT_MOTION_NUM,
	INV_BATCH_STEP_DETECTOR_NUM,
	INV_BATCH_STEP_COUNTER_NUM,
	INV_BATCH_GEOMAGNETIC_ROTATION_VECTOR_NUM,
	INV_BATCH_HEART_RATE_NUM,
	INV_BATCH_SHAKE_NUM,
	INV_BATCH_BRING_TO_SEE_NUM,
	INV_BATCH_ACTIVITY_CLASSIFICATION_NUM,
	INV_BATCH_SCREEN_ROTATION_NUM,	
	INV_BATCH_SENSOR_HUB_NUM,
	INV_BATCH_PERFORM_SELFTEST_NUM,
	INV_BATCH_PLATFORM_SETUP_NUM,
	INV_BATCH_SENSOR_MAX_NUM,
	INV_BATCH_SENSOR_INVALID_NUM,
};

enum command
{
	INV_CMD_SENSOR_OFF = 0,
	INV_CMD_SENSOR_ON = 1,
	INV_CMD_POWER = 2,
	INV_CMD_BATCH = 3,
	INV_CMD_FLUSH = 4,
	INV_CMD_SET_DELAY = 5,
	INV_CMD_SET_CALIBRATION_GAINS= 6,
	INV_CMD_GET_CALIBRATION_GAINS = 7,
	INV_CMD_SET_CALIBRATION_OFFSETS = 8,
	INV_CMD_GET_CALIBRATION_OFFSETS = 9,
	INV_CMD_SET_REFERENCE_FRAME = 10,
	INV_CMD_GET_FIRMWARE_INFO = 11,
	INV_CMD_GET_DATA = 12,
	INV_CMD_GET_CLOCK_RATE = 13,
	INV_CMD_PING = 14,
	INV_CMD_RESET = 15,
#ifdef DMP_DOWNLOAD
	INV_CMD_LOAD = 16, 
#endif	
	INV_CMD_INVALID
};

#ifdef DMP_DOWNLOAD
enum FifoProtocolLoadWho
{
	FIFOPROTOCOL_LOAD_WHO_RESERVED					= 0,
	FIFOPROTOCOL_LOAD_WHO_DMP3_FW					= 1,
	FIFOPROTOCOL_LOAD_WHO_DMP4_FW					= 2,
	FIFOPROTOCOL_LOAD_WHO_MAX,
};

enum FifoProtocolLoadWhat
{
	FIFOPROTOCOL_LOAD_WHAT_RESERVED					= 0,
	FIFOPROTOCOL_LOAD_WHAT_MALLOC					= 1,
	FIFOPROTOCOL_LOAD_WHAT_FREE						= 2,
	FIFOPROTOCOL_LOAD_WHAT_CHECK					= 3,
	FIFOPROTOCOL_LOAD_WHAT_MAX,
};
#endif

enum acuracy
{
	INV_UNRELIABLE = 0,
	INV_ACCURACY_LOW = 1,
	INV_ACCURACY_MEDIUM = 2,
	INV_ACCURACY_HIGH = 3,
};

enum status
{
	INV_STATUS_UPDATED = 0,
	INV_STATUS_CHANGED = 1,
	INV_STATUS_FLUSH = 2,
	INV_STATUS_POLL = 3
};

enum power_status
{
	INV_POWER_SUSPEND = 0,
	INV_POWER_ON = 1,
	INV_POWER_IDLE = 2,
};

enum data_type
{
	INV_DATA_TYPE_INTEGER = 0,
	INV_DATA_TYPE_FLOAT = 1,
};

#ifdef DMP_DOWNLOAD
enum dmp_type
{
	INV_DMP_TYPE_DMP3 = 0,
	INV_DMP_TYPE_DMP4 = 1,
};
#endif

enum
{
	INV_DATA_BAC_ACTIVITY_UNKNOWN			= 0,
	INV_DATA_BAC_ACTIVITY_IN_VEHICLE_START	= 0x01,
	INV_DATA_BAC_ACTIVITY_IN_VEHICLE_END		= 0x81,
	INV_DATA_BAC_ACTIVITY_WALKING_START 	= 0x02,
	INV_DATA_BAC_ACTIVITY_WALKING_END		= 0x82,
	INV_DATA_BAC_ACTIVITY_RUNNING_START 	= 0x03,
	INV_DATA_BAC_ACTIVITY_RUNNING_END		= 0x83,
	INV_DATA_BAC_ACTIVITY_ON_BICYCLE_START	= 0x04,
	INV_DATA_BAC_ACTIVITY_ON_BICYCLE_END	= 0x84,
	INV_DATA_BAC_ACTIVITY_TILT_START			= 0x05,
	INV_DATA_BAC_ACTIVITY_TILT_END			= 0x85,
	INV_DATA_BAC_ACTIVITY_STILL_START		= 0x06,
	INV_DATA_BAC_ACTIVITY_STILL_END 			= 0x86,
};

#define IN_VEHICLE		0x0100
#define WALKING		0x0200
#define RUNNING		0x0400
#define ON_BICYCLE	0x0800
#define TILT			0x1000
#define STILL			0x2000
#define UNKNOWN		0x8000

#define PACKET_SIZE_PER_HEADER_BYTE				(2)
#define PACKET_SIZE_PER_OFFSET_BYTE				(6)
#define PACKET_SIZE_PER_ACCURACY_BYTE			(2)
#define PACKET_SIZE_PER_TIMESTAMP_BYTE			(4)
#define PACKET_SIZE_PER_EVENT_SENSOR			(0)
#define PACKET_SIZE_PER_1AXIS_SENSOR				(2)
#define PACKET_SIZE_PER_3AXIS_SENSOR				(6)
#define PACKET_SIZE_PER_UNCALIBRATED_SENSOR	(PACKET_SIZE_PER_3AXIS_SENSOR + PACKET_SIZE_PER_OFFSET_BYTE)
#define PACKET_SIZE_PER_QUAT_SENSOR				(8 + PACKET_SIZE_PER_OFFSET_BYTE)
#define MAXIMUM_DATA_SIZE 							PACKET_SIZE_PER_QUAT_SENSOR
#define INVENSENSE_KFIFO_LENGTH 					4096
#define INVENSENSE_COMMAND_KFIFO_LENGTH 		512
#define INVENSENSE_INPUT_FIFO_LENGTH 			127
#if (FIRMWARE_VERSION >= FIRMWARE_VERSION_3_2_2)
#define INVENSENSE_OUTPUT_FIFO_LENGTH 			2047
#else
#define INVENSENSE_OUTPUT_FIFO_LENGTH 			4095
#endif

#define CACHE_SIZE_1AXIS		4
#define CACHE_SIZE_3AXIS		12
#define CACHE_SIZE_QUAT		16

#define AXIS_NUM 3
#define QUAT_ELEMENT_NUM 4

#define SIZE_HDR			2
#define SIZE_STAMP		4
#define SIZE_1AXIS			4
#define SIZE_1AXIS_16BIT	2
#define SIZE_3AXIS			6
#define SIZE_QUAT			16
#define SIZE_STEP_COUNTER	4
#define SIZE_RAW_HRM			25
#define SIZE_HRM			2
#define SIZE_ACCURACY_FIELD 2
#define SIZE_PAYLOAD		1
#define SIZE_BAC			1
#define SIZE_NORMAL_PACKET (SIZE_HDR + SIZE_STAMP + SIZE_3AXIS)
#define SIZE_QUAT_PACKET (SIZE_HDR + SIZE_STAMP + SIZE_QUAT + SIZE_ACCURACY_FIELD)
#define SIZE_1AXIS_PACKET (SIZE_HDR + SIZE_STAMP + SIZE_1AXIS)
#define SIZE_1AXIS_16BIT_PACKET (SIZE_HDR + SIZE_STAMP + SIZE_1AXIS_16BIT)
#define SIZE_EVENT_PACKET (SIZE_HDR + SIZE_STAMP)
#define SIZE_STEP_COUNTER_PACKET (SIZE_HDR + SIZE_STAMP + SIZE_STEP_COUNTER)
#ifdef PIXART_LIBRARY_IN_FIRMWARE
#define SIZE_HRM_PACKET (SIZE_HDR + SIZE_STAMP + SIZE_HRM)
#else
#define SIZE_HRM_PACKET (SIZE_HDR + SIZE_STAMP + SIZE_PAYLOAD + SIZE_RAW_HRM)
#endif
#define SIZE_BAC_PACKET (SIZE_HDR + SIZE_STAMP + SIZE_BAC)
#define SIZE_CALIBRATION_GAIN 36
#define SIZE_CALIBRATION_OFFSET 12
#define SIZE_FIRMWARE_INFO 15
#define SIZE_CLOCK_RATE 4
#define SIZE_PING 1
#define SIZE_REFERENCE_FRAME 36
#ifdef DMP_DOWNLOAD
#define SIZE_DMP_INFO 6
#endif
#define SIZE_ID 1
#define SIZE_CMD 1

struct icm30628_status_t {
	bool is_sensor_enabled[INV_SENSOR_MAX_NUM];
	bool is_sensor_in_batchmode[INV_SENSOR_MAX_NUM];	
	u64 sensor_sampling_rate[INV_SENSOR_MAX_NUM];
	u64 sensor_batching_timeout[INV_SENSOR_MAX_NUM];
	bool is_sensor_enabled_internally[INV_SENSOR_MAX_NUM];
	u64 sensor_internal_sampling_rate[INV_SENSOR_MAX_NUM];
	u64 sensor_internal_batching_timeout[INV_SENSOR_MAX_NUM];
	u32 kfifo_size;
	u32 fifo_size;
	u8 fifo_cache[INVENSENSE_KFIFO_LENGTH + SIZE_QUAT_PACKET];
	u32 fifo_unhandled_data_size;;	
	u8 fifo_unhandled_data[SIZE_QUAT_PACKET];	
	int accel_current_data[3];
	u8 accel_current_accuracy;
	int mag_current_data[3];
	u8 mag_current_accuracy;
	int orientation_current_data[3];
	u8 orientation_current_accuracy;
	int gyro_current_data[3];
	u8 gyro_current_accuracy;
	int baro_current_data[1];
	u8 baro_current_accuracy;
	int proximity_current_data[1];
	u8 proximity_current_accuracy;
	int gravity_current_data[3];
	u8 gravity_current_accuracy;
	int la_current_data[3];
	u8 la_current_accuracy;
	int rv_current_data[4];
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
	int orientation_raw_data[4];
#endif
	int rv_timestamp;
	u8 rv_current_accuracy;
	int grv_current_data[4];
	u8 grv_current_accuracy;
	int smd_current_data[1];
	u8 smd_current_accuracy;
	int sd_current_data[1];
	u8 sd_current_accuracy;
	int sc_current_data[1];
	u8 sc_current_accuracy;
	int gmrv_current_data[4];
	u8 gmrv_current_accuracy;
	int hrm_current_data[1];
	int hrm_timestamp;	
#ifndef PIXART_LIBRARY_IN_FIRMWARE
	u8 hrm_current_raw_data[13];
#endif
	u8 hrm_current_accuracy;
	int shk_current_data[1];
	u8 shk_current_accuracy;
	int bts_current_data[1];
	u8 bts_current_accuracy;
	u8 activity_current_data[1];
	u16 activity_current_status;
	u8 activity_current_accuracy;
	int sr_current_data[1];
	u8 sr_current_accuracy;
};

struct icm30628_calibration_info_t {
	u8 accel_calibration_gain[SIZE_CALIBRATION_GAIN];
	u8 accel_calibration_offset[SIZE_CALIBRATION_OFFSET];
	u8 gyro_calibration_gain[SIZE_CALIBRATION_GAIN];
	u8 gyro_calibration_offset[SIZE_CALIBRATION_OFFSET];
	u8 mag_calibration_gain[SIZE_CALIBRATION_GAIN];
	u8 mag_calibration_offset[SIZE_CALIBRATION_OFFSET];
};

struct icm30628_state_t {
#ifdef I2C_INTERFACE
	struct i2c_client *icm30628_i2c_client;
#endif
#ifdef SPI_INTERFACE
	struct spi_device *icm30628_spi_device;
#endif
	struct regulator *vdd_ana;
	struct regulator *vdd_1_8;
	int icm30628_irq;
	int icm30628_wakeup_irq;
	int icm30628_wake_gpio;
	struct icm30628_status_t icm30628_status;
	unsigned char icm30628_orientation[9];
	u32 icm30628_orientation_matrix[9];
	u32 icm30628_compass_orientation_matrix[9];
	long icm30628_quat_chip_to_body[4];
	struct kfifo *kfifo_buffer;
	struct task_struct *daemon_t;
#ifdef ENABLE_LP_EN	
	bool is_lp_enabled;
#endif
	struct mutex fifolock;
	struct wake_lock sleep_lock;
	bool is_firmware_download_done;	
	bool is_download_done;
	bool is_accel_calibrated;
	bool is_gyro_calibrated;
	bool is_mag_calibrated;	
	struct icm30628_calibration_info_t calibration;
#ifdef USING_WORKQUEUE_FOR_SENDING_COMMAND
	struct kfifo *kfifo_command_buffer;
	struct work_struct command_work;
#endif
	struct work_struct clear_int0_work;
	struct work_struct clear_int1_work;
	struct delayed_work watchdog_work;
	u64 watchdog_time;
	bool is_watchdog_running;
#ifdef DMP_DOWNLOAD
	bool dmp3_initiated;	
	bool dmp4_initiated;	
	bool dmp3_checked;	
	bool dmp4_checked;	
	u32 dmp3_address;	
	u32 dmp4_address;	
#endif	
};

struct icm30628_fifo_input_t {
	u8 sensorID;
	u8 command;
};

struct icm30628_fifo_output_t {
	u8 sensorID;
	u8 status;
	u32 timestamp;
	u8 data[MAXIMUM_DATA_SIZE];
};

struct icm30628_sensor_id_table_t {
	u16 sensor;
	u16 sensor_id;
};

#define HAL_DIV_ACCELEROMETER 417 // scale = 1/4096, unit : G, 1 G = 9.8m/s^2 => 4096 / 9.8 = 417
#define HAL_DIV_GEOMAGNETIC_FIELD 16// scale = 1/16, unit : uTesla, 
#define HAL_DIV_ORIENTATION 1000000 // scale 1/1, but for calculating floating point in kernel
#define HAL_DIV_GYROSCOPE 960
#define HAL_DIV_PRESSURE 100 // scale 1/100
#define HAL_DIV_PROXIMITY 1 // scale = 1/1
#define HAL_DIV_GRAVITY 417// scale = 1/4096, unit : G, 1 G = 9.8m/s^2 => 4096 / 9.8 = 417
#define HAL_DIV_LINEAR_ACCELERATION 417// scale = 1/4096, unit : G, 1 G = 9.8m/s^2 => 4096 / 9.8 = 417
#define HAL_DIV_ROTATION_VECTOR 1073741824 // scale  = 1/ 2^30
#define HAL_DIV_GAME_ROTATION_VECTOR 1073741824 // scale  = 1/ 2^30
#define HAL_DIV_SIGNIFICANT_MOTION 1 // event sensor
#define HAL_DIV_STEP_DETECTOR 1 // event sensor
#define HAL_DIV_STEP_COUNTER 1 // scale = 1/1
#define HAL_DIV_GEOMAGNETIC_ROTATION_VECTOR 1073741824 // scale  = 1/ 2^30
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
#define HAL_DIV_HEART_RATE 128 // scale 1/2^7 
#else
#define HAL_DIV_HEART_RATE 1000000 // scale 1/1, but for calculating floating point in kernel
#endif
#define HAL_DIV_SHAKE 1 // event sensor
#define HAL_DIV_BRING_TO_SEE 1 // event sensor
#define HAL_DIV_ACTIVITY_CLASSIFICATION 1 // event sensor
#define HAL_DIV_SENSOR_HUB 1

#if (FIRMWARE_VERSION >= 240)
#define GARNET_FIFO_ID_DATAOUT	1
#define GARNET_FIFO_ID_CMDIN	0
#else
#define GARNET_FIFO_ID_DATAOUT	0
#define GARNET_FIFO_ID_CMDIN	1
#endif

#define IVORY_WHO_AM_I 0x0000
#define IVORY_USER_CTRL_B0 0x03
#define IVORY_I2C_IF_DIS_BIT 0x10
#define IVORY_PWR_MGMT_1 0x0006
#define IVORY_BIT_H_RESET                     0x80
#define IVORY_BIT_SLEEP                       0x40
#define IVORY_BIT_LP_EN                       0x20
#define IVORY_BIT_CLK_PLL                     0x01

#define IVORY_MEM_START_ADDR 0x007C
#define IVORY_MEM_R_W 0x007D
#define IVORY_MEM_BANK_SEL 0x007E
#define IVORY_REG_BANK_SEL 0x7F

#define IVORY_GYRO_CONFIG_1_B2 0x0201     
#define IVORY_TEMP_CONFIG_B2 0x0253
#define IVORY_MOD_CTRL_USR_B2 0x0254

#define 	SCRATCH_M0_INT_STATUS 0x26 

#define MAX_SPI_TRANSACTION_SIZE 128 //256
#define MIN_PACKET_SIZE 6
#define DUMMY_PACKET_SIZE 6

#define SIG_ICM30628						44
#define ICM30628_IOCTL_GROUP                  0x10
#define ICM30628_WRITE_DAEMON_PID			_IO(ICM30628_IOCTL_GROUP, 1)
#define ICM30628_DOWNLOAD_FIRMWARE		_IO(ICM30628_IOCTL_GROUP, 2)
#define ICM30628_DOWNLOAD_DMP3			_IO(ICM30628_IOCTL_GROUP, 3)
#define ICM30628_DOWNLOAD_DMP4			_IO(ICM30628_IOCTL_GROUP, 4)
#define ICM30628_LOAD_CAL					_IO(ICM30628_IOCTL_GROUP, 5)
#define ICM30628_STORE_CAL					_IO(ICM30628_IOCTL_GROUP, 6)
#define ICM30628_READ_SENSOR_DATA		_IO(ICM30628_IOCTL_GROUP, 7)
#define ICM30628_WRITE_SENSOR_DATA		_IO(ICM30628_IOCTL_GROUP, 8)
#define ICM30628_GET_FIFO_SIZE				_IO(ICM30628_IOCTL_GROUP, 9)
#define ICM30628_GET_ORIENTATION			_IO(ICM30628_IOCTL_GROUP, 10)
#define ICM30628_SEND_ORIENTATION			_IO(ICM30628_IOCTL_GROUP, 11)
#define ICM30628_FIRMWARE_SIZE				_IO(ICM30628_IOCTL_GROUP, 12)
#define ICM30628_GET_HRM_DATA				_IO(ICM30628_IOCTL_GROUP, 13)
#define ICM30628_SEND_HRM_DATA			_IO(ICM30628_IOCTL_GROUP, 14)
#define ICM30628_GET_ORIENTATION_DATA	_IO(ICM30628_IOCTL_GROUP, 15)
#define ICM30628_SEND_ORIENTATION_DATA	_IO(ICM30628_IOCTL_GROUP, 16)
#define ICM30628_DMP3_SIZE					_IO(ICM30628_IOCTL_GROUP, 17)
#define ICM30628_DMP4_SIZE					_IO(ICM30628_IOCTL_GROUP, 18)
#define ICM30628_KERNEL_LOG				_IO(ICM30628_IOCTL_GROUP, 19)


#define REQUEST_SIGNAL_LOAD_CALIBRATION			0x01
#define REQUEST_SIGNAL_STORE_CALIBRATION		0x02
#define REQUEST_SIGNAL_PROCESS_HRM				0x03
#define REQUEST_SIGNAL_PROCESS_ORIENTATION	0x04

#ifdef MTK_PLATFORM
#define AXIS_MAP_X	0
#define AXIS_MAP_Y	1
#define AXIS_MAP_Z	2
#define NEGATE_X		1
#define NEGATE_Y		1
#define NEGATE_Z		0
#define MAG_AXIS_MAP_X	0
#define MAG_AXIS_MAP_Y	1
#define MAG_AXIS_MAP_Z	2
#define MAG_NEGATE_X	0
#define MAG_NEGATE_Y	0
#define MAG_NEGATE_Z	0
#else // in case of INVENSENSE TEST PLATFORM
#define AXIS_MAP_X	0
#define AXIS_MAP_Y	1
#define AXIS_MAP_Z	2
#define NEGATE_X		0
#define NEGATE_Y		1
#define NEGATE_Z		1
#define MAG_AXIS_MAP_X	0
#define MAG_AXIS_MAP_Y	1
#define MAG_AXIS_MAP_Z	2
#define MAG_NEGATE_X	0
#define MAG_NEGATE_Y	1
#define MAG_NEGATE_Z	1
#endif

int icm30628_register_data_path(int type);
int icm30628_register_control_path(int type);
int invensense_send_command_to_fifo(struct icm30628_state_t * st, int sensorID, int cmd, u8 * value);
int invensense_get_sensorNum_by_handle(int handle);
int invensense_get_sensorNum_by_fifo_data(u8* data);
int invensense_get_handle_by_sensorNum(int handle);
int invensense_kfifo_in(struct icm30628_state_t * st, u8 * data);
int invensense_kfifo_out(struct icm30628_state_t * st, u8 * data);
int invensense_kfifo_reset(struct icm30628_state_t * st);
int icm30628_set_power(int sensorNum, int powerstatus);
int icm30628_set_sensor_batch(int sensorNum, u64 timeout);
int icm30628_set_flush(int sensorNum);
int icm30628_set_sensor_onoff(int sensorNum, bool enable);
int icm30628_set_delay(int sensorNum, u64 delay_ns);
int invensense_get_fifo_status(struct icm30628_state_t * st, u8 fifo_id, u16 * length);
int invensense_get_data_from_fifo(struct icm30628_state_t * st);
int invensense_get_data_from_fifo_buffer(struct icm30628_state_t * st);
int invensense_fifo_read(void);
int invensense_apply_16_orientation(struct icm30628_state_t * st, const u8 src[]);
int invensense_apply_32_orientation(struct icm30628_state_t * st, const u8 src[]);
int invensense_apply_quat_orientation(struct icm30628_state_t * st, int * src);
int invensense_whoami(struct icm30628_state_t * st);
int icm30628_wake_up_m0(void);
int icm30628_set_LP_enable(int enable);
int icm30628_factory_sensor_enable(int sensorNum, bool enable);
int icm30628_factory_get_sensor_data(int sensorNum, void * data);
int icm30628_factory_clear_calibrator_data(int sensorNum);
int icm30628_factory_get_calibrator_data(int sensorNum, int * data);
int icm30628_factory_set_calibrator_data(int sensorNum, int * data);
u16 invensense_get_sensor_id(u16 sensor_num);
int invensense_fifo_write(struct icm30628_state_t * st, int length, u8 * data);
int invensense_accel_bias_found(struct icm30628_state_t * st, bool found);
int invensense_gyro_bias_found(struct icm30628_state_t * st, bool found);
int invensense_compass_bias_found(struct icm30628_state_t * st, bool found);
int invensense_start_command_workqueue(struct icm30628_state_t * st);
int icm30628_reset_fifo(u8 fifo_id);
int invensense_get_interrupt_status(u8 interrupt, u8 * status);
int invensense_start_clear_interrupt_status_work(void);
u32 invensense_batch_threshold_calculation(struct icm30628_state_t * st, u32 timeout);
int icm30628_update_set(void);
void invensense_start_watchdog_work_func(void);
int invensense_get_div_by_handle(int handle);
int invensense_post_initialize(struct icm30628_state_t * st);

#endif //ICM30628_H
