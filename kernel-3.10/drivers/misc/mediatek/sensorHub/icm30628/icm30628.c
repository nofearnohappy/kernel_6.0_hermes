/*
* Copyright (C) 2015 Invensense, Inc.
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

#include "icm30628.h"

#ifdef MTK_PLATFORM
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>
#include <mach/eint.h>
#include <mach/mt_pm_ldo.h>
extern struct work_struct eint_normal_work;
#ifdef MTK_EINT_WAKE_ENABLE
extern struct work_struct eint_wakeup_work;
#endif
#endif

struct icm30628_state_t * icm30628_state = NULL;

static struct icm30628_sensor_id_table_t icm30628_id_table[] =
{	
	{INV_META_DATA_NUM,INV_META_DATA},
	{INV_ACCELEROMETER_NUM,INV_ACCELEROMETER},
	{INV_MAGNETIC_FIELD_NUM,INV_MAGNETIC_FIELD},
	{INV_ORIENTATION_NUM,INV_ORIENTATION},
	{INV_GYROSCOPE_NUM,INV_GYROSCOPE},
	{INV_LIGHT_NUM,INV_LIGHT},
	{INV_PRESSURE_NUM,INV_PRESSURE},
	{INV_TEMPERATURE_NUM,INV_TEMPERATURE},
	{INV_PROXIMITY_NUM,INV_PROXIMITY},
	{INV_GRAVITY_NUM,INV_GRAVITY},
	{INV_LINEAR_ACCELERATION_NUM,INV_LINEAR_ACCELERATION},
	{INV_ROTATION_VECTOR_NUM,INV_ROTATION_VECTOR},
	{INV_RELATIVE_HUMIDITY_NUM,INV_RELATIVE_HUMIDITY},
	{INV_AMBIENT_TEMPERATURE_NUM,INV_AMBIENT_TEMPERATURE},
	{INV_MAGNETIC_FIELD_UNCALIBRATED_NUM,INV_MAGNETIC_FIELD_UNCALIBRATED},
	{INV_GAME_ROTATION_VECTOR_NUM,INV_GAME_ROTATION_VECTOR},
	{INV_GYROSCOPE_UNCALIBRATED_NUM,INV_GYROSCOPE_UNCALIBRATED},
	{INV_SIGNIFICANT_MOTION_NUM,INV_SIGNIFICANT_MOTION},
	{INV_STEP_DETECTOR_NUM,INV_STEP_DETECTOR},
	{INV_STEP_COUNTER_NUM,INV_STEP_COUNTER},
	{INV_GEOMAGNETIC_ROTATION_VECTOR_NUM,INV_GEOMAGNETIC_ROTATION_VECTOR},
	{INV_HEART_RATE_NUM,INV_HEART_RATE},
	{INV_SHAKE_NUM, INV_SHAKE},
	{INV_ACTIVITY_CLASSIFICATION_NUM,INV_ACTIVITY_CLASSIFICATION},
	{INV_BRING_TO_SEE_NUM, INV_BRING_TO_SEE},
	{INV_SCREEN_ROTATION_NUM,INV_SCREEN_ROTATION},
	{INV_PERFORM_SELFTEST_NUM,INV_PERFORM_SELFTEST},
	{INV_PLATFORM_SETUP_NUM,INV_PLATFORM_SETUP}
};

struct acc_init_info icm30628_accelerometer_info = {
	.name = ICM30628_ACC_DEV_NAME,
	.init = icm30628_local_accelerometer_init,
	.uninit = icm30628_local_accelerometer_remove,
};

struct gyro_init_info icm30628_gyroscope_info = {
	.name = ICM30628_GYRO_DEV_NAME,
	.init = icm30628_local_gyroscope_init,
	.uninit = icm30628_local_gyroscope_remove,
};

struct mag_init_info icm30628_magnetometer_info = {
	.name = ICM30628_MAG_DEV_NAME,
	.init = icm30628_local_magnetic_init,
	.uninit = icm30628_local_magnetic_remove,
};

struct grav_init_info icm30628_gravity_info = {
	.name = ICM30628_GRAV_DEV_NAME,
	.init = icm30628_local_gravity_init,
	.uninit = icm30628_local_gravity_remove,
};

struct la_init_info icm30628_linearaccel_info = {
	.name = ICM30628_LA_DEV_NAME,
	.init = icm30628_local_linearaccel_init,
	.uninit = icm30628_local_linearaccel_remove,
};

struct grv_init_info icm30628_grv_info = {
	.name = ICM30628_GRV_DEV_NAME,
	.init = icm30628_local_grv_init,
	.uninit = icm30628_local_grv_remove,
};

struct gmrv_init_info icm30628_gmrv_info = {
	.name = ICM30628_GMRV_DEV_NAME,
	.init = icm30628_local_gmrv_init,
	.uninit = icm30628_local_gmrv_remove,
};

struct rotationvector_init_info icm30628_rotationvector_info = {
	.name = ICM30628_RV_DEV_NAME,
	.init = icm30628_local_rotationvector_init,
	.uninit = icm30628_local_rotationvector_remove,
};

struct baro_init_info icm30628_baro_info = {
	.name = ICM30628_PRESSURE_DEV_NAME,
	.init = icm30628_local_pressure_init,
	.uninit = icm30628_local_pressure_remove,
};

struct alsps_init_info icm30628_proximity_info = {
	.name = ICM30628_PROXIMITY_DEV_NAME,
	.init = icm30628_local_proximity_init,
	.uninit = icm30628_local_proximity_remove,
};

struct hrm_init_info icm30628_heartrate_info = {
	.name = ICM30628_HRM_DEV_NAME,
	.init = icm30628_local_heartrate_init,
	.uninit = icm30628_local_heartrate_remove,
};

struct shk_init_info icm30628_shake_info = {
	.name = ICM30628_SHAKE_DEV_NAME,
	.init = icm30628_local_shake_init,
	.uninit = icm30628_local_shake_remove,
};

struct bts_init_info icm30628_bts_info = {
	.name = ICM30628_BTS_DEV_NAME,
	.init = icm30628_local_bringtosee_init,
	.uninit = icm30628_local_bringtosee_remove,
};

struct act_init_info icm30628_activity_info = {
	.name = ICM30628_ACTIVITY_DEV_NAME,
	.init = icm30628_local_activity_init,
	.uninit = icm30628_local_activity_remove,
};

struct step_c_init_info icm30628_step_c_info = {
	.name = ICM30628_STEP_C_DEV_NAME,
	.init = icm30628_local_step_c_init,
	.uninit = icm30628_local_step_c_init,
};

struct batch_init_info icm30628_batch_info = {
	.name = ICM30628_BATCH_DEV_NAME,
	.init = icm30628_local_batch_init,
	.uninit = icm30628_local_batch_init,
};

int icm30628_local_accelerometer_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_ACCELEROMETER_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_ACCELEROMETER_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_accelerometer_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_gyroscope_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_GYROSCOPE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_GYROSCOPE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_gyroscope_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_magnetic_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_MAGNETIC_FIELD_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_MAGNETIC_FIELD_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_magnetic_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_gravity_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_GRAVITY_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_GRAVITY_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_gravity_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}


int icm30628_local_linearaccel_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_LINEAR_ACCELERATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_LINEAR_ACCELERATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_linearaccel_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_grv_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_GAME_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_GAME_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}
int icm30628_local_grv_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_gmrv_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_GEOMAGNETIC_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_GEOMAGNETIC_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_gmrv_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_rotationvector_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_rotationvector_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_pressure_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_PRESSURE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_PRESSURE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_pressure_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_proximity_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_PROXIMITY_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_PROXIMITY_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_proximity_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_heartrate_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_HEART_RATE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_HEART_RATE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_heartrate_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_shake_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_SHAKE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_SHAKE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_shake_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_bringtosee_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_BRING_TO_SEE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BRING_TO_SEE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_bringtosee_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_activity_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_ACTIVITY_CLASSIFICATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_ACTIVITY_CLASSIFICATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_activity_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_step_c_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_STEP_COUNTER_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_STEP_COUNTER_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return 0;
}

int icm30628_local_step_c_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

int icm30628_local_batch_init(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = icm30628_register_data_path(INV_BATCH_ACCELEROMETER_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_ACCELEROMETER_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_GYROSCOPE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_GYROSCOPE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_GEOMAGNETIC_FIELD_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_GEOMAGNETIC_FIELD_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_ORIENTATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_ORIENTATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_GRAVITY_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_GRAVITY_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_LINEAR_ACCELERATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_LINEAR_ACCELERATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_GAME_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_GAME_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_GEOMAGNETIC_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_GEOMAGNETIC_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_ROTATION_VECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_PRESSURE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_PRESSURE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_PROXIMITY_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_PROXIMITY_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_ACTIVITY_CLASSIFICATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_ACTIVITY_CLASSIFICATION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_STEP_COUNTER_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_STEP_COUNTER_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_STEP_DETECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_STEP_DETECTOR_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_SIGNIFICANT_MOTION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_SIGNIFICANT_MOTION_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_HEART_RATE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_HEART_RATE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_SHAKE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_SHAKE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_register_data_path(INV_BATCH_BRING_TO_SEE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	ret = icm30628_register_control_path(INV_BATCH_BRING_TO_SEE_NUM);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_local_batch_remove(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	return ret;
}

//int enable_hw_batch(int handle, int flag, long long rate, long long latency)
int enable_hw_batch(int handle, int enable, int flag, long long rate,long long latency)
{
	int ret = 0;	
	struct icm30628_state_t * st = icm30628_state;
	int sensorNum = INV_SENSOR_INVALID_NUM;
	
	INV_DBG_FUNC_NAME;

	sensorNum = invensense_get_sensorNum_by_handle(handle);

	INV_INFO("enable_hw_batch(%d, %d, %lld, %lld), sensor = %d\n", handle, flag, rate, latency, sensorNum);

	if(UNLIKELY(sensorNum == INV_SENSOR_INVALID_NUM)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	//rate = rate * 1000000;

	ret = icm30628_set_delay(sensorNum, rate);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	do_div(latency,1000000);

	ret = icm30628_set_sensor_batch(sensorNum, latency);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_sensor_onoff(sensorNum, enable | st->icm30628_status.is_sensor_enabled[sensorNum]);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int batch_flush(int handle)
{
	int ret = 0;	
	int sensorNum = INV_SENSOR_INVALID_NUM;
	
	INV_DBG_FUNC_NAME;

	sensorNum = invensense_get_sensorNum_by_handle(handle);
	if(UNLIKELY(sensorNum == INV_SENSOR_INVALID_NUM)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_flush(sensorNum);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int batch_get_data(int handle, hwm_sensor_data * data)
{
	int ret = 0;	
	struct icm30628_state_t * st = icm30628_state;
	int sensorNum = INV_SENSOR_INVALID_NUM;
	hwm_sensor_data *sensor_data = data;
	u8 temp[128] = {0};
	int i;
	unsigned char bac_status = 0;
  u64 temp_ts;
  
	INV_DBG_FUNC_NAME;

	if(!data){
		ret = -1;
		INV_ERR;
		return ret;
	}


	ret = invensense_kfifo_out(st, temp);
	if(UNLIKELY(ret <0)){
		return ret;
	}

	sensorNum =  invensense_get_sensorNum_by_fifo_data(temp);	
	if(UNLIKELY(sensorNum == INV_SENSOR_INVALID_NUM)){
		ret = -1;
		INV_ERR;
		return ret;
	}

	handle =  invensense_get_handle_by_sensorNum(sensorNum);
	if(UNLIKELY(handle == -1)){
		ret = -1;
		INV_ERR;
		return ret;
	}

	switch(sensorNum){
		case INV_ACCELEROMETER:
		case INV_GRAVITY:
		case INV_LINEAR_ACCELERATION:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			for( i	= 0 ; i < 3 ; i++){ 		
				sensor_data->values[i] = (int)le16_to_cpup((__le16 *)(temp + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(sensor_data->values[i] > 0x7FFF) sensor_data->values[i] -= 0xFFFF;
			}			
			invensense_apply_32_orientation(st, (u8 *)sensor_data->values); 		
			INV_DATA("[batch] sensor = %d, {%d, %d, %d}\n", sensorNum, sensor_data->values[0] ,sensor_data->values[1] , sensor_data->values[2]);
			sensor_data->value_divide = HAL_DIV_ACCELEROMETER; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_GYROSCOPE:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			for( i	= 0 ; i < 3 ; i++){ 		
				sensor_data->values[i] = (int)le16_to_cpup((__le16 *)(temp + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(sensor_data->values[i] > 0x7FFF) sensor_data->values[i] -= 0xFFFF;
			}			
			invensense_apply_32_orientation(st, (u8 *)sensor_data->values); 		
			INV_DATA("[batch] sensor = %d, {%d, %d, %d}\n", sensorNum, sensor_data->values[0] ,sensor_data->values[1] , sensor_data->values[2]);
			sensor_data->value_divide = HAL_DIV_GYROSCOPE; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_MAGNETIC_FIELD:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			for( i	= 0 ; i < 3 ; i++){ 		
				sensor_data->values[i] = (int)le16_to_cpup((__le16 *)(temp + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(sensor_data->values[i] > 0x7FFF) sensor_data->values[i] -= 0xFFFF;
			}			
			invensense_apply_32_orientation(st, (u8 *)sensor_data->values); 		
			INV_DATA("[batch] sensor = %d, {%d, %d, %d}\n", sensorNum, sensor_data->values[0] ,sensor_data->values[1] , sensor_data->values[2]);
			sensor_data->value_divide = HAL_DIV_GEOMAGNETIC_FIELD; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_ORIENTATION:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
			for( i = 0 ; i < 3 ; i++){ 		
				sensor_data->values[i] = (int)le32_to_cpup((__le32 *)(temp + SIZE_HDR + SIZE_STAMP + i * sizeof(u32)));
			}			
#else
			for( i = 0 ; i < 3 ; i++){ 		
				sensor_data->values[i] = (int)le16_to_cpup((__le16 *)(temp + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(sensor_data->values[i] > 0x7FFF) sensor_data->values[i] -= 0xFFFF;
			}			
			invensense_apply_32_orientation(st, (u8 *)sensor_data->values); 		
#endif			
			INV_DATA("[batch] sensor = %d, {%d, %d, %d}\n", sensorNum, sensor_data->values[0] ,sensor_data->values[1] , sensor_data->values[2]);
			sensor_data->value_divide = HAL_DIV_ORIENTATION; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_GAME_ROTATION_VECTOR:
		case INV_GEOMAGNETIC_ROTATION_VECTOR:
		case INV_ROTATION_VECTOR:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			for( i	= 0 ; i < 4 ; i++){ 		
				sensor_data->values[i] = (int)le32_to_cpup((__le32 *)(temp + SIZE_HDR + SIZE_STAMP + i * sizeof(u32)));
			}			
			invensense_apply_quat_orientation(st, sensor_data->values); 		
			INV_DATA("[batch] sensor = %d, {%d, %d, %d, %d}\n", sensorNum, sensor_data->values[0], sensor_data->values[1], sensor_data->values[2], sensor_data->values[3]);
			sensor_data->value_divide = HAL_DIV_ROTATION_VECTOR; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_HEART_RATE:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			sensor_data->values[0] = (int)le32_to_cpup((__le32 *)(temp + SIZE_HDR + SIZE_STAMP));
			INV_DATA("[batch] sensor = %d, {%d}\n", sensorNum, sensor_data->values[0]);
			sensor_data->value_divide = HAL_DIV_HEART_RATE; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_PRESSURE:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
#ifdef PIXART_LIBRARY_IN_FIRMWARE
			sensor_data->values[0] = (int)le16_to_cpup((__le16 *)(temp + SIZE_HDR + SIZE_STAMP));
#else
			sensor_data->values[0] = (int)le32_to_cpup((__le32 *)(temp + SIZE_HDR + SIZE_STAMP));
#endif
			INV_DATA("[batch] sensor = %d, {%d}\n", sensorNum, sensor_data->values[0]);
			sensor_data->value_divide = HAL_DIV_PRESSURE; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_PROXIMITY:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			sensor_data->values[0] = (int)le16_to_cpup((__le16 *)(temp + SIZE_HDR + SIZE_STAMP));
			INV_DATA("[batch] sensor = %d, {%d}\n", sensorNum, sensor_data->values[0]);
			sensor_data->value_divide = HAL_DIV_PROXIMITY; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_STEP_COUNTER:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			sensor_data->values[0] = (int)le32_to_cpup((__le32 *)(temp + SIZE_HDR + SIZE_STAMP));
			INV_DATA("[batch] sensor = %d, {%d}\n", sensorNum, sensor_data->values[0]);
			sensor_data->value_divide = HAL_DIV_STEP_COUNTER; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_STEP_DETECTOR:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			INV_DATA("[batch] sensor = %d, occurs\n", sensorNum);
			sensor_data->value_divide = HAL_DIV_STEP_DETECTOR; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_SIGNIFICANT_MOTION:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			INV_DATA("[batch] sensor = %d, occurs\n", sensorNum);
			sensor_data->value_divide = HAL_DIV_STEP_DETECTOR; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_ACTIVITY_CLASSIFICATION:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			bac_status = temp[SIZE_HDR + SIZE_STAMP];
			INV_DATA("[batch] sensor = %d, {%x}\n", sensorNum, sensor_data->values[0]);
			if(bac_status == INV_DATA_BAC_ACTIVITY_UNKNOWN){
				st->icm30628_status.activity_current_status = UNKNOWN;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_IN_VEHICLE_START){
				st->icm30628_status.activity_current_status |= IN_VEHICLE;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_IN_VEHICLE_END){
				st->icm30628_status.activity_current_status &= ~IN_VEHICLE;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_WALKING_START){
				st->icm30628_status.activity_current_status |= WALKING;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_WALKING_END){
				st->icm30628_status.activity_current_status &= ~WALKING;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_RUNNING_START){
				st->icm30628_status.activity_current_status |= RUNNING;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_RUNNING_END){
				st->icm30628_status.activity_current_status &= ~RUNNING;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_ON_BICYCLE_START){
				st->icm30628_status.activity_current_status |= ON_BICYCLE;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_ON_BICYCLE_END){
				st->icm30628_status.activity_current_status &= ~ON_BICYCLE;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_TILT_START){
				st->icm30628_status.activity_current_status |= TILT;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_TILT_END){
				st->icm30628_status.activity_current_status &= ~TILT;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_STILL_START){
				st->icm30628_status.activity_current_status |= STILL;
			}
			if(bac_status == INV_DATA_BAC_ACTIVITY_STILL_END){
				st->icm30628_status.activity_current_status &= ~STILL;
			}
			sensor_data->values[0] = (st->icm30628_status.activity_current_status & IN_VEHICLE)? 1 : 0;
			sensor_data->values[1] = (st->icm30628_status.activity_current_status & ON_BICYCLE)? 1 : 0;
			sensor_data->values[2] = (st->icm30628_status.activity_current_status & (WALKING|RUNNING))? 1 : 0;
			sensor_data->values[3] = (st->icm30628_status.activity_current_status & STILL)? 1 : 0;
			sensor_data->values[4] = (st->icm30628_status.activity_current_status & UNKNOWN)? 1 : 0;
			sensor_data->values[5] = (st->icm30628_status.activity_current_status & TILT)? 1 : 0;		
			sensor_data->value_divide = HAL_DIV_ACTIVITY_CLASSIFICATION; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_SHAKE:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			INV_DATA("[batch] sensor = %d, occurs\n", sensorNum);
			sensor_data->value_divide = HAL_DIV_SHAKE; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		case INV_BRING_TO_SEE:
			sensor_data->sensor = handle;
			temp_ts = (u64)(le32_to_cpup((__le32 *)&temp[2]) * GARNET_SYSTEM_CLOCK_UNIT);			
			do_div(temp_ts,100000000);
			sensor_data->time = temp_ts;
			INV_DATA("[batch] sensor = %d, occurs\n", sensorNum);
			sensor_data->value_divide = HAL_DIV_BRING_TO_SEE; 
			sensor_data->status = temp[1] & 0x3;
			sensor_data->update = true;
			break;
		default:
			// unknown error occurs, fifo reset
			invensense_kfifo_reset(st);
			ret = -EINVAL;	
			INV_ERR;
			break;
	}

	return ret;
}

//int batch_get_fifo_status(int * fifo_len, int * fifo_status, int handle)
int batch_get_fifo_status(int * fifo_len, int * fifo_status, char *reserved, struct batch_timestamp_info *p_batch_timestampe_info)
{
	int ret = 0;	
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(!fifo_len){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(!fifo_status){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*fifo_len = st->icm30628_status.kfifo_size;
	*fifo_status = 0;

	return ret;
}

int accelerometer_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;

	return ret;
}

int accelerometer_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_ACCELEROMETER_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int accelerometer_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_ACCELEROMETER_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
		
	return ret;
}
//static int test_value=1000;
//#define TEST_HAL_FAKE_VALUE
int accelerometer_get_data(int *x, int *y, int *z, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!x || !y || !z || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*x = st->icm30628_status.accel_current_data[0];
	*y = st->icm30628_status.accel_current_data[1];
	*z = st->icm30628_status.accel_current_data[2];
	*status = st->icm30628_status.accel_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*x==0 && *y==0 && *z==0)
	{
		*x = *y = *z = test_value++;
	}
#endif

	return ret;
}

int gyroscope_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
				
	return ret;
}

int gyroscope_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_GYROSCOPE_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int gyroscope_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_GYROSCOPE_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int gyroscope_get_data(int *x, int *y, int *z, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!x || !y || !z || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*x = st->icm30628_status.gyro_current_data[0];
	*y = st->icm30628_status.gyro_current_data[1];
	*z = st->icm30628_status.gyro_current_data[2];
	*status = st->icm30628_status.gyro_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*x==0 && *y==0 && *z==0)
	{
		*x = *y = *z = test_value++;
	}
#endif

	return ret;
}

int magnetic_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int magnetic_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_MAGNETIC_FIELD_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int magnetic_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_MAGNETIC_FIELD_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int magnetic_get_data(int *x, int *y, int *z, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!x || !y || !z || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*x = st->icm30628_status.mag_current_data[0];
	*y = st->icm30628_status.mag_current_data[1];
	*z = st->icm30628_status.mag_current_data[2];
	*status = st->icm30628_status.mag_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*x==0 && *y==0 && *z==0)
	{
		*x = *y = *z = test_value++;
	}
#endif

	return ret;
}

int orientation_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int orientation_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_ORIENTATION_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int orientation_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_ORIENTATION_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int orientation_get_data(int *azimuth, int *pitch, int *roll, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!azimuth || !pitch || !roll || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*azimuth = st->icm30628_status.orientation_current_data[0];
	*pitch = st->icm30628_status.orientation_current_data[1];
	*roll = st->icm30628_status.orientation_current_data[2];
	*status = st->icm30628_status.orientation_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*azimuth==0 && *pitch==0 && *roll==0)
	{
		*azimuth = *pitch = *roll = test_value++;
	}
#endif

	return ret;
}

int pressure_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int pressure_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_PRESSURE_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int pressure_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_PRESSURE_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int pressure_get_data(int *pressure, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!pressure || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*pressure = st->icm30628_status.baro_current_data[0];
	*status = st->icm30628_status.baro_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*pressure==0)
	{
		*pressure = test_value++;
	}
#endif

	return ret;
}

int gravity_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;

	return ret;
}

int gravity_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_GRAVITY_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
 
	return ret;
}

int gravity_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_GRAVITY_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int gravity_get_data(int *x, int *y, int *z, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!x || !y || !z || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	*x = st->icm30628_status.gravity_current_data[0];
	*y = st->icm30628_status.gravity_current_data[1];
	*z = st->icm30628_status.gravity_current_data[2];
	*status = st->icm30628_status.gravity_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*x==0 && *y==0 && *z==0)
	{
		*x = *y = *z = test_value++;
	}
#endif
	
	return ret;
}

int linearaccel_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int linearaccel_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;
	
	ret = icm30628_set_sensor_onoff(INV_LINEAR_ACCELERATION_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int linearaccel_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_LINEAR_ACCELERATION_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int linearaccel_get_data(int *x, int *y, int *z, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!x || !y || !z || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	*x = st->icm30628_status.la_current_data[0];
	*y = st->icm30628_status.la_current_data[1];
	*z = st->icm30628_status.la_current_data[2];
	*status = st->icm30628_status.la_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*x==0 && *y==0 && *z==0)
	{
		*x = *y = *z = test_value++;
	}
#endif

	return ret;
}

int rotationvector_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;

	return ret;
}

int rotationvector_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_ROTATION_VECTOR_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
 
	return ret;
}

int rotationvector_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_ROTATION_VECTOR_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int rotationvector_get_data(int *scalar, int *q1, int *q2, int *q3, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!scalar || !q1 || !q2 || !q3 || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*scalar = st->icm30628_status.rv_current_data[0];
	*q1 = st->icm30628_status.rv_current_data[1];
	*q2 = st->icm30628_status.rv_current_data[2];
	*q3 = st->icm30628_status.rv_current_data[3];
	*status = st->icm30628_status.rv_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*q1==0 && *q2==0 && *q3==0)
	{
		*q1 = *q2 = *q3 = test_value++;
	}
#endif

	return ret;
}

int grv_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int grv_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_GAME_ROTATION_VECTOR_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int grv_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_GAME_ROTATION_VECTOR_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int grv_get_data(int *scalar, int *q1, int *q2, int *q3, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!scalar || !q1 || !q2 || !q3 || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*scalar = st->icm30628_status.grv_current_data[0];
	*q1 = st->icm30628_status.grv_current_data[1];
	*q2 = st->icm30628_status.grv_current_data[2];
	*q3 = st->icm30628_status.grv_current_data[3];
	*status = st->icm30628_status.grv_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*q1==0 && *q2==0 && *q3==0)
	{
		*q1 = *q2 = *q3 = test_value++;
	}
#endif

	return ret;
}

int smd_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int smd_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_SIGNIFICANT_MOTION_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int smd_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_SIGNIFICANT_MOTION_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int smd_get_data(int *detect)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!detect)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*detect = *(int *)&st->icm30628_status.smd_current_data[0];

#ifdef TEST_HAL_FAKE_VALUE	
	if(*detect==0)
	{
		*detect = test_value++;
	}
#endif

	return ret;
}

int sd_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;

	return ret;
}

int sd_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_STEP_DETECTOR_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
 
	return ret;
}

int sd_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_STEP_DETECTOR_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int sd_get_data(int *detect)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!detect)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*detect = st->icm30628_status.sd_current_data[0];

#ifdef TEST_HAL_FAKE_VALUE	
	if(*detect==0)
	{
		*detect = test_value++;
	}
#endif

	return ret;
}

int sc_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int sc_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_STEP_COUNTER_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int sc_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_STEP_COUNTER_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int sc_get_data(u64 *count, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!count || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*count = st->icm30628_status.sc_current_data[0];

#ifdef TEST_HAL_FAKE_VALUE	
	if(*count==0)
	{
		*count = test_value++;
	}
#endif

#ifdef POLLING_MODE
#else
	INV_INFO("step counter report!! = %lld\n", *count);
#endif

	return ret;
}

int gmrv_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int gmrv_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_GEOMAGNETIC_ROTATION_VECTOR_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int gmrv_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_GEOMAGNETIC_ROTATION_VECTOR_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int gmrv_get_data(int *scalar, int *q1, int *q2, int *q3, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!scalar || !q1 || !q2 || !q3 || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*scalar = st->icm30628_status.gmrv_current_data[0];
	*q1 = st->icm30628_status.gmrv_current_data[1];
	*q2 = st->icm30628_status.gmrv_current_data[2];
	*q3 = st->icm30628_status.gmrv_current_data[3];
	*status = st->icm30628_status.gmrv_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*q1==0 && *q2==0 && *q3==0)
	{
		*q1 = *q2 = *q3 = test_value++;
	}
#endif

	return ret;
}

int hrm_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int hrm_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_HEART_RATE_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int hrm_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_HEART_RATE_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int hrm_get_data(int *rate, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!rate || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*rate = st->icm30628_status.hrm_current_data[0];
	*status = st->icm30628_status.hrm_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*rate==0)
	{
		*rate = test_value++;
	}
#endif

	return ret;
}

int proximity_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int proximity_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_PROXIMITY_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int proximity_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_PROXIMITY_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int proximity_get_data(int *distance, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!distance || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*distance = st->icm30628_status.proximity_current_data[0];
	*status = st->icm30628_status.proximity_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*distance==0)
	{
		*distance = test_value++;
	}
#endif

	return ret;
}

int activity_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;
 
	return ret;
}

int activity_set_enable(int enable)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_ACTIVITY_CLASSIFICATION_NUM, enable);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int activity_set_delay(u64 delay)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_delay(INV_ACTIVITY_CLASSIFICATION_NUM, delay);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
	
	return ret;
}

int activity_get_data(u16 *detect, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!detect || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	detect[0] = (st->icm30628_status.activity_current_status & IN_VEHICLE)? 1 : 0;
	detect[1] = (st->icm30628_status.activity_current_status & ON_BICYCLE)? 1 : 0;
	detect[2] = (st->icm30628_status.activity_current_status & (WALKING|RUNNING))? 1 : 0;
	detect[3] = (st->icm30628_status.activity_current_status & STILL)? 1 : 0;
	detect[4] = (st->icm30628_status.activity_current_status & UNKNOWN)? 1 : 0;
	detect[5] = (st->icm30628_status.activity_current_status & TILT)? 1 : 0;
	*status = st->icm30628_status.activity_current_accuracy;

	return ret;
}

int shake_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_SHAKE_NUM, open);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
 
	return ret;
}

int shake_get_data(u16 *detect, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!detect || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*detect = st->icm30628_status.shk_current_data[0];
	*status = st->icm30628_status.shk_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*detect==0)
	{
		*detect = test_value++;
	}
#endif

	return ret;
}

int bts_set_open_report(int open)
{
	int ret = 0;	

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_sensor_onoff(INV_BRING_TO_SEE_NUM, open);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
 
	return ret;
}

int bts_get_data(u16 *detect, int *status)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	if(UNLIKELY(!detect || !status)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	*detect = st->icm30628_status.bts_current_data[0];
	*status = st->icm30628_status.bts_current_accuracy;

#ifdef TEST_HAL_FAKE_VALUE	
	if(*detect==0)
	{
		*detect = test_value++;
	}
#endif

	return ret;
}

int icm30628_register_data_path(int type)
{
	int ret = 0;

	struct acc_data_path accelerometer_data_path={0};	
	struct gyro_data_path gyroscope_data_path={0};	
	struct mag_data_path magnetic_data_path={0};
	struct grav_data_path gravity_data_path={0};
	struct la_data_path linearaccel_data_path={0};
	struct grv_data_path grv_data_path={0};
	struct gmrv_data_path gmrv_data_path={0};
	struct rotationvector_data_path rotationvector_data_path={0};
	struct baro_data_path pressure_data_path={0};
	struct ps_data_path proximity_data_path={0};
	struct hrm_data_path heartrate_data_path={0};
	struct shk_data_path shake_data_path={0};
	struct bts_data_path bringtosee_data_path={0};
	struct act_data_path activity_data_path={0};
	struct step_c_data_path step_c_data_path={0};
	struct batch_data_path batch_data_path={0};

	INV_DBG_FUNC_NAME;

	batch_data_path.samplingPeriodMs = 5;
	batch_data_path.flags = 0;
	batch_data_path.get_data = batch_get_data;
	batch_data_path.is_batch_supported = true;
	batch_data_path.get_fifo_status = batch_get_fifo_status;

	switch(type){
		case INV_ACCELEROMETER_NUM:
			accelerometer_data_path.get_data = accelerometer_get_data;
			accelerometer_data_path.get_raw_data = NULL;			
			accelerometer_data_path.vender_div = HAL_DIV_ACCELEROMETER;
			ret = acc_register_data_path(&accelerometer_data_path);		
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_MAGNETIC_FIELD_NUM:	
		case INV_ORIENTATION_NUM:
			magnetic_data_path.get_data_m = magnetic_get_data;
			magnetic_data_path.get_data_o = orientation_get_data;
			magnetic_data_path.get_raw_data = NULL;			
			magnetic_data_path.div_m = HAL_DIV_GEOMAGNETIC_FIELD;
			magnetic_data_path.div_o = HAL_DIV_ORIENTATION;
			ret = mag_register_data_path(&magnetic_data_path);		
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_GYROSCOPE_NUM:
			gyroscope_data_path.get_data = gyroscope_get_data;
			gyroscope_data_path.get_raw_data = NULL;
			gyroscope_data_path.vender_div = HAL_DIV_GYROSCOPE;
			ret = gyro_register_data_path(&gyroscope_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_PRESSURE_NUM:
			pressure_data_path.get_data = pressure_get_data;
			pressure_data_path.get_raw_data = NULL;
			pressure_data_path.vender_div = HAL_DIV_PRESSURE;
			ret = baro_register_data_path(&pressure_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_PROXIMITY_NUM:
			proximity_data_path.get_data = proximity_get_data;
			proximity_data_path.ps_get_raw_data = NULL;
			proximity_data_path.vender_div = HAL_DIV_PROXIMITY;
			ret = ps_register_data_path(&proximity_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_GRAVITY_NUM:
			gravity_data_path.get_data = gravity_get_data;
			gravity_data_path.get_raw_data = NULL;
			gravity_data_path.vender_div = HAL_DIV_GRAVITY;
			ret = grav_register_data_path(&gravity_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_LINEAR_ACCELERATION_NUM:
			linearaccel_data_path.get_data = linearaccel_get_data;
			linearaccel_data_path.get_raw_data = NULL;
			linearaccel_data_path.vender_div = HAL_DIV_LINEAR_ACCELERATION;
			ret = la_register_data_path(&linearaccel_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_ROTATION_VECTOR_NUM:
			rotationvector_data_path.get_data = rotationvector_get_data;
			rotationvector_data_path.get_raw_data = NULL;
			rotationvector_data_path.vender_div = HAL_DIV_ROTATION_VECTOR;
			ret = rotationvector_register_data_path(&rotationvector_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_GAME_ROTATION_VECTOR_NUM:
			grv_data_path.get_data = grv_get_data;
			grv_data_path.get_raw_data = NULL;
			grv_data_path.vender_div = HAL_DIV_GAME_ROTATION_VECTOR;
			ret = grv_register_data_path(&grv_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_SIGNIFICANT_MOTION_NUM:
		case INV_STEP_DETECTOR_NUM:
		case INV_STEP_COUNTER_NUM:
			step_c_data_path.get_data = sc_get_data;
			step_c_data_path.get_data_significant =  smd_get_data;
			step_c_data_path.get_data_step_d =  sd_get_data;
			step_c_data_path.vender_div = HAL_DIV_STEP_COUNTER;
			ret = step_c_register_data_path(&step_c_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_GEOMAGNETIC_ROTATION_VECTOR_NUM:
			gmrv_data_path.get_data = gmrv_get_data;
			gmrv_data_path.get_raw_data = NULL;
			gmrv_data_path.vender_div = HAL_DIV_GEOMAGNETIC_ROTATION_VECTOR;
			ret = gmrv_register_data_path(&gmrv_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_HEART_RATE_NUM:
			heartrate_data_path.get_data = hrm_get_data;
			//heartrate_data_path.get_raw_data = NULL;
			heartrate_data_path.vender_div = HAL_DIV_HEART_RATE;
			ret = hrm_register_data_path(&heartrate_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_ACTIVITY_CLASSIFICATION_NUM:
			activity_data_path.get_data = activity_get_data;
			activity_data_path.vender_div = HAL_DIV_ACTIVITY_CLASSIFICATION;
			ret = act_register_data_path(&activity_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_SHAKE_NUM:
			shake_data_path.get_data = shake_get_data;
			ret = shk_register_data_path(&shake_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_BRING_TO_SEE_NUM:
			bringtosee_data_path.get_data = bts_get_data;
			ret = bts_register_data_path(&bringtosee_data_path); 	
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}					
			break;
		case INV_BATCH_ACCELEROMETER_NUM:
			batch_register_data_path(ID_ACCELEROMETER, &batch_data_path);			
			batch_register_support_info(ID_ACCELEROMETER, true, invensense_get_div_by_handle(ID_ACCELEROMETER), true);
			break;
		case INV_BATCH_GEOMAGNETIC_FIELD_NUM:
			batch_register_data_path(ID_MAGNETIC, &batch_data_path);	
			batch_register_support_info(ID_MAGNETIC, true, invensense_get_div_by_handle(ID_MAGNETIC), true);
			break;
		case INV_BATCH_ORIENTATION_NUM:
			batch_register_data_path(ID_ORIENTATION, &batch_data_path);			
			batch_register_support_info(ID_ORIENTATION, true, invensense_get_div_by_handle(ID_ORIENTATION), true);
			break;
		case INV_BATCH_GYROSCOPE_NUM:
			batch_register_data_path(ID_GYROSCOPE, &batch_data_path);			
			batch_register_support_info(ID_GYROSCOPE, true, invensense_get_div_by_handle(ID_GYROSCOPE), true);
			break;
		case INV_BATCH_PRESSURE_NUM:
			batch_register_data_path(ID_PRESSURE, &batch_data_path);			
			batch_register_support_info(ID_PRESSURE, true, invensense_get_div_by_handle(ID_PRESSURE), true);
			break;
		case INV_BATCH_PROXIMITY_NUM:
			batch_register_data_path(ID_PROXIMITY, &batch_data_path);			
			batch_register_support_info(ID_PROXIMITY, true, invensense_get_div_by_handle(ID_PROXIMITY), true);
			break;
		case INV_BATCH_GRAVITY_NUM:
			batch_register_data_path(ID_GRAVITY, &batch_data_path);			
			batch_register_support_info(ID_GRAVITY, true, invensense_get_div_by_handle(ID_GRAVITY), true);
			break;
		case INV_BATCH_LINEAR_ACCELERATION_NUM:
			batch_register_data_path(ID_LINEAR_ACCELERATION, &batch_data_path);			
			batch_register_support_info(ID_LINEAR_ACCELERATION, true, invensense_get_div_by_handle(ID_LINEAR_ACCELERATION), true);
			break;
		case INV_BATCH_ROTATION_VECTOR_NUM:
			batch_register_data_path(ID_ROTATION_VECTOR, &batch_data_path);			
			batch_register_support_info(ID_ROTATION_VECTOR, true, invensense_get_div_by_handle(ID_ROTATION_VECTOR), true);
			break;
		case INV_BATCH_GAME_ROTATION_VECTOR_NUM:
			batch_register_data_path(ID_GAME_ROTATION_VECTOR, &batch_data_path);			
			batch_register_support_info(ID_GAME_ROTATION_VECTOR, true, invensense_get_div_by_handle(ID_GAME_ROTATION_VECTOR), true);
			break;
		case INV_BATCH_SIGNIFICANT_MOTION_NUM:
			batch_register_data_path(ID_SIGNIFICANT_MOTION, &batch_data_path);			
			batch_register_support_info(ID_SIGNIFICANT_MOTION, false, invensense_get_div_by_handle(ID_SIGNIFICANT_MOTION), true);
			break;
		case INV_BATCH_STEP_DETECTOR_NUM:
			batch_register_data_path(ID_STEP_DETECTOR, &batch_data_path);			
			batch_register_support_info(ID_STEP_DETECTOR, false, invensense_get_div_by_handle(ID_STEP_DETECTOR), true);
			break;
		case INV_BATCH_STEP_COUNTER_NUM:
			batch_register_data_path(ID_STEP_COUNTER, &batch_data_path);			
			batch_register_support_info(ID_STEP_COUNTER, false, invensense_get_div_by_handle(ID_STEP_COUNTER), true);
			break;
		case INV_BATCH_GEOMAGNETIC_ROTATION_VECTOR_NUM:
			batch_register_data_path(ID_GEOMAGNETIC_ROTATION_VECTOR, &batch_data_path);			
			batch_register_support_info(ID_GEOMAGNETIC_ROTATION_VECTOR, true, invensense_get_div_by_handle(ID_GEOMAGNETIC_ROTATION_VECTOR), true);
			break;
		case INV_BATCH_ACTIVITY_CLASSIFICATION_NUM:
			batch_register_data_path(ID_ACTIVITY, &batch_data_path);			
			batch_register_support_info(ID_ACTIVITY, false, invensense_get_div_by_handle(ID_ACTIVITY), true);
			break;
		case INV_BATCH_HEART_RATE_NUM:
			batch_register_data_path(ID_HEART_RATE, &batch_data_path);			
			batch_register_support_info(ID_HEART_RATE, true, invensense_get_div_by_handle(ID_HEART_RATE), true);
			break;
		case INV_BATCH_SHAKE_NUM:
			batch_register_data_path(ID_SHAKE, &batch_data_path);			
			batch_register_support_info(ID_SHAKE, false, invensense_get_div_by_handle(ID_SHAKE), true);
			break;
		case INV_BATCH_BRING_TO_SEE_NUM:
			batch_register_data_path(ID_BRINGTOSEE, &batch_data_path);			
			batch_register_support_info(ID_BRINGTOSEE, false, invensense_get_div_by_handle(ID_BRINGTOSEE), true);
			break;
		case INV_BATCH_SENSOR_HUB_NUM:
			batch_register_data_path(MAX_ANDROID_SENSOR_NUM, &batch_data_path);			
			batch_register_support_info(MAX_ANDROID_SENSOR_NUM, true, invensense_get_div_by_handle(MAX_ANDROID_SENSOR_NUM), true);
			break;		
		default :
			ret = -EINVAL;
			INV_ERR;
			break;
	}

	return ret;
}

int icm30628_register_control_path(int type)
{
	int ret = 0;

	struct acc_control_path accelerometer_control_path={0};	
	struct gyro_control_path gyroscope_control_path={0};	
	struct mag_control_path magnetic_control_path={0};
	struct grav_control_path gravity_control_path={0};
	struct la_control_path linearaccel_control_path={0};
	struct grv_control_path grvector_control_path={0};
	struct gmrv_control_path gmrvector_control_path={0};
	struct rotationvector_control_path rvector_control_path={0};
	struct baro_control_path pressure_control_path={0};
	struct ps_control_path proximity_control_path={0};
	struct hrm_control_path heartrate_control_path={0};
	struct shk_control_path shake_control_path={0};
	struct bts_control_path bringtosee_control_path={0};
	struct act_control_path activity_control_path={0};
	struct step_c_control_path step_counter_control_path={0};
	struct batch_control_path batchmode_control_path={0};

	INV_DBG_FUNC_NAME;

	switch(type){
		case INV_ACCELEROMETER_NUM:
			accelerometer_control_path.enable_nodata = accelerometer_set_enable;
			accelerometer_control_path.is_report_input_direct = false;
			accelerometer_control_path.is_support_batch = true;
			accelerometer_control_path.open_report_data = accelerometer_set_open_report;
			accelerometer_control_path.set_delay = accelerometer_set_delay;
			ret = acc_register_control_path(&accelerometer_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_MAGNETIC_FIELD_NUM:
		case INV_ORIENTATION_NUM:
			magnetic_control_path.is_report_input_direct = false;
			magnetic_control_path.is_support_batch = true;
			magnetic_control_path.m_enable = magnetic_set_enable;
			magnetic_control_path.m_open_report_data = magnetic_set_open_report;
			magnetic_control_path.m_set_delay = magnetic_set_delay;
			magnetic_control_path.o_enable = orientation_set_enable;
			magnetic_control_path.o_open_report_data = orientation_set_open_report;
			magnetic_control_path.o_set_delay = orientation_set_delay;
			ret = mag_register_control_path(&magnetic_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_GYROSCOPE_NUM:
			gyroscope_control_path.enable_nodata = gyroscope_set_enable;
			gyroscope_control_path.is_report_input_direct = false;
			gyroscope_control_path.is_support_batch = true;
			gyroscope_control_path.open_report_data = gyroscope_set_open_report;
			gyroscope_control_path.set_delay = gyroscope_set_delay;
			ret = gyro_register_control_path(&gyroscope_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_PRESSURE_NUM:
			pressure_control_path.enable_nodata = pressure_set_enable;
			pressure_control_path.is_report_input_direct = false;
			pressure_control_path.is_support_batch = true;
			pressure_control_path.open_report_data = pressure_set_open_report;
			pressure_control_path.set_delay = pressure_set_delay;
			ret = baro_register_control_path(&pressure_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_PROXIMITY_NUM:
			proximity_control_path.enable_nodata = proximity_set_enable;
#ifdef POLLING_MODE
			proximity_control_path.is_polling_mode = true;
			proximity_control_path.is_report_input_direct = false; //polling mode
#else
			proximity_control_path.is_polling_mode = false;
			proximity_control_path.is_report_input_direct = true;
#endif
			proximity_control_path.is_support_batch = true;
			proximity_control_path.open_report_data = proximity_set_open_report;
			proximity_control_path.set_delay = proximity_set_delay;
			ret = ps_register_control_path(&proximity_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_GRAVITY_NUM:
			gravity_control_path.enable_nodata = gravity_set_enable;
			gravity_control_path.is_report_input_direct = false;
			gravity_control_path.is_support_batch = true;
			gravity_control_path.open_report_data = gravity_set_open_report;
			gravity_control_path.set_delay = gravity_set_delay;
			ret = grav_register_control_path(&gravity_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_LINEAR_ACCELERATION_NUM:
			linearaccel_control_path.enable_nodata = linearaccel_set_enable;
			linearaccel_control_path.is_report_input_direct = false;
			linearaccel_control_path.is_support_batch = true;
			linearaccel_control_path.open_report_data = linearaccel_set_open_report;
			linearaccel_control_path.set_delay = linearaccel_set_delay;
			ret = la_register_control_path(&linearaccel_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_ROTATION_VECTOR_NUM:
			rvector_control_path.enable_nodata = rotationvector_set_enable;
			rvector_control_path.is_report_input_direct = false;
			rvector_control_path.is_support_batch = true;
			rvector_control_path.open_report_data = rotationvector_set_open_report;
			rvector_control_path.set_delay = rotationvector_set_delay;
			ret = rotationvector_register_control_path(&rvector_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_GAME_ROTATION_VECTOR_NUM:
			grvector_control_path.enable_nodata = grv_set_enable;
			grvector_control_path.is_report_input_direct = false;
			grvector_control_path.is_support_batch = true;
			grvector_control_path.open_report_data = grv_set_open_report;
			grvector_control_path.set_delay = grv_set_delay;
			ret = grv_register_control_path(&grvector_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_SIGNIFICANT_MOTION_NUM:
		case INV_STEP_DETECTOR_NUM:
		case INV_STEP_COUNTER_NUM:
			step_counter_control_path.enable_nodata = sc_set_enable;
			step_counter_control_path.enable_significant = smd_set_enable;
			step_counter_control_path.enable_step_detect = sd_set_enable;
			step_counter_control_path.is_report_input_direct = false;
			step_counter_control_path.is_support_batch = false;
			step_counter_control_path.open_report_data = sc_set_open_report;
			step_counter_control_path.set_delay = sc_set_delay;
			ret = step_c_register_control_path(&step_counter_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_GEOMAGNETIC_ROTATION_VECTOR_NUM:
			gmrvector_control_path.enable_nodata = gmrv_set_enable;
			gmrvector_control_path.is_report_input_direct = false;
			gmrvector_control_path.is_support_batch = true;
			gmrvector_control_path.open_report_data = gmrv_set_open_report;
			gmrvector_control_path.set_delay = gmrv_set_delay;
			ret = gmrv_register_control_path(&gmrvector_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_HEART_RATE_NUM:
			heartrate_control_path.enable_nodata = hrm_set_enable;
			heartrate_control_path.is_report_input_direct = false;
			heartrate_control_path.is_support_batch = true;
			heartrate_control_path.open_report_data = hrm_set_open_report;
			heartrate_control_path.set_delay = hrm_set_delay;
			ret = hrm_register_control_path(&heartrate_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_ACTIVITY_CLASSIFICATION_NUM:
			activity_control_path.enable_nodata = activity_set_enable;
			activity_control_path.is_report_input_direct = false;
			activity_control_path.is_support_batch = false;
			activity_control_path.open_report_data = activity_set_open_report;
			activity_control_path.set_delay = activity_set_delay;
			ret = act_register_control_path(&activity_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_SHAKE_NUM:
			shake_control_path.open_report_data = shake_set_open_report;
			ret = shk_register_control_path(&shake_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_BRING_TO_SEE_NUM:
			bringtosee_control_path.open_report_data = bts_set_open_report;
			ret = bts_register_control_path(&bringtosee_control_path);				
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}			
			break;
		case INV_BATCH_ACCELEROMETER_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_ACCELEROMETER, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_ACCELEROMETER, true, HAL_DIV_ACCELEROMETER, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_GEOMAGNETIC_FIELD_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_MAGNETIC, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_MAGNETIC, true, HAL_DIV_GEOMAGNETIC_FIELD, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_ORIENTATION_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_ORIENTATION, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_ORIENTATION, true, HAL_DIV_ORIENTATION, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_GYROSCOPE_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_GYROSCOPE, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_GYROSCOPE, true, HAL_DIV_GYROSCOPE, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_PRESSURE_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_PRESSURE, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_PRESSURE, true, HAL_DIV_PRESSURE, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_PROXIMITY_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_PROXIMITY, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_PROXIMITY, true, HAL_DIV_PROXIMITY, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_GRAVITY_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_GRAVITY, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_GRAVITY, true, HAL_DIV_GRAVITY, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_LINEAR_ACCELERATION_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_LINEAR_ACCELERATION, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_LINEAR_ACCELERATION, true, HAL_DIV_LINEAR_ACCELERATION, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_ROTATION_VECTOR_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_ROTATION_VECTOR, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_ROTATION_VECTOR, true, HAL_DIV_ROTATION_VECTOR, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_GAME_ROTATION_VECTOR_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_GAME_ROTATION_VECTOR, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_GAME_ROTATION_VECTOR, true, HAL_DIV_GAME_ROTATION_VECTOR, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_SIGNIFICANT_MOTION_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_SIGNIFICANT_MOTION, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_SIGNIFICANT_MOTION, true, HAL_DIV_SIGNIFICANT_MOTION, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_STEP_DETECTOR_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_STEP_DETECTOR, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_STEP_DETECTOR, true, HAL_DIV_STEP_DETECTOR, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_STEP_COUNTER_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_STEP_COUNTER, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_STEP_COUNTER, true, HAL_DIV_STEP_COUNTER, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_GEOMAGNETIC_ROTATION_VECTOR_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_GEOMAGNETIC_ROTATION_VECTOR, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_GEOMAGNETIC_ROTATION_VECTOR, true, HAL_DIV_GEOMAGNETIC_ROTATION_VECTOR, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_ACTIVITY_CLASSIFICATION_NUM:
			batchmode_control_path.flush = batch_flush; 		
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_ACTIVITY, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_ACTIVITY, true, HAL_DIV_ACTIVITY_CLASSIFICATION, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_HEART_RATE_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_HEART_RATE, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_HEART_RATE, true, HAL_DIV_HEART_RATE, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_SHAKE_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_SHAKE, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_SHAKE, true, HAL_DIV_SHAKE, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_BRING_TO_SEE_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(ID_BRINGTOSEE, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(ID_BRINGTOSEE, true, HAL_DIV_BRING_TO_SEE, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;
		case INV_BATCH_SENSOR_HUB_NUM:
			batchmode_control_path.flush = batch_flush;			
			batchmode_control_path.enable_hw_batch = enable_hw_batch;
			ret = batch_register_control_path(MAX_ANDROID_SENSOR_NUM, &batchmode_control_path);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}
			ret = batch_register_support_info(MAX_ANDROID_SENSOR_NUM, true, HAL_DIV_SENSOR_HUB, true);
			if(UNLIKELY(ret <0)){
				INV_ERR;
				return ret;
			}		
			break;	
		default :
			ret = -EINVAL;
			INV_ERR;
			break;
	}

	return ret;
}

u64 icm30628_get_time_ns(void)
{
	struct timespec ts;

	INV_DBG_FUNC_NAME;

	ktime_get_ts(&ts);
	
	return timespec_to_ns(&ts);
}

int icm30628_set_power(int sensorNum, int powerstatus)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2] = {0,};

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	INV_INFO("icm30628_set_power(%d, %d)\n", sensorNum, powerstatus);

	temp[0] = powerstatus;
	
	ret = invensense_send_command_to_fifo(st, sensorNum, INV_CMD_POWER, temp);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
			
	return ret;
}


int icm30628_set_sensor_batch(int sensorNum, u64 timeout)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	bool previous_batch_mode;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	INV_INFO("icm30628_set_sensor_batch(%d, %lld)\n", sensorNum, timeout);

	previous_batch_mode = st->icm30628_status.is_sensor_in_batchmode[sensorNum];

	if(timeout == 0){
		st->icm30628_status.is_sensor_in_batchmode[sensorNum] = false;
	} else {
		st->icm30628_status.is_sensor_in_batchmode[sensorNum] = true;	
	}

	st->icm30628_status.sensor_batching_timeout[sensorNum] = timeout;

	ret = icm30628_update_set();
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}	

	return ret;
}

int icm30628_set_flush(int sensorNum)
{
	int ret = 0;
	u8 time[2] = {0,};
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	INV_INFO("icm30628_set_flush(%d)\n", sensorNum);

	ret = invensense_send_command_to_fifo(st, sensorNum, INV_CMD_FLUSH, NULL);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	if(st->icm30628_status.is_sensor_in_batchmode[sensorNum] == true){
#if (FIRMWARE_VERSION >= 220)
		time[1] = (st->icm30628_status.sensor_batching_timeout[sensorNum] >> 8) & 0xFF;
		time[0] = st->icm30628_status.sensor_batching_timeout[sensorNum] & 0xFF;
#else
		time[0] = (st->icm30628_status.sensor_batching_timeout[sensorNum] >> 8) & 0xFF;
		time[1] = st->icm30628_status.sensor_batching_timeout[sensorNum] & 0xFF;
#endif		
		ret = invensense_send_command_to_fifo(st, sensorNum, INV_CMD_BATCH, time);
		if(UNLIKELY(ret <0)){
			INV_ERR;
			return ret;
		}
	}

	return ret;
}

int icm30628_set_sensor_onoff(int sensorNum, bool enable)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	bool previous_enable;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	INV_INFO("icm30628_set_sensor_onoff(%d, %d)\n", sensorNum, enable);

	previous_enable = st->icm30628_status.is_sensor_enabled[sensorNum];
	st->icm30628_status.is_sensor_enabled[sensorNum] = enable;

	if(sensorNum == INV_ACTIVITY_CLASSIFICATION_NUM){
		st->icm30628_status.activity_current_status = STILL;
	}

	ret = icm30628_update_set();
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}	

	return ret;
}

int icm30628_set_delay(int sensorNum, u64 delay_ns)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	bool previous_delay;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	INV_INFO("icm30628_set_delay(%d, %lld)\n", sensorNum, delay_ns);

	if(delay_ns == 0){
		delay_ns = LONG_MAX; //disable
	}

	if(INV_HEART_RATE_NUM == sensorNum){
		delay_ns = 50000000; // heart rate sensor needs 20hz as default
	}

	if(INV_STEP_COUNTER_NUM == sensorNum
		|| INV_STEP_DETECTOR_NUM == sensorNum
		|| INV_SIGNIFICANT_MOTION_NUM== sensorNum
		|| INV_ACTIVITY_CLASSIFICATION_NUM == sensorNum
		|| INV_SHAKE_NUM == sensorNum
		|| INV_BRING_TO_SEE_NUM== sensorNum){
		delay_ns = 10000000; //100hz, accel based sensors need 100hz
	}

	previous_delay = st->icm30628_status.sensor_sampling_rate[sensorNum];
	st->icm30628_status.sensor_sampling_rate[sensorNum] = delay_ns; 

	ret = icm30628_update_set();
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}	
		
	return ret;
}

int icm30628_update_set(void)
{
	
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	bool sensor_enable[INV_SENSOR_MAX_NUM] = {false,};
	u64 sensor_sampling_rate_ms[INV_SENSOR_MAX_NUM] = {LONG_MAX,};
	u64 sensor_batching_timeout_ms[INV_SENSOR_MAX_NUM] = {LONG_MAX,};
	int cmd = INV_CMD_INVALID;
	u8 time_ms[2] = {0};
	int i;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	for(i = 0 ; i < INV_SENSOR_MAX_NUM ; i++){
		sensor_enable[i] = st->icm30628_status.is_sensor_enabled[i];
	}
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
	if(st->icm30628_status.is_sensor_enabled[INV_GRAVITY_NUM]){
		sensor_enable[INV_GAME_ROTATION_VECTOR_NUM] = true;
	}
	if(st->icm30628_status.is_sensor_enabled[INV_LINEAR_ACCELERATION_NUM]){
		sensor_enable[INV_ACCELEROMETER_NUM] = true;
		sensor_enable[INV_GAME_ROTATION_VECTOR_NUM] = true;
	}
	if(st->icm30628_status.is_sensor_enabled[INV_ORIENTATION_NUM]){
		sensor_enable[INV_ROTATION_VECTOR_NUM] = true;
	}
#endif	
	for(i = 0; i < INV_SENSOR_MAX_NUM ; i ++){		
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
		if(i == INV_GRAVITY_NUM
			|| i == INV_LINEAR_ACCELERATION_NUM 
			|| i == INV_ORIENTATION_NUM ){
			continue;
		}
#endif
		cmd = sensor_enable[i]? INV_CMD_SENSOR_ON : INV_CMD_SENSOR_OFF;
		ret = invensense_send_command_to_fifo(st, i, cmd, NULL);
		if(UNLIKELY(ret <0)){
			INV_ERR;
			return ret;
		}
	}

	for(i = 0 ; i < INV_SENSOR_MAX_NUM ; i++){
		sensor_sampling_rate_ms[i] = st->icm30628_status.sensor_sampling_rate[i];
		do_div(sensor_sampling_rate_ms[i],1000000);
		if(sensor_sampling_rate_ms[i] < 10 && sensor_sampling_rate_ms[i] > 0){
			sensor_sampling_rate_ms[i] = 10;
		}
		if(sensor_sampling_rate_ms[i] > 0xFFFF){
			sensor_sampling_rate_ms[i] = 0xFFFF;
		}
	}
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
	if(st->icm30628_status.is_sensor_enabled[INV_GRAVITY_NUM]){
		sensor_sampling_rate_ms[INV_GAME_ROTATION_VECTOR_NUM] = 
			MIN(sensor_sampling_rate_ms[INV_GRAVITY_NUM], 
			sensor_sampling_rate_ms[INV_GAME_ROTATION_VECTOR_NUM]);
	}
	if(st->icm30628_status.is_sensor_enabled[INV_LINEAR_ACCELERATION_NUM]){
		sensor_sampling_rate_ms[INV_ACCELEROMETER_NUM] = 
			MIN(sensor_sampling_rate_ms[INV_ACCELEROMETER_NUM], 
			sensor_sampling_rate_ms[INV_LINEAR_ACCELERATION_NUM]);
		sensor_sampling_rate_ms[INV_GAME_ROTATION_VECTOR_NUM] = 
			MIN(sensor_sampling_rate_ms[INV_GAME_ROTATION_VECTOR_NUM], 
			sensor_sampling_rate_ms[INV_LINEAR_ACCELERATION_NUM]);
	}
	if(st->icm30628_status.is_sensor_enabled[INV_ORIENTATION_NUM]){
		sensor_sampling_rate_ms[INV_ROTATION_VECTOR_NUM] = 
			MIN(sensor_sampling_rate_ms[INV_ROTATION_VECTOR_NUM], 
			sensor_sampling_rate_ms[INV_ORIENTATION_NUM]);
	}
#endif	
	for(i = 0; i < INV_SENSOR_MAX_NUM ; i ++){
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
		if(i == INV_GRAVITY_NUM
			|| i == INV_LINEAR_ACCELERATION_NUM 
			|| i == INV_ORIENTATION_NUM ){
			continue;
		}
#endif
#if (FIRMWARE_VERSION >= 220)
		time_ms[1] = (sensor_sampling_rate_ms[i] >> 8) & 0xFF;
		time_ms[0] = sensor_sampling_rate_ms[i] & 0xFF;	
#else
		time_ms[0] = (sensor_sampling_rate_ms[i] >> 8) & 0xFF;
		time_ms[1] = sensor_sampling_rate_ms[i] & 0xFF;
#endif		
		ret = invensense_send_command_to_fifo(st, i, INV_CMD_SET_DELAY, time_ms);
		if(UNLIKELY(ret <0)){
			INV_ERR;
			return ret;
		}
	}

	for(i = 0 ; i < INV_SENSOR_MAX_NUM ; i++){
		sensor_batching_timeout_ms[i] = 
			st->icm30628_status.is_sensor_in_batchmode[i]? st->icm30628_status.sensor_batching_timeout[i] : 0;
		if(sensor_batching_timeout_ms[i] < 10 && sensor_batching_timeout_ms[i] > 0){
			sensor_batching_timeout_ms[i] = 10;
		}
		if(sensor_batching_timeout_ms[i] > 0xFFFF){
			sensor_batching_timeout_ms[i] = 0xFFFF;
		}
	}
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION	
	if(st->icm30628_status.is_sensor_enabled[INV_GRAVITY_NUM]
		&& st->icm30628_status.is_sensor_enabled[INV_GAME_ROTATION_VECTOR_NUM]){
		sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM] = 
			MIN(sensor_batching_timeout_ms[INV_GRAVITY_NUM], 
			sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM]);
	}
	if(st->icm30628_status.is_sensor_enabled[INV_GRAVITY_NUM]
		&& !st->icm30628_status.is_sensor_enabled[INV_GAME_ROTATION_VECTOR_NUM]){
		sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM] = sensor_batching_timeout_ms[INV_GRAVITY_NUM];
	}	
	if(st->icm30628_status.is_sensor_enabled[INV_LINEAR_ACCELERATION_NUM]
		&& st->icm30628_status.is_sensor_enabled[INV_ACCELEROMETER_NUM]
		&& st->icm30628_status.is_sensor_enabled[INV_GAME_ROTATION_VECTOR_NUM]){
		sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM] = 
			MIN(sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM], 
			sensor_batching_timeout_ms[INV_LINEAR_ACCELERATION_NUM]);
		sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM] = 
			MIN(sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM], 
			sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM]);
		sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM] = 
			sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM];
	}
	if(st->icm30628_status.is_sensor_enabled[INV_LINEAR_ACCELERATION_NUM]
		&& !st->icm30628_status.is_sensor_enabled[INV_ACCELEROMETER_NUM]
		&& st->icm30628_status.is_sensor_enabled[INV_GAME_ROTATION_VECTOR_NUM]){
		sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM] = 
			MIN(sensor_batching_timeout_ms[INV_LINEAR_ACCELERATION_NUM], 
			sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM]);		
		sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM] = 
			sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM];
	}
	if(st->icm30628_status.is_sensor_enabled[INV_LINEAR_ACCELERATION_NUM]
		&& st->icm30628_status.is_sensor_enabled[INV_ACCELEROMETER_NUM]
		&& !st->icm30628_status.is_sensor_enabled[INV_GAME_ROTATION_VECTOR_NUM]){
		sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM] = 
			MIN(sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM], 
			sensor_batching_timeout_ms[INV_LINEAR_ACCELERATION_NUM]);		
		sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM] = 
			sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM];
	}
	if(st->icm30628_status.is_sensor_enabled[INV_LINEAR_ACCELERATION_NUM]
		&& !st->icm30628_status.is_sensor_enabled[INV_ACCELEROMETER_NUM]
		&& !st->icm30628_status.is_sensor_enabled[INV_GAME_ROTATION_VECTOR_NUM]){
		sensor_batching_timeout_ms[INV_ACCELEROMETER_NUM] = 
			sensor_batching_timeout_ms[INV_LINEAR_ACCELERATION_NUM];					
		sensor_batching_timeout_ms[INV_GAME_ROTATION_VECTOR_NUM] = 
			sensor_batching_timeout_ms[INV_LINEAR_ACCELERATION_NUM];
	}	
	if(st->icm30628_status.is_sensor_enabled[INV_ORIENTATION_NUM]
		&& st->icm30628_status.is_sensor_enabled[INV_ROTATION_VECTOR_NUM]){
		sensor_batching_timeout_ms[INV_ROTATION_VECTOR_NUM] = 
			MIN(sensor_batching_timeout_ms[INV_ROTATION_VECTOR_NUM], 
			sensor_batching_timeout_ms[INV_ORIENTATION_NUM]);
	}
	if(st->icm30628_status.is_sensor_enabled[INV_ORIENTATION_NUM]
		&& !st->icm30628_status.is_sensor_enabled[INV_ROTATION_VECTOR_NUM]){
		sensor_batching_timeout_ms[INV_ROTATION_VECTOR_NUM] = 
			sensor_batching_timeout_ms[INV_ORIENTATION_NUM];
	}
#endif	
	for(i = 0; i < INV_SENSOR_MAX_NUM ; i ++){		
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
		if(i == INV_GRAVITY_NUM
			|| i == INV_LINEAR_ACCELERATION_NUM 
			|| i == INV_ORIENTATION_NUM ){
			continue;
		}
#endif
		sensor_batching_timeout_ms[i] = invensense_batch_threshold_calculation(st, sensor_batching_timeout_ms[i]);	
#if (FIRMWARE_VERSION >= 220)
		time_ms[1] = (sensor_batching_timeout_ms[i] >> 8) & 0xFF;
		time_ms[0] = sensor_batching_timeout_ms[i] & 0xFF; 
#else
		time_ms[0] = (sensor_batching_timeout_ms[i] >> 8) & 0xFF;
		time_ms[1] = sensor_batching_timeout_ms[i] & 0xFF;
#endif		
		ret = invensense_send_command_to_fifo(st, i, INV_CMD_BATCH, time_ms);
		if(UNLIKELY(ret <0)){
			INV_ERR;
			return ret;
		}
	}

#ifndef POLLING_MODE 
	invensense_start_watchdog_work_func();
#endif

	return ret;
}

int icm30628_m0_enable(bool enable)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2] = {0};

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	ret = icm30628_set_LP_enable(false);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_MOD_EN_B0, 1, temp);		
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		ret |= icm30628_set_LP_enable(true);
		return ret;
	}
#ifdef SPI_INTERFACE	
	temp[0] = temp[0] | GARNET_M0_EN_BIT | GARNET_I2C_IF_DIS_BIT;
#else
	temp[0] = temp[0] | GARNET_M0_EN_BIT;
#endif
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_MOD_EN_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		ret |= icm30628_set_LP_enable(true);
		return ret;
	}

	ret = icm30628_set_LP_enable(true);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
			
	return ret;
}

int icm30628_kick_m0_interrupt(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2];
#if (FIRMWARE_VERSION >= 220)
	u16 fifo_length;
	int cnt = 0;
#endif

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#if (FIRMWARE_VERSION >= 220)
	do {		
		if(cnt > 0){
			msleep(10);
		}
		
		ret = invensense_bank_read(st, GARNET_REG_BANK_1, SCRATCH_M0_INT_STATUS, 1, temp);			
		if (UNLIKELY(ret < 0)) {
			INV_ERR;
			return ret;
		}
		temp[0] |= 0x80;
		ret = invensense_bank_write(st, GARNET_REG_BANK_1, SCRATCH_M0_INT_STATUS, 1, temp); 		
		if (UNLIKELY(ret < 0)) {
			INV_ERR;
			return ret;
		}
	
		ret = invensense_get_fifo_status(st, GARNET_FIFO_ID_CMDIN, &fifo_length);
		if(UNLIKELY(ret <0)){
			INV_ERR;
			fifo_length = 0;
		}
		
		cnt++;
	} while(fifo_length > 0 && cnt < 200); // 10ms x 200 = 2sec
	if(fifo_length > 0){
		icm30628_reset_fifo(GARNET_FIFO_ID_CMDIN);
	}
#else
	ret = invensense_bank_read(st, GARNET_REG_BANK_1, SCRATCH_M0_INT_STATUS, 1, temp);			
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] |= 0x80;
	ret = invensense_bank_write(st, GARNET_REG_BANK_1, SCRATCH_M0_INT_STATUS, 1, temp);			
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
#endif

	return ret;
}

int icm30628_set_deep_sleep(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2];

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef ENABLE_DEEP_SLEEP    
	temp[0] = GARNET_DEEP_SLEEP_EN_BIT | GARNET_SLEEP_REQ_BIT;
#else
	temp[0] = GARNET_SLEEP_REQ_BIT;
#endif
	ret = invensense_reg_write(st, GARNET_PWR_MGMT_1_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_set_sleep(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2];

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	ret = invensense_reg_read(st, GARNET_PWR_MGMT_1_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
	}
	temp[0] &= ~GARNET_SLEEP_REQ_BIT;
#ifdef ENABLE_DEEP_SLEEP    
	temp[0] |= GARNET_DEEP_SLEEP_EN_BIT;	
#endif
	temp[0] |= GARNET_LP_EN_BIT;
	ret |= invensense_reg_write(st, GARNET_PWR_MGMT_1_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
	}

	return ret;
}

int icm30628_wake_up_m0(void) 
{ 
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2];

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_reg_read(st, GARNET_PWR_MGMT_1_B0, 1, temp); 
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

#ifdef ENABLE_DEEP_SLEEP    
	if(temp[0] & (GARNET_SLEEP_REQ_BIT | GARNET_DEEP_SLEEP_EN_BIT) )
#else
	if(temp[0] & (GARNET_SLEEP_REQ_BIT) )
#endif
	{
		temp[0] &= ~GARNET_SLEEP_REQ_BIT; 
#ifdef ENABLE_DEEP_SLEEP    
		temp[0] &= ~GARNET_DEEP_SLEEP_EN_BIT;
#endif		
		ret = invensense_reg_write(st, GARNET_PWR_MGMT_1_B0, 1, temp); 
		if (UNLIKELY(ret < 0)) {
			INV_ERR;
			return ret;
		}
	}

	msleep(10);

	return ret; 
} 

int icm30628_factory_sensor_enable(int sensorNum, bool enable)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_sensor_onoff(sensorNum, enable);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_delay(sensorNum, 10000000); // 100ms as default
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_factory_get_sensor_data(int sensorNum, void * data)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	if(st->is_download_done){
		invensense_fifo_read();
	}
#endif

	switch(sensorNum)
	{
		case INV_ACCELEROMETER_NUM:
			memcpy(data, st->icm30628_status.accel_current_data, sizeof(st->icm30628_status.accel_current_data));
			INV_INFO("icm30628_factory_get_sensor_data : accel = %d, %d, %d\n", *(int *)data, *(int *)(data + 4), *(int *)(data + 8));
			break;
		case INV_GYROSCOPE_NUM:
			memcpy(data, st->icm30628_status.gyro_current_data, sizeof(st->icm30628_status.gyro_current_data));
			INV_INFO("icm30628_factory_get_sensor_data : gyro = %d, %d, %d\n", *(int *)data, *(int *)(data + 4), *(int *)(data + 8));
			break;
		case INV_MAGNETIC_FIELD_NUM:
			memcpy(data, st->icm30628_status.mag_current_data, sizeof(st->icm30628_status.mag_current_data));
			INV_INFO("icm30628_factory_get_sensor_data : mag = %d, %d, %d\n", *(int *)data, *(int *)(data + 4), *(int *)(data + 8));
			break;
		case INV_PRESSURE_NUM:
			memcpy(data, st->icm30628_status.baro_current_data, sizeof(st->icm30628_status.baro_current_data));
			INV_INFO("icm30628_factory_get_sensor_data : pressure = %d, %d, %d\n", *(int *)data, *(int *)(data + 4), *(int *)(data + 8));
			break;
		case INV_PROXIMITY_NUM:
			memcpy(data, st->icm30628_status.proximity_current_data, sizeof(st->icm30628_status.proximity_current_data));
			INV_INFO("icm30628_factory_get_sensor_data : proxmity = %d, %d, %d\n", *(int *)data, *(int *)(data + 4), *(int *)(data + 8));
			break;
		case INV_HEART_RATE_NUM:
			memcpy(data, st->icm30628_status.hrm_current_data, sizeof(st->icm30628_status.hrm_current_data));
			INV_INFO("icm30628_factory_get_sensor_data : hrm = %d, %d, %d\n", *(int *)data, *(int *)(data + 4), *(int *)(data + 8));
			break;
		default:
			ret = -EINVAL;
			INV_ERR;
			break;
	}

	return ret;
}

int icm30628_factory_clear_calibrator_data(int sensorNum)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	// to-do

	return ret;
}

int icm30628_factory_get_calibrator_data(int sensorNum, int * data)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	// to-do

	return ret;
}

int icm30628_factory_set_calibrator_data(int sensorNum, int * data)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	// to-do

	return ret;
}

int icm30628_set_LP_enable(int enable)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
#ifdef ENABLE_LP_EN	
	u8 temp[2];
	u8 bank, reg_addr;
#endif

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef ENABLE_LP_EN	
	if(st->is_lp_enabled == enable) return 0;

	bank = (GARNET_PWR_MGMT_1_B0 >> 8) & 0xFF;
	reg_addr = GARNET_PWR_MGMT_1_B0 & 0xFF;
	ret = invensense_bank_read(st, bank, reg_addr, 1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	
	if(enable) {	
		temp[0] |= GARNET_LP_EN_BIT;
	} else {
		temp[0] &= ~GARNET_LP_EN_BIT;
	}

	ret = invensense_bank_write(st, bank, reg_addr, 1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	st->is_lp_enabled = enable;
#endif

	return ret;
}

int icm30628_init_serial(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
#ifdef SPI_INTERFACE	
	u8 temp[2];
#endif

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef SPI_INTERFACE	
	temp[0] = GARNET_I2C_IF_DIS_BIT;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_MOD_EN_B0, 1, temp);			
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
#endif

	return ret;
}

int icm30628_reset_fifo(u8 fifo_id)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2];

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	temp[0] = (u8) (1 << fifo_id);
	ret = invensense_reg_write(st, GARNET_FIFO_RST_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
	
	temp[0] = 0x00;
	ret = invensense_reg_write(st, GARNET_FIFO_RST_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}


int icm30628_configure_fifo(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2];

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_reg_read(st, GARNET_MOD_EN_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
	
#ifdef SPI_INTERFACE	
	temp[0] = temp[0] | GARNET_SERIF_FIFO_EN_BIT | GARNET_I2C_IF_DIS_BIT;
#else
	temp[0] = temp[0] | GARNET_SERIF_FIFO_EN_BIT;
#endif
	ret |= invensense_reg_write(st, GARNET_MOD_EN_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0x00;
	ret |= invensense_reg_write(st, GARNET_FIFO_DATA_RDY_INT_EN_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0x00;
	ret |= invensense_reg_write(st, GARNET_FIFO_0_PKT_SIZE_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret |= invensense_reg_write(st, GARNET_FIFO_1_PKT_SIZE_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0xFF;
	ret |= invensense_reg_write(st, GARNET_FIFO_0_SIZE_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0x5F;
	ret |= invensense_reg_write(st, GARNET_FIFO_1_SIZE_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0x03;
	ret |= invensense_reg_write(st, GARNET_FIFO_PKT_SIZE_OVERRIDE_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret |= invensense_reg_read(st, GARNET_FIFO_MODE_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = temp[0] | 0x03;
	ret |= invensense_reg_write(st, GARNET_FIFO_MODE_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret |= invensense_reg_read(st, GARNET_MOD_CTRL_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = temp[0] & ~GARNET_SERIF_FIFO_DIS_BIT;
	ret |= invensense_reg_write(st, GARNET_MOD_CTRL_B0, 1, temp);	
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret |= invensense_reg_read(st, GARNET_MOD_CTRL2_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = temp[0] | GARNET_FIFO_EMPTY_IND_DIS_BIT;
	ret |= invensense_reg_write(st, GARNET_MOD_CTRL2_B0, 1, temp);	
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0x03;
	ret |= invensense_reg_write(st, GARNET_FIFO_RST_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0x00;
	ret |= invensense_reg_write(st, GARNET_FIFO_RST_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_configure_for_debugger(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	unsigned char data;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_LP_enable(false);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
	
	data = GARNET_TEST_MODES_SKU_TYPE_BIT | GARNET_TEST_MODES_DAP_EN_BIT;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0,GARNET_TEST_MODES_B0,1,&data);
	if (ret < 0) {
		INV_ERR;
		ret |= icm30628_set_LP_enable(true);
		return ret;
	}

	ret = icm30628_set_LP_enable(true);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}


int icm30628_set_ivory_power(u8 enable)
{
	int ret = 0;	
	struct icm30628_state_t * st = icm30628_state;
	u8 regData = enable ? 0x00 : IVORY_BIT_SLEEP;
	u8 temp = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	temp = 0x00;
	ret = invensense_mems_reg_write(st, IVORY_REG_BANK_SEL, 1, &temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = invensense_mems_reg_write(st, IVORY_PWR_MGMT_1, 1, &regData);							
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	if(enable)
		msleep(100);

	return ret;
}

int icm30628_ivory_register_read(struct icm30628_state_t * st, u8 ivoryBank, u8 regAddr, u8 len, u8 *data)
{
	int ret = 0;	
	u8 bank;
	
	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	bank = ivoryBank << 4;
	ret = invensense_mems_reg_write(st, IVORY_REG_BANK_SEL, 1, &bank);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = invensense_mems_reg_read(st, regAddr, len, data);							
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_ivory_whoami(void)
{
	int ret = 0;	
	struct icm30628_state_t * st = icm30628_state;
	u8 buffer[128] = {0};
	
	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_ivory_register_read(st, GARNET_REG_BANK_0, 0x00, 1, buffer);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	INV_INFO("IVORY WHOAMI = 0x%x\n", buffer[0]);

	return ret;
}

int icm30628_garnet_whoami(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 data[10];

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_reg_read(st, GARNET_WHOAMI_B0, 1, data);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}					

	INV_INFO("GARNET WHOAMI = 0x%x\n", data[0]);

	return ret;
}

int icm30628_configure_spi_master(u8 numBytesForCh0)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[4] = {0};

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	temp[0] = IVORY_I2C_IF_DIS_BIT;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_MOD_EN_B0, 1, temp);			
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_MOD_EN_B0, 1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	
	temp[0] |= GARNET_SPI_MST_EN_BIT;	
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_MOD_EN_B0, 1, temp);	
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_MOD_CTRL_B0, 1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	temp[0] &= ~GARNET_SPI_MST_DIS_BIT;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_MOD_CTRL_B0, 1, temp);	
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0x00;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_SEC_INTF_SLV_INT_CFG_B0, 1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	temp[0] = 0x1F;
	ret = invensense_bank_mem_write(st, GARNET_GPIO_START_ADDR+0x18,1,temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	
	ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_SCRATCH_INT_EN_B0, 1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	
	temp[0] |= GARNET_RAW_DATA_RDY_INT_PIN_BIT;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0,GARNET_SCRATCH_INT_EN_B0,1,temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = invensense_bank_read(st, GARNET_REG_BANK_1, GARNET_M0_INT_ENABLE_B1, 1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	
	temp[0] |= GARNET_CH_DATA_RDY_M0_INT_EN_BIT;		
	ret = invensense_bank_write(st, GARNET_REG_BANK_1,GARNET_M0_INT_ENABLE_B1,1,temp);	
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	temp[0] = numBytesForCh0;
	temp[0] |= (GARNET_SEC_INTF_CH0_MST_MAP_BITS_SPI |GARNET_SEC_INTF_CH0_EN_BIT );	
	ret = invensense_bank_write(st, GARNET_REG_BANK_0,GARNET_SEC_INTF_CH0_CONFIG_B0,1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_get_calibration_gain(int sensorNum)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	
	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_send_command_to_fifo(st, sensorNum, INV_CMD_GET_CALIBRATION_GAINS, NULL);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_set_calibration_gain(int sensorNum, u8 * gain)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	
	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_send_command_to_fifo(st, sensorNum, INV_CMD_SET_CALIBRATION_GAINS, gain);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_get_calibration_offsets(int sensorNum)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	
	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_send_command_to_fifo(st, sensorNum, INV_CMD_GET_CALIBRATION_OFFSETS, NULL);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_set_calibration_offsets(int sensorNum, u8 * offsets)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_send_command_to_fifo(st, sensorNum, INV_CMD_SET_CALIBRATION_OFFSETS, offsets);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_configure_reference_frame(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[SIZE_REFERENCE_FRAME] = {0,};
	
	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	memcpy(temp, st->icm30628_orientation_matrix, SIZE_REFERENCE_FRAME);

	ret = invensense_send_command_to_fifo(st, INV_ACCELEROMETER_NUM, INV_CMD_SET_REFERENCE_FRAME, temp);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = invensense_send_command_to_fifo(st, INV_GYROSCOPE_NUM, INV_CMD_SET_REFERENCE_FRAME, temp);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	memcpy(temp, st->icm30628_compass_orientation_matrix, SIZE_REFERENCE_FRAME);

	ret = invensense_send_command_to_fifo(st, INV_MAGNETIC_FIELD_NUM, INV_CMD_SET_REFERENCE_FRAME, temp);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_get_firmware_info(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_send_command_to_fifo(st, INV_DUMMY_SENSOR_ID, INV_CMD_GET_FIRMWARE_INFO, NULL);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_get_data(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_send_command_to_fifo(st, INV_DUMMY_SENSOR_ID, INV_CMD_GET_DATA, NULL);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_ping(int sensorNum)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_send_command_to_fifo(st, sensorNum, INV_CMD_PING, NULL);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

#ifdef DMP_DOWNLOAD
int icm30628_initiate_dmp_load(u8 dmp_type, u32 dmpsize, u32 * address)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 data[8] = {0};
	bool *dmp_initiated = 0x00;
	u32 *dmp_address = 0x00;
	u32 cnt = 0;
#ifdef POLLING_MODE 
	u8 interrupt_status = 0;
	u8 interrupt;
#endif

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	INV_INFO("icm30628_initiate_dmp_load start %d\n", dmp_type);

	if(dmp_type == INV_DMP_TYPE_DMP3){
		data[2] = FIFOPROTOCOL_LOAD_WHO_DMP3_FW;
		dmp_initiated = &st->dmp3_initiated;
		dmp_address = &st->dmp3_address;
	} else if(dmp_type == INV_DMP_TYPE_DMP4){
		data[2] = FIFOPROTOCOL_LOAD_WHO_DMP4_FW;
		dmp_initiated = &st->dmp4_initiated;
		dmp_address = &st->dmp4_address;
	} else {
		return -1;
	}

	data[0] = INV_META_DATA_NUM;
	data[1] = INV_CMD_LOAD;
	if(dmp_type == INV_DMP_TYPE_DMP3){
		data[2] = FIFOPROTOCOL_LOAD_WHO_DMP3_FW;
		dmp_initiated = &st->dmp3_initiated;
		dmp_address = &st->dmp3_address;
	} else if(dmp_type == INV_DMP_TYPE_DMP4){
		data[2] = FIFOPROTOCOL_LOAD_WHO_DMP4_FW;
		dmp_initiated = &st->dmp4_initiated;
		dmp_address = &st->dmp4_address;
	} else {
		return -1;
	}	
	data[3] = FIFOPROTOCOL_LOAD_WHAT_MALLOC;
#if (FIRMWARE_VERSION >= FIRMWARE_VERSION_3_2_2)
	data[4] = dmpsize & 0xFF;								
	data[5] = dmpsize >> 8;
	data[6] = dmpsize >> 16;
	data[7] = dmpsize >> 24;	
#else
	data[4] = dmpsize >> 24;								
	data[5] = dmpsize >> 16;
	data[6] = dmpsize >> 8;
	data[7] = dmpsize & 0xFF;
#endif
	ret = invensense_fifo_write(st, 8, data);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	do{
		invensense_get_interrupt_status(GARNET_SCRATCH_INT0_STATUS_B0, &interrupt_status);
		interrupt = interrupt_status;
		invensense_get_interrupt_status(GARNET_SCRATCH_INT1_STATUS_B0, &interrupt_status);
		interrupt |= interrupt_status;
		msleep(1);
		cnt++;
	} while(interrupt == 0 && cnt < 1000);
	invensense_fifo_read();
	if(!(*dmp_initiated)){
		INV_INFO("icm30628_load_dmp_code error, timeout %d\n", dmp_type);
		return -1;
	}	
#else	
	do
	{
		msleep(1);
		cnt++;
	} while (!(*dmp_initiated) && cnt < 1000);
	if(!(*dmp_initiated)){
		INV_INFO("icm30628_initiate_dmp_load error, timeout %d\n", dmp_type);
		return -1;
	}
#endif
	*address = *dmp_address;
	INV_INFO("icm30628_initiate_dmp_load end %d\n", dmp_type);

	return ret;
}

int icm30628_load_dmp_code(u8 dmp_type, u32 address, u8 * dmp, u32 dmpsize)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 data[8] = {0};
	bool *dmp_checked = 0x00;
	u32 cnt = 0;
#ifdef POLLING_MODE 
	u8 interrupt_status = 0;
	u8 interrupt;
#endif

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st || !dmp)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	INV_INFO("icm30628_load_dmp_code start %d\n", dmp_type);

	ret = invensense_memory_write(st, address, dmp, dmpsize);
	if (ret) {
		INV_ERR;
		return ret;
	}
	
	data[0] = INV_META_DATA_NUM;
	data[1] = INV_CMD_LOAD;
	if(dmp_type == INV_DMP_TYPE_DMP3){
		data[2] = FIFOPROTOCOL_LOAD_WHO_DMP3_FW;
		dmp_checked = &st->dmp3_checked;
	} else if(dmp_type == INV_DMP_TYPE_DMP4){
		data[2] = FIFOPROTOCOL_LOAD_WHO_DMP4_FW;
		dmp_checked = &st->dmp4_checked;
	} else {
		return -1;
	}
	data[3] = FIFOPROTOCOL_LOAD_WHAT_CHECK;
	data[4] = 0x00;
	data[5] = 0x00;
	data[6] = 0x00;
	data[7] = 0x00;			
	ret = invensense_fifo_write(st, 8, data);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

#ifdef POLLING_MODE 
	cnt = 0;
	do{
		invensense_get_interrupt_status(GARNET_SCRATCH_INT0_STATUS_B0, &interrupt_status);
		interrupt = interrupt_status;
		INV_INFO("invensense_get_interrupt0_status interrupt=%d\n", interrupt_status);
		invensense_get_interrupt_status(GARNET_SCRATCH_INT1_STATUS_B0, &interrupt_status);
		INV_INFO("invensense_get_interrupt1_status interrupt=%d\n", interrupt_status);
		interrupt |= interrupt_status;
		msleep(1);
		cnt++;
	} while(interrupt == 0 && cnt < 1000);
	invensense_fifo_read();
	if(!(*dmp_checked)){
		INV_INFO("icm30628_load_dmp_code error, timeout %d\n", dmp_type);
		return -1;
	}
#else	
	do
	{
		msleep(1);
		cnt++;
	} while (!(*dmp_checked) && cnt < 1000);
	if(!(*dmp_checked)){
		INV_INFO("icm30628_load_dmp_code error, timeout %d\n", dmp_type);
		return -1;
	}
#endif
	INV_INFO("icm30628_load_dmp_code end %d\n", dmp_type);

	return ret;
}
#endif

int icm30628_erase_flash(u8 page)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	unsigned char current_value[2] = {0};
	unsigned int erase_time = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_LP_enable(false);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_FLASH_CFG_B0, 1, current_value);
	if (ret < 0) {
		INV_ERR;
		ret = icm30628_set_LP_enable(true);
		if (UNLIKELY(ret < 0)) {
			INV_ERR;
			return ret;
		}
		return ret;
	}
	current_value[0] = current_value[0] | GARNET_FLASH_IFRN_DIS_BIT;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_FLASH_CFG_B0, 1, current_value);
	if (ret < 0) {
		INV_ERR;
		ret = icm30628_set_LP_enable(true);
		if (UNLIKELY(ret < 0)) {
			INV_ERR;
			return ret;
		}
		return ret;
	}
		
	if (page > GARNET_MAX_FLASH_PAGE_ADDRESS)
	{
		current_value[0] = GARNET_FLASH_ERASE_MASS_EN_BIT;		
		ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_FLASH_ERASE_B0, 1, current_value);
		if (ret < 0) {
			INV_ERR;
			ret = icm30628_set_LP_enable(true);
			if (UNLIKELY(ret < 0)) {
				INV_ERR;
				return ret;
			}
			return ret;
		}
		do{
			ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_FLASH_ERASE_B0, 1, current_value);			
			if (ret < 0) {
				INV_ERR;
				ret = icm30628_set_LP_enable(true);
				if (UNLIKELY(ret < 0)) {
					INV_ERR;
					return ret;
				}
				return ret;
			}
			msleep(1);
			erase_time++;			
		} while (current_value[0] & GARNET_FLASH_ERASE_MASS_EN_BIT);
	}	

	ret = icm30628_set_LP_enable(true);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_soft_reset(void)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 temp[2] = {0};

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	temp[0] = GARNET_SOFT_RESET_BIT;		
	ret = invensense_bank_write(st, GARNET_REG_BANK_0,GARNET_PWR_MGMT_1_B0, 1, temp);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	msleep(10);

	ret =icm30628_wake_up_m0();
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_dma_initialize(unsigned char dmaChannel, unsigned int sourceAddr, unsigned int destAddr, unsigned int numBytes )
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	u8 controlRegDMA_bytes[4] = { 
		GARNET_DMA_CONTROL_REGISTER_BYTE_0_WORD_SIZE_BITS,
		GARNET_DMA_CONTROL_REGISTER_BYTE_1_MAX_BURST_BITS,
		(GARNET_DMA_CONTROL_REGISTER_BYTE_2_CHG_BIT | GARNET_DMA_CONTROL_REGISTER_BYTE_2_STRT_BIT),
		(GARNET_DMA_CONTROL_REGISTER_BYTE_3_INT_BIT | GARNET_DMA_CONTROL_REGISTER_BYTE_3_TC_BIT |GARNET_DMA_CONTROL_REGISTER_BYTE_3_SINC_BIT |GARNET_DMA_CONTROL_REGISTER_BYTE_3_DINC_BIT)};	
	unsigned int dmaAddr = GARNET_DMA_CH_0_START_ADDR + dmaChannel * GARNET_DMA_CHANNEL_ADDRESS_OFFSET;	
	unsigned char dmaSourceDestAddrs[8] = {0};
	unsigned char dmaLength[4] = {0};
	unsigned char int_status[2] = {0};
	int num_reads = 0;
	
	inv_int32_to_little8(sourceAddr, dmaSourceDestAddrs);
	inv_int32_to_little8(destAddr, &dmaSourceDestAddrs[4]);

	ret = invensense_bank_mem_write(st, dmaAddr, 8, dmaSourceDestAddrs);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	
	inv_int32_to_little8(numBytes,dmaLength);		
	ret = invensense_bank_mem_write(st, dmaAddr+GARNET_DMA_TRANSFER_COUNT_OFFSET, 4, dmaLength);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = invensense_bank_mem_write(st, dmaAddr+GARNET_DMA_CONTROL_REGISTER_BYTE_0_OFFSET, 2, controlRegDMA_bytes);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}
	ret = invensense_bank_mem_write(st, dmaAddr+GARNET_DMA_CONTROL_REGISTER_BYTE_3_OFFSET, 1, &controlRegDMA_bytes[3]);				
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	msleep(100);
	
	ret = invensense_bank_mem_write(st, dmaAddr+GARNET_DMA_CONTROL_REGISTER_BYTE_2_OFFSET, 1, &controlRegDMA_bytes[2]);						
	if (ret < 0) {
		INV_ERR;
		return ret;
	}


#if 1 //temporary, TODO: check why SPI fail
	// temporary
	msleep(1000);
#else
	do
	{
		ret = invensense_bank_mem_read(st, GARNET_DMA_INTERRUPT_REGISTER, 1, int_status);
		if (ret) {
			INV_ERR;
			return ret;
		}
		msleep(10);
		num_reads++;
	} while (!(int_status[0] & (1 << dmaChannel)));

	do
	{
		ret = invensense_bank_read(st, GARNET_REG_BANK_0,GARNET_IDLE_STATUS_B0, 1, int_status);
		if (ret) {
			INV_ERR;
			return ret;
		}
		msleep(1);
		num_reads++;
	} while (!(int_status[0] & (GARNET_FLASH_IDLE_BIT | GARNET_FLASH_LOAD_DONE_BIT)));
#endif

	return ret;
}

#if 1 //test
void icm30628_sensor_test(void)
{
	int i;
	unsigned char data[10];
	int data_int[4] = {0,};

	msleep(6000);
			
	icm30628_factory_sensor_enable(INV_ACCELEROMETER_NUM,true);
	icm30628_factory_sensor_enable(INV_GYROSCOPE_NUM,true);
	icm30628_factory_sensor_enable(INV_MAGNETIC_FIELD_NUM,true);
	icm30628_factory_sensor_enable(INV_PROXIMITY_NUM,true);
	icm30628_factory_sensor_enable(INV_PRESSURE_NUM,true);
	icm30628_factory_sensor_enable(INV_HEART_RATE_NUM,true);

	msleep(3000);
	icm30628_factory_get_sensor_data(INV_ACCELEROMETER_NUM, data_int);
	icm30628_factory_get_sensor_data(INV_GYROSCOPE_NUM, data_int);
	icm30628_factory_get_sensor_data(INV_MAGNETIC_FIELD_NUM, data_int);
	icm30628_factory_get_sensor_data(INV_PROXIMITY_NUM, data_int);
	icm30628_factory_get_sensor_data(INV_PRESSURE_NUM, data_int);
	icm30628_factory_get_sensor_data(INV_HEART_RATE_NUM, data_int);

	printk("icm30628_sensor_test \n");
		
	hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_3300, "sensor");
	hwPowerOn(MT6323_POWER_LDO_VGP3, VOL_1800, "sensor");
	printk("enable VGP1 VPG3 LDO \n");


	msleep(30000); //need to review this code...
	
	icm30628_garnet_whoami();
	
	/*
	data[0] = 1;
	invensense_bank_write(st, GARNET_REG_BANK_0, 0x7f, 1, data);
	data[0] = 0xFF;
	invensense_bank_read(st, GARNET_REG_BANK_0, 0x7f, 1, data);
	printk("read = 0x%x\n", data[0]);
data[0] = 0;
	invensense_bank_write(st, GARNET_REG_BANK_0, 0x7f, 1, data);
	data[0] = 0xFF;
	invensense_bank_read(st, GARNET_REG_BANK_0, 0x7f, 1, data);
	printk("read = 0x%x\n", data[0]);
		*/
	
	
	icm30628_set_sensor_onoff(INV_ACCELEROMETER_NUM, true);	
	icm30628_set_delay(INV_ACCELEROMETER_NUM, 100000000);

	for(i = 0 ; i < 20 ; i ++){	
		printk("accel count = %d\n", i);
		msleep(200);
	invensense_fifo_read();
	}

	icm30628_set_sensor_onoff(INV_GYROSCOPE_NUM, true);	
	icm30628_set_delay(INV_GYROSCOPE_NUM, 100000000);
	
	for(i = 0 ; i < 20 ; i ++){	
		printk("gyro count = %d\n", i);
		msleep(200);
	invensense_fifo_read();
	}

	icm30628_set_sensor_onoff(INV_HEART_RATE_NUM, true);	
	icm30628_set_delay(INV_HEART_RATE_NUM, 100000000);

	icm30628_set_sensor_onoff(INV_PRESSURE_NUM, true);	
	icm30628_set_delay(INV_PRESSURE_NUM, 100000000);

	icm30628_set_sensor_onoff(INV_PROXIMITY_NUM, true);	
	icm30628_set_delay(INV_PROXIMITY_NUM, 100000000);

	icm30628_set_sensor_onoff(INV_GAME_ROTATION_VECTOR_NUM, true);	
	icm30628_set_delay(INV_GAME_ROTATION_VECTOR_NUM, 100000000);

	icm30628_set_sensor_onoff(INV_MAGNETIC_FIELD_NUM, true);	
	icm30628_set_delay(INV_MAGNETIC_FIELD_NUM, 100000000);

	icm30628_set_sensor_onoff(INV_STEP_DETECTOR_NUM, true);	
	icm30628_set_delay(INV_STEP_DETECTOR_NUM, 100000000);

	icm30628_set_sensor_onoff(INV_STEP_COUNTER_NUM, true);	
	icm30628_set_delay(INV_STEP_COUNTER_NUM, 100000000);

	for(i = 0 ; i < 20 ; i ++){	
		printk("count = %d\n", i);
		msleep(200);
	invensense_fifo_read();
	}

	return;
}
#endif

int icm30628_load_flash_image(u8 *flash_image, int flash_size)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st || !flash_image)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_garnet_whoami();
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}					

	INV_INFO("firmware downloading...\n");

	ret = invensense_memory_write(st, GARNET_SRAM_START_ADDR, flash_image, flash_size);
	if (ret<0) {
		INV_ERR;
		return ret;
	}
		
	ret = icm30628_erase_flash(65);
	if (ret<0) {
		INV_ERR;
		return ret;
	}

	ret = icm30628_soft_reset();
	if (ret<0) {
		INV_ERR;
		return ret;
	}

	ret = icm30628_dma_initialize(1,GARNET_SRAM_START_ADDR, GARNET_FLASH_START_ADDR, flash_size );			
	if (ret<0) {
		INV_ERR;
		return ret;
	}

	ret = icm30628_init_serial();
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}	

	ret = icm30628_configure_fifo();
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}	

	ret = icm30628_m0_enable(true);
	if (ret < 0) {
		INV_ERR;
		return ret;
	}

	ret = icm30628_configure_for_debugger();
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}	

	ret = icm30628_set_LP_enable(false);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}

	msleep(5);

	ret = icm30628_set_deep_sleep();
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_set_sleep();
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}	

	st->is_firmware_download_done = true;

	INV_INFO("firmware downloading... done\n");

	icm30628_get_firmware_info();
	
	return 0;
}

int icm30628_driver_add(void)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = acc_driver_add(&icm30628_accelerometer_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = gyro_driver_add(&icm30628_gyroscope_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = mag_driver_add(&icm30628_magnetometer_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = grav_driver_add(&icm30628_gravity_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = la_driver_add(&icm30628_linearaccel_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = grv_driver_add(&icm30628_grv_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = gmrv_driver_add(&icm30628_gmrv_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = rotationvector_driver_add(&icm30628_rotationvector_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = baro_driver_add(&icm30628_baro_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = alsps_driver_add(&icm30628_proximity_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = hrm_driver_add(&icm30628_heartrate_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = shk_driver_add(&icm30628_shake_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = bts_driver_add(&icm30628_bts_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = act_driver_add(&icm30628_activity_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}
		
	ret = step_c_driver_add(&icm30628_step_c_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	ret = batch_driver_add(&icm30628_batch_info);	
	if(UNLIKELY(ret <0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int icm30628_wakelock(bool lock)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(lock == false){
		wake_unlock(&st->sleep_lock);
	} else {
		wake_lock(&st->sleep_lock);
	}

	return ret;
}

int invensense_accel_bias_found(struct icm30628_state_t * st, bool found)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(found == st->is_accel_calibrated){
		return 0;
	}

	st->is_accel_calibrated = found;

	if(!found){
		return 0;
	}

	ret = icm30628_get_calibration_gain(INV_ACCELEROMETER_NUM);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret = icm30628_get_calibration_offsets(INV_ACCELEROMETER_NUM);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_gyro_bias_found(struct icm30628_state_t * st, bool found)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(found == st->is_gyro_calibrated){
		return 0;
	}

	st->is_gyro_calibrated = found;

	if(!found){
		return 0;
	}

	ret = icm30628_get_calibration_gain(INV_GYROSCOPE_NUM);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret = icm30628_get_calibration_offsets(INV_GYROSCOPE_NUM);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_compass_bias_found(struct icm30628_state_t * st, bool found)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(found == st->is_mag_calibrated){
		return 0;
	}

	st->is_mag_calibrated = found;

	if(!found){
		return 0;
	}

	ret = icm30628_get_calibration_gain(INV_MAGNETIC_FIELD_NUM);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret = icm30628_get_calibration_offsets(INV_MAGNETIC_FIELD_NUM);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

int Invensense_send_signal_to_daemon(struct icm30628_state_t * st, int signal)
{
	int ret = 0;
	struct siginfo info;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!st->daemon_t)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
 	memset(&info, 0, sizeof(struct siginfo));
	info.si_signo = SIG_ICM30628;
	info.si_code = SI_QUEUE;
	info.si_int = signal;

	ret = send_sig_info(SIG_ICM30628, &info, st->daemon_t);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	return ret;
}

u16 invensense_get_sensor_id(u16 sensor_num)
{
	int i;

	INV_DBG_FUNC_NAME;

	if(sensor_num >= INV_SENSOR_MAX_NUM){
		return INV_SENSOR_INVALID;
	}

	for(i = 0 ; i < INV_SENSOR_MAX_NUM ; i++){
		if(icm30628_id_table[i].sensor == sensor_num)
			return icm30628_id_table[i].sensor_id;
	}

	return INV_SENSOR_INVALID_NUM;
}

u16 invensense_get_sensor_num(u16 sensor_id)
{
	int i;
	
	INV_DBG_FUNC_NAME;

	for(i = 0 ; i < INV_SENSOR_MAX_NUM ; i++){
		if(icm30628_id_table[i].sensor_id == sensor_id)
			return icm30628_id_table[i].sensor;
	}
	
	return INV_SENSOR_INVALID_NUM;
}

u16 invensense_get_wakeup_sensor_id(u16 sensorNum)
{
	int i;
	u16 sensorID = INV_SENSOR_INVALID;
	
	INV_DBG_FUNC_NAME;

	sensorID = invensense_get_sensor_id(sensorNum);

	for(i = 0 ; i < INV_SENSOR_MAX_NUM ; i++){
		if(icm30628_id_table[i].sensor_id == sensorID)
			return icm30628_id_table[i].sensor | INV_WAKEUP_SENSOR_FLAG;
	}

	return INV_SENSOR_INVALID_NUM;
}

u16 invensense_get_sensor_id_from_wakeup_id(u16 wakeupSensorID)
{	
	INV_DBG_FUNC_NAME;

	return wakeupSensorID & (~INV_WAKEUP_SENSOR_FLAG);
}


int invensense_get_fifo_command_size(int cmd)
{
	int size = 0;

	INV_DBG_FUNC_NAME;

	switch(cmd){
		case INV_CMD_SENSOR_OFF:
			size = SIZE_ID + SIZE_CMD;
			break;
		case INV_CMD_SENSOR_ON:
			size = SIZE_ID + SIZE_CMD;
			break;
		case INV_CMD_POWER:
			size = 4;
			break;
		case INV_CMD_BATCH:
			size = 4;
			break;
		case INV_CMD_FLUSH:
			size = SIZE_ID + SIZE_CMD;
			break;
		case INV_CMD_SET_DELAY:
			size = 4;
			break;
		case INV_CMD_SET_CALIBRATION_GAINS:
			size = SIZE_ID + SIZE_CMD + SIZE_CALIBRATION_GAIN;
			break;
		case INV_CMD_GET_CALIBRATION_GAINS:
			size = SIZE_ID + SIZE_CMD;
			break;
		case INV_CMD_SET_CALIBRATION_OFFSETS:
			size = SIZE_ID + SIZE_CMD + SIZE_CALIBRATION_OFFSET;
			break;
		case INV_CMD_GET_CALIBRATION_OFFSETS:
			size = SIZE_ID + SIZE_CMD;
			break;
		case INV_CMD_SET_REFERENCE_FRAME:
			size = SIZE_ID + SIZE_CMD + SIZE_REFERENCE_FRAME;
			break;
		case INV_CMD_GET_FIRMWARE_INFO:
			size = SIZE_ID + SIZE_CMD;
			break;		
		case INV_CMD_GET_DATA:
			size = SIZE_ID + SIZE_CMD;
			break;
		case INV_CMD_GET_CLOCK_RATE:
			size = SIZE_ID + SIZE_CMD;
			break;
		case INV_CMD_PING:
			size = SIZE_ID + SIZE_CMD;
			break;
#ifdef DMP_DOWNLOAD
		case INV_CMD_LOAD:
			size = SIZE_ID + SIZE_CMD + SIZE_DMP_INFO;
			break;
#endif			
		default:
			size = -1;
			break;
	}

	return size;
}

int invensense_get_batch_data_size(int sensorNum)
{
	int size = 0;

	INV_DBG_FUNC_NAME;

	switch(sensorNum){
		case INV_ACCELEROMETER_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_GYROSCOPE_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_MAGNETIC_FIELD_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_ORIENTATION_NUM:
			size = SIZE_HDR + SIZE_STAMP + SIZE_1AXIS * AXIS_NUM;
			break;
		case INV_GRAVITY_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_LINEAR_ACCELERATION_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_GAME_ROTATION_VECTOR_NUM:
			size = SIZE_QUAT_PACKET;
			break;
		case INV_GEOMAGNETIC_ROTATION_VECTOR_NUM:
			size = SIZE_QUAT_PACKET;
			break;
		case INV_ROTATION_VECTOR_NUM:
			size = SIZE_QUAT_PACKET;
			break;
		case INV_HEART_RATE_NUM:
			size = SIZE_HDR + SIZE_STAMP + SIZE_1AXIS;
			break;
		case INV_PRESSURE_NUM:
			size = SIZE_1AXIS_PACKET;
			break;
		case INV_PROXIMITY_NUM:
			size = SIZE_1AXIS_16BIT_PACKET;
			break;
		case INV_STEP_COUNTER_NUM:
			size = SIZE_STEP_COUNTER_PACKET;
			break;
		case INV_STEP_DETECTOR_NUM:
			size =  SIZE_EVENT_PACKET;
			break;
		case INV_SIGNIFICANT_MOTION_NUM:
			size = SIZE_EVENT_PACKET;
			break;
		case INV_ACTIVITY_CLASSIFICATION_NUM:
			size =  SIZE_BAC_PACKET;
			break;
		case INV_SHAKE_NUM:
			size = SIZE_EVENT_PACKET;
			break;
		case INV_BRING_TO_SEE_NUM:
			size = SIZE_EVENT_PACKET + SIZE_PAYLOAD;;
			break;
		default:
			size = -1;
			break;
	}

	return size;
}


int invensense_get_fifo_data_size(int sensorNum)
{
	int size = 0;

	INV_DBG_FUNC_NAME;

	switch(sensorNum){
		case INV_ACCELEROMETER_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_GYROSCOPE_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_MAGNETIC_FIELD_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_ORIENTATION_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_GRAVITY_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_LINEAR_ACCELERATION_NUM:
			size = SIZE_NORMAL_PACKET;
			break;
		case INV_GAME_ROTATION_VECTOR_NUM:
			size = SIZE_QUAT_PACKET;
			break;
		case INV_GEOMAGNETIC_ROTATION_VECTOR_NUM:
			size = SIZE_QUAT_PACKET;
			break;
		case INV_ROTATION_VECTOR_NUM:
			size = SIZE_QUAT_PACKET;
			break;
		case INV_HEART_RATE_NUM:
			size = SIZE_HRM_PACKET;
			break;
		case INV_PRESSURE_NUM:
			size = SIZE_1AXIS_PACKET;
			break;
		case INV_PROXIMITY_NUM:
			size = SIZE_1AXIS_16BIT_PACKET;
			break;
		case INV_STEP_COUNTER_NUM:
			size = SIZE_STEP_COUNTER_PACKET;
			break;
		case INV_STEP_DETECTOR_NUM:
			size =  SIZE_EVENT_PACKET;
			break;
		case INV_SIGNIFICANT_MOTION_NUM:
			size = SIZE_EVENT_PACKET;
			break;
		case INV_ACTIVITY_CLASSIFICATION_NUM:
			size =  SIZE_BAC_PACKET;
			break;
		case INV_SHAKE_NUM:
			size = SIZE_EVENT_PACKET;
			break;
		case INV_BRING_TO_SEE_NUM:
			size = SIZE_EVENT_PACKET + SIZE_PAYLOAD;
			break;
		default:
			size = -1;
			break;
	}

	return size;
}

int invensense_get_handle_by_sensorNum(int sensorNum)
{
	int handle = -1;

	INV_DBG_FUNC_NAME;

	switch(sensorNum){
		case INV_ACCELEROMETER_NUM:
			handle = ID_ACCELEROMETER;
			break;
		case INV_GYROSCOPE_NUM:
			handle = ID_GYROSCOPE;
			break;
		case INV_MAGNETIC_FIELD_NUM:
			handle = ID_MAGNETIC;
			break;
		case INV_ORIENTATION_NUM:
			handle = ID_ORIENTATION;
			break;
		case INV_GRAVITY_NUM:
			handle = ID_GRAVITY;
			break;
		case INV_LINEAR_ACCELERATION_NUM:
			handle = ID_LINEAR_ACCELERATION;
			break;
		case INV_GAME_ROTATION_VECTOR_NUM:
			handle = ID_GAME_ROTATION_VECTOR;
			break;
		case INV_GEOMAGNETIC_ROTATION_VECTOR_NUM:
			handle = ID_GEOMAGNETIC_ROTATION_VECTOR;
			break;
		case INV_ROTATION_VECTOR_NUM:
			handle = ID_ROTATION_VECTOR;
			break;
		case INV_HEART_RATE_NUM:
			handle = ID_HEART_RATE;
			break;
		case INV_PRESSURE_NUM:
			handle = ID_PRESSURE;
			break;
		case INV_PROXIMITY_NUM:
			handle = ID_PROXIMITY;
			break;
		case INV_STEP_COUNTER_NUM:
			handle = ID_STEP_COUNTER;
			break;
		case INV_STEP_DETECTOR_NUM:
			handle = ID_STEP_DETECTOR;
			break;
		case INV_SIGNIFICANT_MOTION_NUM:
			handle = ID_SIGNIFICANT_MOTION;
			break;
		case INV_ACTIVITY_CLASSIFICATION_NUM:
			handle = ID_ACTIVITY;
			break;
		case INV_SHAKE_NUM:
			handle = ID_SHAKE;
			break;
		case INV_BRING_TO_SEE_NUM:
			handle = ID_BRINGTOSEE;
			break;
		default:
			handle = -1;
			break;
	}

	return handle;
}


int invensense_get_sensorNum_by_handle(int handle)
{
	int sensorNum = INV_SENSOR_INVALID_NUM;

	INV_DBG_FUNC_NAME;
	
	switch(handle){
		case ID_ACCELEROMETER:
			sensorNum = INV_ACCELEROMETER_NUM;
			break;
		case ID_GYROSCOPE:
			sensorNum = INV_GYROSCOPE_NUM;
			break;
		case ID_MAGNETIC:
			sensorNum = INV_MAGNETIC_FIELD_NUM;
			break;
		case ID_ORIENTATION:
			sensorNum = INV_ORIENTATION_NUM;
			break;
		case ID_GRAVITY:
			sensorNum = INV_GRAVITY_NUM;
			break;
		case ID_LINEAR_ACCELERATION:
			sensorNum = INV_LINEAR_ACCELERATION_NUM;
			break;
		case ID_GAME_ROTATION_VECTOR:
			sensorNum = INV_GAME_ROTATION_VECTOR_NUM;
			break;
		case ID_GEOMAGNETIC_ROTATION_VECTOR:
			sensorNum = INV_GEOMAGNETIC_ROTATION_VECTOR_NUM;
			break;
		case ID_ROTATION_VECTOR:
			sensorNum = INV_ROTATION_VECTOR_NUM;
			break;
		case ID_HEART_RATE:
			sensorNum = INV_HEART_RATE_NUM;
			break;
		case ID_PRESSURE:
			sensorNum = INV_PRESSURE_NUM;
			break;
		case ID_PROXIMITY:
			sensorNum = INV_PROXIMITY_NUM;
			break;
		case ID_STEP_COUNTER:
			sensorNum = INV_STEP_COUNTER_NUM;
			break;
		case ID_STEP_DETECTOR:
			sensorNum = INV_STEP_DETECTOR_NUM;
			break;
		case ID_SIGNIFICANT_MOTION:
			sensorNum = INV_SIGNIFICANT_MOTION_NUM;
			break;
		case ID_ACTIVITY:
			sensorNum = INV_ACTIVITY_CLASSIFICATION_NUM;
			break;
		case ID_SHAKE:
			sensorNum = INV_SHAKE_NUM;
			break;
		case ID_BRINGTOSEE:
			sensorNum = INV_BRING_TO_SEE_NUM;
			break;
		default:
			sensorNum = INV_SENSOR_INVALID_NUM;
			break;
	}
	
	return sensorNum;
}

int invensense_get_sensorNum_by_fifo_data(u8* data)
{
	int ret = 0;
	int sensorID = INV_SENSOR_INVALID;
	int sensorNum = INV_SENSOR_INVALID_NUM;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	sensorID = data[0];

	switch(sensorID){
		case INV_ACCELEROMETER:
			sensorNum = INV_ACCELEROMETER_NUM;
			break;
		case INV_GYROSCOPE:
			sensorNum = INV_GYROSCOPE_NUM;
			break;
		case INV_MAGNETIC_FIELD:
			sensorNum = INV_MAGNETIC_FIELD_NUM;
			break;
		case INV_ORIENTATION:
			sensorNum = INV_ORIENTATION_NUM;
			break;
		case INV_GRAVITY:
			sensorNum = INV_GRAVITY_NUM;
			break;
		case INV_LINEAR_ACCELERATION:
			sensorNum = INV_LINEAR_ACCELERATION_NUM;
			break;
		case INV_GAME_ROTATION_VECTOR:
			sensorNum = INV_GAME_ROTATION_VECTOR_NUM;
			break;
		case INV_GEOMAGNETIC_ROTATION_VECTOR:
			sensorNum = INV_GEOMAGNETIC_ROTATION_VECTOR_NUM;
			break;
		case INV_ROTATION_VECTOR:
			sensorNum = INV_ROTATION_VECTOR_NUM;
			break;
		case INV_HEART_RATE:
			sensorNum = INV_HEART_RATE_NUM;
			break;
		case INV_PRESSURE:
			sensorNum = INV_PRESSURE_NUM;
			break;
		case INV_PROXIMITY:
			sensorNum = INV_PROXIMITY_NUM;
			break;
		case INV_STEP_COUNTER:
			sensorNum = INV_STEP_COUNTER_NUM;
			break;
		case INV_STEP_DETECTOR:
			sensorNum = INV_STEP_DETECTOR_NUM;
			break;
		case INV_SIGNIFICANT_MOTION:
			sensorNum = INV_SIGNIFICANT_MOTION_NUM;
			break;
		case INV_ACTIVITY_CLASSIFICATION:
			sensorNum = INV_ACTIVITY_CLASSIFICATION_NUM;
			break;
		case INV_SHAKE:
			sensorNum = INV_SHAKE_NUM;
			break;
		case INV_BRING_TO_SEE:
			sensorNum = INV_BRING_TO_SEE_NUM;
			break;
		default:
			sensorNum = INV_SENSOR_INVALID_NUM;
			break;
	}

	return sensorNum;
}

int invensense_get_div_by_handle(int handle)
{
	int div = 1;

	INV_DBG_FUNC_NAME;
	
	switch(handle){
		case ID_ACCELEROMETER:
			div = HAL_DIV_ACCELEROMETER;
			break;
		case ID_GYROSCOPE:
			div = HAL_DIV_GYROSCOPE;
			break;
		case ID_MAGNETIC:
			div = HAL_DIV_GEOMAGNETIC_FIELD;
			break;
		case ID_ORIENTATION:
			div = HAL_DIV_ORIENTATION;
			break;
		case ID_GRAVITY:
			div = HAL_DIV_GRAVITY;
			break;
		case ID_LINEAR_ACCELERATION:
			div = HAL_DIV_LINEAR_ACCELERATION;
			break;
		case ID_GAME_ROTATION_VECTOR:
			div = HAL_DIV_GAME_ROTATION_VECTOR;
			break;
		case ID_GEOMAGNETIC_ROTATION_VECTOR:
			div = HAL_DIV_GEOMAGNETIC_ROTATION_VECTOR;
			break;
		case ID_ROTATION_VECTOR:
			div = HAL_DIV_ROTATION_VECTOR;
			break;
		case ID_HEART_RATE:
			div = HAL_DIV_HEART_RATE;
			break;
		case ID_PRESSURE:
			div = HAL_DIV_PRESSURE;
			break;
		case ID_PROXIMITY:
			div = HAL_DIV_PROXIMITY;
			break;
		case ID_STEP_COUNTER:
			div = HAL_DIV_STEP_COUNTER;
			break;
		case ID_STEP_DETECTOR:
			div = HAL_DIV_STEP_DETECTOR;
			break;
		case ID_SIGNIFICANT_MOTION:
			div = HAL_DIV_SIGNIFICANT_MOTION;
			break;
		case ID_ACTIVITY:
			div = HAL_DIV_ACTIVITY_CLASSIFICATION;
			break;
		case ID_SHAKE:
			div = HAL_DIV_SHAKE;
			break;
		case ID_BRINGTOSEE:
			div = HAL_DIV_BRING_TO_SEE;
			break;
		default:
			div = 1;
			break;
	}
	
	return div;
}

int invensense_is_motion_event_sensor(int type)
{
	INV_DBG_FUNC_NAME;
	
	switch(type){
		case INV_ACCELEROMETER_NUM:
		case INV_MAGNETIC_FIELD_NUM:
		case INV_ORIENTATION_NUM:
		case INV_GRAVITY_NUM:
		case INV_LINEAR_ACCELERATION_NUM:
		case INV_GAME_ROTATION_VECTOR_NUM:
		case INV_GEOMAGNETIC_ROTATION_VECTOR_NUM:
		case INV_ROTATION_VECTOR_NUM:
		case INV_HEART_RATE_NUM:
		case INV_PRESSURE_NUM:
		case INV_PROXIMITY_NUM:
			return false;
		case INV_STEP_COUNTER_NUM:
		case INV_STEP_DETECTOR_NUM:
		case INV_SIGNIFICANT_MOTION_NUM:
		case INV_ACTIVITY_CLASSIFICATION_NUM:
		case INV_SHAKE_NUM:
		case INV_BRING_TO_SEE_NUM:
			return true;
		default:
			return -1;
	}

	return -1;
}


int invensense_orientation_matrix(struct icm30628_state_t * st)
{
	int ret = 0;
	int i;
	u32 temp_val, temp_val2;

	INV_DBG_FUNC_NAME;

#ifdef M0_ORIENTATION_MATRIX
	for (i = 0; i < 9; i++){
		st->icm30628_orientation_matrix[i] = 0;
	}

	temp_val = AXIS_MAP_X;
	temp_val2 = NEGATE_X;
	if (temp_val2)
		st->icm30628_orientation_matrix[temp_val] = -1 * (1<<30);
	else
		st->icm30628_orientation_matrix[temp_val] = 1 * (1<<30);

	temp_val = AXIS_MAP_Y;
	temp_val2 = NEGATE_Y;
	if (temp_val2)
		st->icm30628_orientation_matrix[temp_val + 3] = -1 * (1<<30);
	else
		st->icm30628_orientation_matrix[temp_val + 3] = 1 * (1<<30);

	temp_val = AXIS_MAP_Z;
	temp_val2 = NEGATE_Z;
	if (temp_val2)
		st->icm30628_orientation_matrix[temp_val + 6] = -1 * (1<<30);
	else
		st->icm30628_orientation_matrix[temp_val + 6] = 1 * (1<<30);

	for (i = 0; i < 9; i++){
		st->icm30628_compass_orientation_matrix[i] = 0;
	}

	temp_val = MAG_AXIS_MAP_X;
	temp_val2 = MAG_NEGATE_X;
	if (temp_val2)
		st->icm30628_compass_orientation_matrix[temp_val] = -1 * (1<<30);
	else
		st->icm30628_compass_orientation_matrix[temp_val] = 1 * (1<<30);

	temp_val = MAG_AXIS_MAP_Y;
	temp_val2 = MAG_NEGATE_Y;
	if (temp_val2)
		st->icm30628_compass_orientation_matrix[temp_val + 3] = -1 * (1<<30);
	else
		st->icm30628_compass_orientation_matrix[temp_val + 3] = 1 * (1<<30);

	temp_val = MAG_AXIS_MAP_Z;
	temp_val2 = MAG_NEGATE_Z;
	if (temp_val2)
		st->icm30628_compass_orientation_matrix[temp_val + 6] = -1 * (1<<30);
	else
		st->icm30628_compass_orientation_matrix[temp_val + 6] = 1 * (1<<30);
#else
	for (i = 0; i < 9; i++){
		st->icm30628_orientation[i] = 0;
	}

	temp_val = AXIS_MAP_X;
	temp_val2 = NEGATE_X;
	if (temp_val2)
		st->icm30628_orientation[temp_val] = -1;
	else
		st->icm30628_orientation[temp_val] = 1;

	temp_val = AXIS_MAP_Y;
	temp_val2 = NEGATE_Y;
	if (temp_val2)
		st->icm30628_orientation[temp_val + 3] = -1;
	else
		st->icm30628_orientation[temp_val + 3] = 1;

	temp_val = AXIS_MAP_Z;
	temp_val2 = NEGATE_Z;
	if (temp_val2)
		st->icm30628_orientation[temp_val + 6] = -1;
	else
		st->icm30628_orientation[temp_val + 6] = 1;
#endif

	return ret;
}

u32 invensense_batch_threshold_calculation(struct icm30628_state_t * st, u32 timeout)
{
	int ret = 0;
	int i;
	u32 temp;
	u32 batch_period = 0;
	u32 packet_size_per_1sec = 0;
	u32 packet_size_per_10ms = 0;
	u64 min_latency_ms = LONG_MAX;
	u64 sampleing_rate_ns = 0;
	u32 sampleing_rate_ms = 0;
	
	
	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	for( i = 0 ; i < INV_SENSOR_MAX_NUM ; i ++){
		if(st->icm30628_status.is_sensor_enabled[i] 
			&& st->icm30628_status.is_sensor_in_batchmode[i]
			&& !invensense_is_motion_event_sensor(i)){
			sampleing_rate_ns = st->icm30628_status.sensor_sampling_rate[i];
			if(sampleing_rate_ns < 10000000){
				sampleing_rate_ns = 10000000;
			}
			do_div(sampleing_rate_ns,1000000);
			sampleing_rate_ms = sampleing_rate_ns;
			packet_size_per_1sec +=  invensense_get_fifo_data_size(i) * (1000/sampleing_rate_ms);
			min_latency_ms = MIN(min_latency_ms, st->icm30628_status.sensor_batching_timeout[i]);
		}
	}	
	packet_size_per_10ms = packet_size_per_1sec/100;
	if(packet_size_per_10ms == 0){
		packet_size_per_10ms = 1;
	}
	temp = INVENSENSE_OUTPUT_FIFO_LENGTH / packet_size_per_10ms;

	if(temp < 100){
		batch_period = 10;
	} else {
		batch_period = (temp - 100) * 10;
	}

	batch_period = MIN(batch_period, timeout);

	return batch_period;
}


int invensense_apply_16_orientation(struct icm30628_state_t * st, const u8 src[])
{
	int ret = 0;
	short tmp[AXIS_NUM];
	int oriented[AXIS_NUM];
	int i = 0;

#ifdef M0_ORIENTATION_MATRIX
	return 0;
#endif
	
	INV_DBG_FUNC_NAME;

	if (UNLIKELY(!src)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	tmp[0] = ((short) src[0] << 8) + src[1];
	tmp[1] = ((short) src[2] << 8) + src[3];
	tmp[2] = ((short) src[4] << 8) + src[5];

	for (i = 0; i < AXIS_NUM; i++) {
		oriented[i] = tmp[0] * st->icm30628_orientation[i * 3 + 0] 
			+ tmp[1] *  st->icm30628_orientation[i * 3 + 1] 
			+ tmp[2] *  st->icm30628_orientation[i * 3 + 2];
	}

	for (i = 0; i < AXIS_NUM ; i++){
		tmp[i] = (short) oriented[i];
	}
	memcpy((void *) src, (void *) tmp, sizeof(tmp));

	return ret;
}

int invensense_apply_32_orientation(struct icm30628_state_t * st, const u8 src[])
{
	int ret = 0;
	int tmp[AXIS_NUM];
	long oriented[AXIS_NUM];
	int i = 0;

#ifdef M0_ORIENTATION_MATRIX
	return 0;
#endif
	
	INV_DBG_FUNC_NAME;

	if (UNLIKELY(!src)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	tmp[0] = *(int*)&src[0];
	tmp[1] = *(int*)&src[4];
	tmp[2] = *(int*)&src[8];

	for (i = 0; i < AXIS_NUM; i++) {
		oriented[i] = tmp[0] * st->icm30628_orientation[i * 3 + 0] 
			+ tmp[1] * st->icm30628_orientation[i * 3 + 1] 
			+ tmp[2] * st->icm30628_orientation[i * 3 + 2];
	}

	for (i = 0; i < AXIS_NUM; i++){
		tmp[i] = (int) oriented[i];
	}
	memcpy((void *) src, (void *) tmp, sizeof(tmp));

	return ret;
}

int invensense_apply_quat_orientation(struct icm30628_state_t * st, int * src)
{
	int ret = 0;
	long q1[4], q2[4];

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef M0_ORIENTATION_MATRIX
	return 0;
#endif

	q1[0] = src[3];
	q1[1] = src[0];
	q1[2] = src[1];
	q1[3] = src[2];
	inv_q_mult((const long *)q1, (const long *)st->icm30628_quat_chip_to_body, q2); 		
	src[0] = q2[1];
	src[1] = q2[2];
	src[2] = q2[3];
	src[3] = q2[0];
	
	return ret;
}

#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
int invensense_convert_quat_to_gravity(int * gravity, int * quat)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!gravity || !quat)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	gravity[0] = (2 * inv_q30_mult(quat[0], quat[2])
		- 2 * inv_q30_mult(quat[3], quat[1])) >> 18;
	gravity[1] = (2 * inv_q30_mult(quat[1], quat[2])
		+ 2 * inv_q30_mult(quat[3], quat[0])) >> 18;
	gravity[2] = ((1 << 30) - 2 * inv_q30_mult(quat[0], quat[0]) 
		- 2 * inv_q30_mult(quat[1], quat[1])) >>18;

	return ret;
}

int invensense_convert_quat_and_accel_to_linearAccel(int * linearaccel, int * quat, int * accel)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!linearaccel || !quat || !accel)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	linearaccel[0] = accel[0] - ((2 * inv_q30_mult(quat[0], quat[2])
		- 2 * inv_q30_mult(quat[3], quat[1])) >> 18);
	linearaccel[1] = accel[1] - ((2 * inv_q30_mult(quat[1], quat[2])
		+ 2 * inv_q30_mult(quat[3], quat[0])) >> 18);
	linearaccel[2] = accel[2] - (((1 << 30) - 2 * inv_q30_mult(quat[0], quat[0]) 
		- 2 * inv_q30_mult(quat[1], quat[1])) >>18);

	return ret;
}

int invensense_request_to_build_orientation(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = Invensense_send_signal_to_daemon(st, REQUEST_SIGNAL_PROCESS_ORIENTATION);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}					

	return ret;
}
#endif

int invensense_request_to_build_heartrate(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = Invensense_send_signal_to_daemon(st, REQUEST_SIGNAL_PROCESS_HRM);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}					

	return ret;
}

int invensense_request_load_calibration(struct icm30628_state_t * st)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = Invensense_send_signal_to_daemon(st, REQUEST_SIGNAL_LOAD_CALIBRATION);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}					

	return ret;
}

int invensense_request_store_calibration(struct icm30628_state_t * st)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	ret = Invensense_send_signal_to_daemon(st, REQUEST_SIGNAL_STORE_CALIBRATION);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}					

	return ret;
}


int invensense_whoami(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = icm30628_ivory_whoami();
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}					

	ret = icm30628_garnet_whoami();
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}					

	return ret;
}

int invensense_kfifo_allocation(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(st->kfifo_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	st->kfifo_buffer = kzalloc(sizeof(struct kfifo), GFP_KERNEL);
	if (UNLIKELY(!st->kfifo_buffer)) {
		INV_ERR;
		return -ENOMEM;
	}			

	ret = kfifo_alloc(st->kfifo_buffer, INVENSENSE_KFIFO_LENGTH, GFP_KERNEL);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return -ENOMEM;
	}					

#ifdef USING_WORKQUEUE_FOR_SENDING_COMMAND
	if(UNLIKELY(st->kfifo_command_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	st->kfifo_command_buffer = kzalloc(sizeof(struct kfifo), GFP_KERNEL);
	if (UNLIKELY(!st->kfifo_command_buffer)) {
		INV_ERR;
		return -ENOMEM;
	}			

	ret = kfifo_alloc(st->kfifo_command_buffer, INVENSENSE_COMMAND_KFIFO_LENGTH, GFP_KERNEL);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return -ENOMEM;
	}
#endif

	return ret;
}

int invensense_kfifo_free(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!st->kfifo_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	kfifo_free(st->kfifo_buffer);
	st->kfifo_buffer = NULL;
	
	return ret;
}


int invensense_kfifo_in(struct icm30628_state_t * st, u8 * data)
{
	int ret = 0;	
	int length = 0;
	int sensorNum = INV_SENSOR_INVALID_NUM;
 
	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!st->kfifo_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	sensorNum = invensense_get_sensorNum_by_fifo_data(data);
	if(UNLIKELY(sensorNum == INV_SENSOR_INVALID_NUM)){
		ret = -1;
		INV_ERR;
		return ret;
	}

	length = invensense_get_batch_data_size(sensorNum);
	if(UNLIKELY(length < 0)){
		ret = -1;
		INV_ERR;
		return ret;
	}

	if(kfifo_len(st->kfifo_buffer) > INVENSENSE_KFIFO_LENGTH - length){
		batch_notify(TYPE_BATCHFULL);
	}

	ret = kfifo_in(st->kfifo_buffer, data, length);	
	if(UNLIKELY(!ret)){
		ret = -1;
		INV_ERR;
		return ret;
	} else {
		ret = 0;
	}

	st->icm30628_status.kfifo_size++;

	return ret;
}

int invensense_kfifo_out(struct icm30628_state_t * st, u8 * data)
{
	int ret = 0;
	int length = 0;
	int sensorNum = INV_SENSOR_INVALID_NUM;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!st->kfifo_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(st->icm30628_status.kfifo_size == 0){
		ret = -1;
		return ret;		
	}

	ret = kfifo_out(st->kfifo_buffer, data, 2); 
	if(!ret){
		ret = -1;
		INV_ERR;
		return ret;
	} else {
		ret = 0;
	}

	sensorNum = invensense_get_sensorNum_by_fifo_data(data);
	if(UNLIKELY(sensorNum == INV_SENSOR_INVALID_NUM)){
		ret = -1;
		INV_ERR;
		return ret;
	}

	length = invensense_get_batch_data_size(sensorNum);	
	if(UNLIKELY(length < 0)){
		ret = -1;
		INV_ERR;
		return ret;
	}

	ret = kfifo_out(st->kfifo_buffer, &data[2], length -2); 
	if(!ret){
		ret = -1;
		//INV_ERR;
		return ret;
	} else {
		ret = 0;
	}

	st->icm30628_status.kfifo_size--;

	return 0;
}

int invensense_kfifo_reset(struct icm30628_state_t * st)
{
	int ret = 0;
	
	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!st->kfifo_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	kfifo_reset(st->kfifo_buffer); 

	st->icm30628_status.kfifo_size = 0;

	return 0;
}

int invensense_get_interrupt_status(u8 interrupt, u8 * status)
{
	int ret = 0;
	u8 temp[3];
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	ret = invensense_bank_read(st, GARNET_REG_BANK_0, interrupt, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	*status = temp[0];

	return ret;
}

static void invensense_clear_INT0_status_work(struct work_struct *work)
{
	struct icm30628_state_t * st = icm30628_state;
	u8 interrupt_status;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		return;
	}

	invensense_get_interrupt_status(GARNET_SCRATCH_INT0_STATUS_B0, &interrupt_status);

	return;
}

void invensense_start_clear_INT0_status_work(void)
{
	struct icm30628_state_t * st = icm30628_state;

	if(UNLIKELY(!st)){
		return;
	}

	schedule_work(&st->clear_int0_work);	

	return;
}

static void invensense_clear_INT1_status_work(struct work_struct *work)
{
	struct icm30628_state_t * st = icm30628_state;
	u8 interrupt_status;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		return;
	}

	invensense_get_interrupt_status(GARNET_SCRATCH_INT1_STATUS_B0, &interrupt_status);

	return;
}

void invensense_start_clear_INT1_status_work(void)
{
	struct icm30628_state_t * st = icm30628_state;

	if(UNLIKELY(!st)){
		return;
	}

	schedule_work(&st->clear_int1_work);	

	return;
}

u64 invensense_get_time_ns(void)
{
	struct timespec ts;

	INV_DBG_FUNC_NAME;

	ktime_get_ts(&ts);
	
	return timespec_to_ns(&ts);
}

static void invensense_watchdog_work_func(struct work_struct *work)
{
	struct icm30628_state_t * st = icm30628_state;
	bool sensor_is_running =  false;
	u64 min_period = LONG_MAX;
	u64 dt;
	int i;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		return;
	}

	dt = invensense_get_time_ns() - st->watchdog_time;

	for(i = 0 ; i < INV_SENSOR_MAX_NUM ; i++){
		if(st->icm30628_status.is_sensor_enabled[i]){
			sensor_is_running = true;
			if(st->icm30628_status.is_sensor_in_batchmode[i]){
				if(st->icm30628_status.sensor_batching_timeout[i] * 1000000L > st->icm30628_status.sensor_sampling_rate[i]){
					min_period = MIN(min_period, st->icm30628_status.sensor_batching_timeout[i] * 1000000L);
				} else {
					min_period = MIN(min_period, st->icm30628_status.sensor_sampling_rate[i]);
				}
			} else {
				min_period = MIN(min_period, st->icm30628_status.sensor_sampling_rate[i]);
			}
		}
	}

	if(dt > 15000000L && sensor_is_running){
		invensense_start_clear_INT0_status_work();
		invensense_start_clear_INT1_status_work();
	}

	if(sensor_is_running) {
		do_div(min_period, 1000000);
		if(min_period < 10){
			min_period = 10;
		}
		schedule_delayed_work(&st->watchdog_work, msecs_to_jiffies(min_period));
	} else {		
		st->is_watchdog_running = false;
	}
	
	return;
}

void invensense_start_watchdog_work_func(void)
{
	struct icm30628_state_t * st = icm30628_state;

	if(UNLIKELY(!st)){
		return;
	}

	if(st->is_watchdog_running){
		return;
	} else {
		st->is_watchdog_running = true;
	}

	schedule_delayed_work(&st->watchdog_work, msecs_to_jiffies(200));

	return;
}



int invensense_get_fifo_status(struct icm30628_state_t * st, u8 fifo_id, u16 * length)
{
	int ret = 0;
	u8 temp[3];

	INV_DBG_FUNC_NAME

	if(UNLIKELY(!st || !length)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	temp[0] = fifo_id;
	ret = invensense_bank_write(st, GARNET_REG_BANK_0, GARNET_FIFO_INDEX_B0, 1, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
	
	ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_FIFO_COUNTH_B0, 2, temp);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
	*length  = (u16) (temp[0] << 8);
	*length |= (u16) temp[1];

	return ret;
}

int invensense_fifo_write(struct icm30628_state_t * st, int length, u8 * data)
{
	int ret = 0;
	u16 bytesWritten = 0;
	u8 temp[16 /*GARNET_MAX_FIFO_WRITE*/ +1];
	int i;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	mutex_lock(&st->fifolock);

	ret = icm30628_set_LP_enable(false);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		mutex_unlock(&st->fifolock);
		return ret;
	}

	temp[0] = GARNET_FIFO_ID_CMDIN;
	ret = invensense_reg_write(st, GARNET_FIFO_INDEX_B0, 1, temp);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		ret |= icm30628_set_LP_enable(true);
		mutex_unlock(&st->fifolock);
		return ret;
	}

	while (bytesWritten < length) {
		u16 thisLen = MIN(16 /*GARNET_MAX_FIFO_WRITE*/, length-bytesWritten);
		memcpy(temp, &data[bytesWritten], thisLen);
		ret = invensense_reg_write(st, GARNET_FIFO_R_W_B0, thisLen, temp);
		if (ret != 0) break;
		bytesWritten += thisLen;
	}

	INV_INFO("input fifo = { ");
	for(i = 0 ; i < length ; i ++){
		INV_INFO2("%3d ",  data[i]);
	}
	INV_INFO2("}\n");

	printk("input fifo = { ");
	for(i = 0 ; i < length ; i ++){
		printk("%3d ",  data[i]);
	}
	printk("}\n");
	
	ret = icm30628_kick_m0_interrupt();
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		ret |= icm30628_set_LP_enable(true);
		mutex_unlock(&st->fifolock);
		return ret;
	}

	ret = icm30628_set_LP_enable(true);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		mutex_unlock(&st->fifolock);
		return ret;
	}

	mutex_unlock(&st->fifolock);

	return ret;
}

int invensense_fifo_read(void)
{
	int ret = 0;
	u16 length = 0;
	int target_bytes, tmp;
	u8 *dptr;
	struct icm30628_state_t * st = icm30628_state;
	int i;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(!st->is_firmware_download_done){
		return 0;
	}

	mutex_lock(&st->fifolock);

	ret = icm30628_set_LP_enable(false);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		ret |= icm30628_set_LP_enable(true);
		mutex_unlock(&st->fifolock);
		return ret;
	}

	ret = invensense_get_fifo_status(st, GARNET_FIFO_ID_DATAOUT, &length);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		ret |= icm30628_set_LP_enable(true);
		mutex_unlock(&st->fifolock);
		return ret;
	}

#ifdef POLLING_MODE
	//polling mode cause too much log
	//INV_INFO("invensense_fifo_read... size = %d\n", length);
#else
	INV_DATA("invensense_fifo_read... size = %d\n", length);
#endif

	if(length > INVENSENSE_OUTPUT_FIFO_LENGTH){
		ret = -1;
		INV_ERR;
		ret |= icm30628_reset_fifo(GARNET_FIFO_ID_DATAOUT);
		ret |= icm30628_set_LP_enable(true);
		mutex_unlock(&st->fifolock);		
		return ret;
	}

	if(length < MIN_PACKET_SIZE){
		printk("packet length is not enough, size is %d\n", length);
		INV_DATA("length(%d) < MIN_PACKET_SIZE\n", length);
		ret |= icm30628_set_LP_enable(true);
		mutex_unlock(&st->fifolock);		
		return 0;
	}

	st->icm30628_status.fifo_size = length + st->icm30628_status.fifo_unhandled_data_size;
	
	if(st->icm30628_status.fifo_unhandled_data_size > 0){
		memcpy(st->icm30628_status.fifo_cache, st->icm30628_status.fifo_unhandled_data, st->icm30628_status.fifo_unhandled_data_size);
	}
	dptr = st->icm30628_status.fifo_cache + st->icm30628_status.fifo_unhandled_data_size;
	st->icm30628_status.fifo_unhandled_data_size = 0;
	target_bytes = length;
	while (target_bytes > 0) {
		if (target_bytes < MAX_READ_SIZE){
			tmp = target_bytes;
		}else{
			tmp = MAX_READ_SIZE;
		}
		ret = invensense_bank_read(st, GARNET_REG_BANK_0, GARNET_FIFO_R_W_B0, tmp, dptr);
		if(UNLIKELY(ret <0)){
			INV_ERR;
			ret |= icm30628_set_LP_enable(true);
			mutex_unlock(&st->fifolock);
			return ret;
		}		
		dptr += tmp;
		target_bytes -= tmp;
	}

	INV_DATA("output fifo = { ");
	for(i = 0 ; i < st->icm30628_status.fifo_size ; i ++){
		INV_DATA2("%3d ",  st->icm30628_status.fifo_cache[i]);
	}
	INV_DATA2("}\n");

	ret = invensense_get_data_from_fifo_buffer(st);
	if(UNLIKELY(ret <0)){
		INV_ERR;
		ret |= icm30628_set_LP_enable(true);
		mutex_unlock(&st->fifolock);
		return ret;
	}

	ret = icm30628_set_LP_enable(true);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		mutex_unlock(&st->fifolock);
		return ret;
	}

	mutex_unlock(&st->fifolock);

	return ret;
}

#ifdef USING_WORKQUEUE_FOR_SENDING_COMMAND
int invensense_get_command_from_kfifo(struct icm30628_state_t * st, u8 * data)
{
	int ret = 0;
	int length = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!st->kfifo_command_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = kfifo_out(st->kfifo_command_buffer, data, 2); 
	if(!ret){
		ret = -1;
		INV_ERR;
		return ret;
	} else {
		ret = 0;
	}

	length = invensense_get_fifo_command_size(data[1]);	
	if(UNLIKELY(length < 0)){
		ret = -1;
		INV_ERR;
		return ret;
	}

	if(length < 3){
		return 0;
	}

	ret = kfifo_out(st->kfifo_command_buffer, &data[2], length -2); 
	if(!ret){
		ret = -1;
		INV_ERR;
		return ret;
	} else {
		ret = 0;
	}

	return ret;
}

int invensense_put_command_to_kfifo(struct icm30628_state_t * st, u8 * data)
{
	int ret = 0;
	int length = 0;
	int count = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!st->kfifo_command_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	length = invensense_get_fifo_command_size(data[1]);
	if(UNLIKELY(length < 0)){
		ret = -1;
		INV_ERR;
		return ret;
	}

	while(kfifo_len(st->kfifo_command_buffer) > INVENSENSE_COMMAND_KFIFO_LENGTH - length
		&& count < 10){
		msleep(5);
		count++;
	}

	ret = kfifo_in(st->kfifo_command_buffer, data, length);	
	if(UNLIKELY(!ret)){
		ret = -1;
		INV_ERR;
		return ret;
	} else {
		ret = 0;
	}

	return ret;
}

int invensense_reset_command_kfifo(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!st->kfifo_command_buffer)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	kfifo_reset(st->kfifo_command_buffer); 

	return 0;
}

int invensense_start_command_workqueue(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	schedule_work(&st->command_work);	

	return ret;
}

static void invensense_command_work(struct work_struct *work)
{
	int ret = 0;
	u8 data[38];
	int length;	
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return;
	}

	if(kfifo_len(st->kfifo_command_buffer) < 1){
		return; //fifo is empty
	}

	ret = invensense_get_command_from_kfifo(st, data);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return;
	}

	length = invensense_get_fifo_command_size(data[1]);
	if (UNLIKELY(length < 0)) {
		ret = -1;
		INV_ERR;
		return;
	}
	
	ret =  invensense_fifo_write(st, length, data);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return;
	}

	ret = invensense_start_command_workqueue(st);//restart
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return;
	}

	return;
}
#endif

int invensense_put_data_to_cache(struct icm30628_state_t * st, int sensorNum, u8 * data)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	if(UNLIKELY(!data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_pull_data_from_cache(struct icm30628_state_t * st, int sensorNum, u8 * data)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	
	if(UNLIKELY(!data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_verify_command(struct icm30628_state_t * st, int sensorNum, int cmd, u8 * value)
{
	u64 delay;

	if(cmd == INV_CMD_SENSOR_ON || cmd == INV_CMD_SENSOR_OFF){
		if(st->icm30628_status.is_sensor_enabled_internally[sensorNum] == cmd){
			return -1;
		}
		st->icm30628_status.is_sensor_enabled_internally[sensorNum] = cmd;
	}
	if(cmd == INV_CMD_SET_DELAY){
		delay = ((value[0] << 8) + value[1]) * 1000000; 	
		if(st->icm30628_status.sensor_internal_sampling_rate[sensorNum] == delay){
			return -1;
		}
		st->icm30628_status.sensor_internal_sampling_rate[sensorNum] = delay;
	}
	if(cmd == INV_CMD_BATCH){
		delay = ((value[0] << 8) + value[1]) * 1000000; 	
		if(st->icm30628_status.sensor_internal_batching_timeout[sensorNum] == delay){
			return -1;
		}
		st->icm30628_status.sensor_internal_batching_timeout[sensorNum] = delay;
	}

	return 0;
}

int invensense_send_command_to_fifo(struct icm30628_state_t * st, int sensorNum, int cmd, u8 * value)
{
	int ret = 0;
	u8 data[38] = {0};
	int data_size = 0;
	u16 sensorID = INV_SENSOR_INVALID;

	INV_DBG_FUNC_NAME_DETAIL;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_verify_command(st, sensorNum, cmd, value);
	if (ret < 0) {
		// same as before
		return 0;
	}

	sensorID = invensense_get_sensor_id(sensorNum);
	if (UNLIKELY(sensorID < 0)) {
		ret = -1;
		INV_ERR;
		return ret;
	}
	
	data[0] = sensorID;
	data[1] = cmd;
	data_size = invensense_get_fifo_command_size(cmd);
	if (UNLIKELY(data_size < 0)) {
		ret = -1;
		INV_ERR;
		return ret;
	}

	if(value){
		memcpy(&data[2], value, data_size - 2);
	}

#ifdef USING_WORKQUEUE_FOR_SENDING_COMMAND
	invensense_put_command_to_kfifo(st, data);
	if(st->is_download_done){
		invensense_start_command_workqueue(st);
	}
#else
	ret =  invensense_fifo_write(st, data_size, data);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
#endif

	return ret;
}

int invensense_build_data(struct icm30628_state_t * st, u8 * data)
{
	int ret = 0;
	u8 id;
	int i;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	if(UNLIKELY(!data)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	id = *data;

	switch(id){
		case INV_ACCELEROMETER:
			st->icm30628_status.accel_current_accuracy = 	data[1] & 0x3;	
			if (st->icm30628_status.accel_current_accuracy == 3) {
				invensense_accel_bias_found(st, true);
			}else{
				invensense_accel_bias_found(st, false);
			}
			for( i  = 0 ; i < 3 ; i++){			
				st->icm30628_status.accel_current_data[i] = (int)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(st->icm30628_status.accel_current_data[i] > 0x7FFF) st->icm30628_status.accel_current_data[i] -= 0xFFFF;
			} 			
			invensense_apply_32_orientation(st, (u8 *)st->icm30628_status.accel_current_data);
			INV_DATA("accel %d, %d, %d\n", st->icm30628_status.accel_current_data[0] ,st->icm30628_status.accel_current_data[1] , st->icm30628_status.accel_current_data[2]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_GYROSCOPE:
			st->icm30628_status.gyro_current_accuracy = 	data[1] & 0x3;
			if (st->icm30628_status.gyro_current_accuracy == 3) {
				invensense_gyro_bias_found(st, true);
			}else{
				invensense_gyro_bias_found(st, false);
			}
			for( i  = 0 ; i < 3 ; i++){
				st->icm30628_status.gyro_current_data[i] = (int)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(st->icm30628_status.gyro_current_data[i] > 0x7FFF) st->icm30628_status.gyro_current_data[i] -= 0xFFFF;
			} 			
			invensense_apply_32_orientation(st, (u8 *)st->icm30628_status.gyro_current_data);
			INV_DATA("gyro %d, %d, %d\n", st->icm30628_status.gyro_current_data[0] ,st->icm30628_status.gyro_current_data[1] , st->icm30628_status.gyro_current_data[2]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_MAGNETIC_FIELD:
			st->icm30628_status.mag_current_accuracy = 	data[1] & 0x3;
			if (st->icm30628_status.mag_current_accuracy == 3) {
				invensense_compass_bias_found(st, true);
			}else{
				invensense_compass_bias_found(st, false);
			}
			for( i  = 0 ; i < 3 ; i++){
				st->icm30628_status.mag_current_data[i] = (int)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(st->icm30628_status.mag_current_data[i] > 0x7FFF) st->icm30628_status.mag_current_data[i] -= 0xFFFF;
			}
			invensense_apply_32_orientation(st, (u8 *)st->icm30628_status.mag_current_data);
			INV_DATA("compass %d, %d, %d\n", st->icm30628_status.mag_current_data[0] ,st->icm30628_status.mag_current_data[1] , st->icm30628_status.mag_current_data[2]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_ORIENTATION:
			st->icm30628_status.orientation_current_accuracy = data[1] & 0x3;
			for( i  = 0 ; i < 3 ; i++){
				st->icm30628_status.orientation_current_data[i] = (int)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(st->icm30628_status.orientation_current_data[i] > 0x7FFF) st->icm30628_status.orientation_current_data[i] -= 0xFFFF;
			} 			
			invensense_apply_32_orientation(st, (u8 *)st->icm30628_status.orientation_current_data);
			INV_DATA("orientation %d, %d, %d\n", st->icm30628_status.orientation_current_data[0] ,st->icm30628_status.orientation_current_data[1] , st->icm30628_status.orientation_current_data[2]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_GRAVITY:
			st->icm30628_status.gravity_current_accuracy = data[1] & 0x3;
			for( i  = 0 ; i < 3 ; i++){
				st->icm30628_status.gravity_current_data[i] = (int)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(st->icm30628_status.gravity_current_data[i] > 0x7FFF) st->icm30628_status.gravity_current_data[i] -= 0xFFFF;
			} 			
			invensense_apply_32_orientation(st, (u8 *)st->icm30628_status.gravity_current_data);
			INV_DATA("gravity %d, %d, %d\n", st->icm30628_status.gravity_current_data[0] ,st->icm30628_status.gravity_current_data[1] , st->icm30628_status.gravity_current_data[2]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_LINEAR_ACCELERATION:
			st->icm30628_status.la_current_accuracy = data[1] & 0x3;
			for( i  = 0 ; i < 3 ; i++){
				st->icm30628_status.la_current_data[i] = (int)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u16)));
				if(st->icm30628_status.la_current_data[i] > 0x7FFF) st->icm30628_status.la_current_data[i] -= 0xFFFF;
			} 			
			invensense_apply_32_orientation(st, (u8 *)st->icm30628_status.la_current_data);
			INV_DATA("linear accel %d, %d, %d\n", st->icm30628_status.la_current_data[0] ,st->icm30628_status.la_current_data[1] , st->icm30628_status.la_current_data[2]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_GAME_ROTATION_VECTOR:
			st->icm30628_status.grv_current_accuracy = data[1] & 0x3;
			for( i  = 0 ; i < 4 ; i++){
				st->icm30628_status.grv_current_data[i] = (int)le32_to_cpup((__le32 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u32)));
			} 				
			invensense_apply_quat_orientation(st, st->icm30628_status.grv_current_data);
			INV_DATA("game rotation vector %d, %d, %d, %d\n", st->icm30628_status.grv_current_data[0] ,st->icm30628_status.grv_current_data[1] , st->icm30628_status.grv_current_data[2] , st->icm30628_status.grv_current_data[3]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
			if(st->icm30628_status.is_sensor_enabled[INV_GRAVITY_NUM]){
				invensense_convert_quat_to_gravity(st->icm30628_status.gravity_current_data, 
					st->icm30628_status.grv_current_data);
				INV_DATA("gravity %d, %d, %d, batchmode = %d\n", st->icm30628_status.gravity_current_data[0] ,st->icm30628_status.gravity_current_data[1] , st->icm30628_status.gravity_current_data[2], st->icm30628_status.is_sensor_in_batchmode[INV_GRAVITY_NUM]);
				if(st->icm30628_status.is_sensor_in_batchmode[INV_GRAVITY_NUM] == true){
					u8 temp[SIZE_HDR + SIZE_STAMP + CACHE_SIZE_3AXIS] = {0};
					memcpy(temp, data, SIZE_HDR + SIZE_STAMP);
					memcpy(&temp[SIZE_HDR + SIZE_STAMP], st->icm30628_status.gravity_current_data, CACHE_SIZE_3AXIS);
					temp[0] = INV_GRAVITY;
					temp[1] = data[1];
					ret = invensense_kfifo_in(st, temp);
					if(UNLIKELY(ret < 0)){
						INV_ERR;
						return ret;
					}
				}
			}
			if(st->icm30628_status.is_sensor_enabled[INV_LINEAR_ACCELERATION_NUM]){
				invensense_convert_quat_and_accel_to_linearAccel(st->icm30628_status.la_current_data
					, st->icm30628_status.grv_current_data
					, st->icm30628_status.accel_current_data);
				INV_DATA("linear accel %d, %d, %d, batchmode = %d\n", st->icm30628_status.la_current_data[0] ,st->icm30628_status.la_current_data[1] , st->icm30628_status.la_current_data[2], st->icm30628_status.is_sensor_in_batchmode[INV_LINEAR_ACCELERATION_NUM]);
				if(st->icm30628_status.is_sensor_in_batchmode[INV_LINEAR_ACCELERATION_NUM] == true){
					u8 temp[SIZE_HDR + SIZE_STAMP + CACHE_SIZE_3AXIS] = {0};
					memcpy(temp, data, SIZE_HDR + SIZE_STAMP);
					memcpy(&temp[SIZE_HDR + SIZE_STAMP], st->icm30628_status.la_current_data, CACHE_SIZE_3AXIS);
					temp[0] = INV_LINEAR_ACCELERATION;
					temp[1] = data[1];
					ret = invensense_kfifo_in(st, temp);
					if(UNLIKELY(ret < 0)){
						INV_ERR;
						return ret;
					}
				}
			}
#endif			
			break;
		case INV_GEOMAGNETIC_ROTATION_VECTOR:
			st->icm30628_status.gmrv_current_accuracy = data[1] & 0x3;
			for( i  = 0 ; i < 4 ; i++){
				st->icm30628_status.gmrv_current_data[i] = (int)le32_to_cpup((__le32 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u32)));
			} 				
			invensense_apply_quat_orientation(st, st->icm30628_status.gmrv_current_data);
			INV_DATA("geomagnetic rotation vector %d, %d, %d, %d\n", st->icm30628_status.gmrv_current_data[0] ,st->icm30628_status.gmrv_current_data[1] , st->icm30628_status.gmrv_current_data[2], st->icm30628_status.gmrv_current_data[3]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_ROTATION_VECTOR:
			st->icm30628_status.rv_current_accuracy = (short)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP + SIZE_QUAT));
			for( i  = 0 ; i < 4 ; i++){
				st->icm30628_status.rv_current_data[i] = (int)le32_to_cpup((__le32 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u32)));
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
				if(st->icm30628_status.is_sensor_enabled[INV_ORIENTATION_NUM]){
					st->icm30628_status.orientation_raw_data[i] = (int)le32_to_cpup((__le32 *)(data + SIZE_HDR + SIZE_STAMP + i * sizeof(u32)));
				}
#endif
			} 				
			invensense_apply_quat_orientation(st, st->icm30628_status.rv_current_data);
			INV_DATA("rotation vector %d, %d, %d, %d\n", st->icm30628_status.rv_current_data[0] ,st->icm30628_status.rv_current_data[1] , st->icm30628_status.rv_current_data[2] , st->icm30628_status.rv_current_data[3]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
			invensense_apply_quat_orientation(st, st->icm30628_status.orientation_raw_data);
			if(st->icm30628_status.is_sensor_enabled[INV_ORIENTATION_NUM]){
				invensense_request_to_build_orientation(st);
			}
#endif
			break;
		case INV_HEART_RATE:
#ifdef PIXART_LIBRARY_IN_FIRMWARE
			st->icm30628_status.hrm_current_accuracy = data[1] & 0x3;
			st->icm30628_status.hrm_current_data[0] = (int)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP));
			INV_DATA("heart rate %d\n", st->icm30628_status.hrm_current_data[0]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
#else			
			st->icm30628_status.hrm_current_accuracy = data[1] & 0x3;
			memcpy(st->icm30628_status.hrm_current_raw_data, &data[SIZE_HDR + SIZE_STAMP + SIZE_PAYLOAD], 13);
			INV_DATA("heart rate raw ");
			for (i = 0; i < 13 ; i++){
				INV_DATA2("%d ", st->icm30628_status.hrm_current_raw_data[i]);
			}
			INV_DATA2("\n");			
			invensense_request_to_build_heartrate(st);
#endif			
			break;
		case INV_PRESSURE:
			st->icm30628_status.baro_current_accuracy = data[1] & 0x3;
			st->icm30628_status.baro_current_data[0] = (int)le32_to_cpup((__le32 *)(data + SIZE_HDR + SIZE_STAMP));
			INV_DATA("pressure %d\n", st->icm30628_status.baro_current_data[0]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_PROXIMITY:
			st->icm30628_status.proximity_current_accuracy = data[1] & 0x3;
			st->icm30628_status.proximity_current_data[0] = (int)le16_to_cpup((__le16 *)(data + SIZE_HDR + SIZE_STAMP));
			INV_DATA("proximity %d\n", st->icm30628_status.proximity_current_data[0]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_STEP_COUNTER:
			st->icm30628_status.sc_current_accuracy = data[1] & 0x3;
			st->icm30628_status.sc_current_data[0] = (int)le64_to_cpup((__le64 *)(data + SIZE_HDR + SIZE_STAMP));
			INV_DATA("step counter %d\n", st->icm30628_status.sc_current_data[0]);
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_STEP_DETECTOR:
			st->icm30628_status.sd_current_accuracy = data[1] & 0x3;
			INV_DATA("step detector\n");
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			} else {
				step_notify(TYPE_STEP_DETECTOR);
			}
			break;
		case INV_SIGNIFICANT_MOTION:
			st->icm30628_status.smd_current_accuracy = data[1] & 0x3;
			INV_DATA("significant motion\n");
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			} else {
				step_notify(TYPE_SIGNIFICANT);
			}
			break;
		case INV_ACTIVITY_CLASSIFICATION:
			st->icm30628_status.activity_current_accuracy = data[1] & 0x3;
			st->icm30628_status.activity_current_data[0] = data[SIZE_HDR + SIZE_STAMP];
			INV_DATA("activity %x\n",st->icm30628_status.activity_current_data[0] );
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_UNKNOWN){
				st->icm30628_status.activity_current_status = UNKNOWN;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_IN_VEHICLE_START){
				st->icm30628_status.activity_current_status |= IN_VEHICLE;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_IN_VEHICLE_END){
				st->icm30628_status.activity_current_status &= ~IN_VEHICLE;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_WALKING_START){
				st->icm30628_status.activity_current_status |= WALKING;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_WALKING_END){
				st->icm30628_status.activity_current_status &= ~WALKING;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_RUNNING_START){
				st->icm30628_status.activity_current_status |= RUNNING;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_RUNNING_END){
				st->icm30628_status.activity_current_status &= ~RUNNING;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_ON_BICYCLE_START){
				st->icm30628_status.activity_current_status |= ON_BICYCLE;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_ON_BICYCLE_END){
				st->icm30628_status.activity_current_status &= ~ON_BICYCLE;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_TILT_START){
				st->icm30628_status.activity_current_status |= TILT;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_TILT_END){
				st->icm30628_status.activity_current_status &= ~TILT;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_STILL_START){
				st->icm30628_status.activity_current_status |= STILL;
			}
			if(st->icm30628_status.activity_current_data[0] == INV_DATA_BAC_ACTIVITY_STILL_END){
				st->icm30628_status.activity_current_status &= ~STILL;
			}
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
		case INV_SHAKE:
			st->icm30628_status.shk_current_accuracy = data[1] & 0x3;
			INV_DATA("shake\n");
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			} else {
				shk_notify();
			}
			break;
		case INV_BRING_TO_SEE:
			st->icm30628_status.bts_current_accuracy = data[1] & 0x3;
			INV_DATA("bring to see\n");
			if(st->icm30628_status.is_sensor_in_batchmode[invensense_get_sensor_num(id)] == true
				&& st->icm30628_status.is_sensor_enabled[invensense_get_sensor_num(id)] == true){
				ret = invensense_kfifo_in(st, data);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			} else {
				bts_notify();
			}
			break;
		default:
			ret = -1;	
			INV_ERR;
			return -EINVAL;
	}

	return ret;
}


int invensense_get_data_from_fifo_buffer(struct icm30628_state_t * st)
{
	int ret = 0;
	u8 id;
	u8 status;
	u8 status_field;
	u8 is_answer;
	u8 has_payload;
	u8 payload_size;
	u8 *dptr, *d;
	u32 t;
	int target_bytes;
	int packet_size;
	int cmd;
	u8 firmware_info[SIZE_FIRMWARE_INFO];
	u8 clock_rate[SIZE_CLOCK_RATE];
#ifdef DMP_DOWNLOAD
	u8 who; 
	u8 what;
	u32 response;
#endif
	int i;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}	

	target_bytes = st->icm30628_status.fifo_size;
	dptr = st->icm30628_status.fifo_cache;
	d = dptr;
	
	while (dptr - d <= target_bytes - MIN_PACKET_SIZE) {
		id = *dptr;
		status = *(dptr + 1);		
		status_field = (status & 0x0C) >> 2;
		is_answer = (status & 0x20) >> 5;
		has_payload = (status & 0x40) >> 6;
		if((status_field  == INV_STATUS_CHANGED) || (status_field  == INV_STATUS_FLUSH)){
			INV_INFO("sensor id %d is enabled\n", id);
			dptr += DUMMY_PACKET_SIZE;
			if(has_payload){
				payload_size = *dptr;
				dptr += 1;
			}
			continue;
		}	
		if(is_answer){
			INV_INFO("received answer from sensor\n");
			cmd = *(dptr + 2);
			dptr += DUMMY_PACKET_SIZE;
			switch(cmd)
			{
				case INV_CMD_PING:
					INV_INFO("ping ok, sensor %d, data = %d\n", id, *dptr);					
					dptr += 1;	
					break;
#ifdef DMP_DOWNLOAD
				case INV_CMD_LOAD:
					who = dptr[0];
					what = dptr[1];					
					response = (int)le32_to_cpup((__le32 *)(&dptr[2]));
					INV_INFO("load dmp image, sensor %d, %d, %d\n",who, what, response);
					if(what == FIFOPROTOCOL_LOAD_WHAT_CHECK){
						INV_INFO("DMP check ok\n");
						if(who == FIFOPROTOCOL_LOAD_WHO_DMP3_FW){
							if(0xFFFFFFFF == response){								
								INV_INFO("DMP3 crc error\n");
								st->dmp3_checked = false; 
							} else {
								st->dmp3_checked = true; 
							}
						} else if(who == FIFOPROTOCOL_LOAD_WHO_DMP4_FW){
							if(0xFFFFFFFF == response){								
								INV_INFO("DMP4 crc error\n");
								st->dmp4_checked = false; 
							} else {
								st->dmp4_checked = true; 
							}
						} else {
							INV_INFO("unkown DMP type\n");
							return -1;
						}
						dptr += SIZE_DMP_INFO;
						break;
					} else if(what == FIFOPROTOCOL_LOAD_WHAT_MALLOC){
						if(response == 0x00){
							INV_INFO("M0 unable to allocate requested SRAM\n");
							return -1;
						}
						if(who == FIFOPROTOCOL_LOAD_WHO_DMP3_FW){
							st->dmp3_initiated = true; 
							st->dmp3_address = response;
						} else if(who == FIFOPROTOCOL_LOAD_WHO_DMP4_FW){
							st->dmp4_initiated = true; 
							st->dmp4_address = response;
						} else {
							INV_INFO("unkown DMP type\n");
							return -1;
						}
						dptr += SIZE_DMP_INFO;	
					}
					break;
#endif
				case INV_CMD_GET_CALIBRATION_GAINS:
					INV_INFO("calibration gain, sensor %d\n", id);
					INV_INFO("output fifo = { ");
					for(i = 0 ; i < SIZE_CALIBRATION_GAIN ; i ++){
						INV_INFO2("%3d ",  dptr[i]);
					}
					INV_INFO2("}\n");
					if(id == INV_ACCELEROMETER){
						memcpy(st->calibration.accel_calibration_gain, dptr, SIZE_CALIBRATION_GAIN);
					}
					if(id == INV_GYROSCOPE){
						memcpy(st->calibration.gyro_calibration_gain, dptr, SIZE_CALIBRATION_GAIN);
					}
					if(id == INV_MAGNETIC_FIELD){
						memcpy(st->calibration.mag_calibration_gain, dptr, SIZE_CALIBRATION_GAIN);
					}					
					ret = invensense_request_store_calibration(st);
					if (UNLIKELY(ret < 0)) {
						INV_ERR;
						return ret;
					}					
					dptr += SIZE_CALIBRATION_GAIN;	
					break;
				case INV_CMD_GET_CALIBRATION_OFFSETS:
					INV_INFO("calibration offset, sensor %d\n", id);
					INV_INFO("output fifo = { ");
					for(i = 0 ; i < SIZE_CALIBRATION_OFFSET ; i ++){
						INV_INFO2("%3d ",  dptr[i]);
					}
					INV_INFO2("}\n");
					if(id == INV_ACCELEROMETER){
						memcpy(st->calibration.accel_calibration_offset, dptr, SIZE_CALIBRATION_OFFSET);
					}
					if(id == INV_GYROSCOPE){
						memcpy(st->calibration.gyro_calibration_offset, dptr, SIZE_CALIBRATION_OFFSET);
					}
					if(id == INV_MAGNETIC_FIELD){
						memcpy(st->calibration.mag_calibration_offset, dptr, SIZE_CALIBRATION_OFFSET);
					}
					ret = invensense_request_store_calibration(st);
					if (UNLIKELY(ret < 0)) {
						INV_ERR;
						return ret;
					}					
					dptr += SIZE_CALIBRATION_OFFSET;					
					break;
				case INV_CMD_GET_CLOCK_RATE:
					INV_INFO("clock rate, sensor %d\n", id);
					INV_INFO("output fifo = { ");
					for(i = 0 ; i < SIZE_CLOCK_RATE ; i ++){
						INV_INFO2("%3d ",  dptr[i]);
					}
					INV_INFO2("}\n");
					memcpy(clock_rate, dptr, SIZE_CLOCK_RATE);
					dptr += SIZE_CLOCK_RATE;					
					break;
				case INV_CMD_GET_FIRMWARE_INFO:
					INV_INFO("firmware information\n");
					INV_INFO("output fifo = { ");
					for(i = 0 ; i < SIZE_FIRMWARE_INFO ; i ++){
						INV_INFO2("%3d ",  dptr[i]);
					}
					INV_INFO2("}\n");
					memcpy(firmware_info, dptr, SIZE_FIRMWARE_INFO);
					dptr += SIZE_FIRMWARE_INFO;					
					break;
				default:
					INV_INFO("unknown answer recieved... \n");					
					break;
			}			
			continue;
		}
		t = (__le32)le32_to_cpup((__le32 *)(dptr + 2));
		if(t == 0){
			INV_INFO("timestamp is 0, it means a dummy packet so skip it\n");
			INV_INFO("output fifo = { ");
			for(i = 0 ; i < DUMMY_PACKET_SIZE ; i ++){
				INV_INFO2("%3d ",  dptr[i]);
			}
			INV_INFO2("}\n");
			dptr += DUMMY_PACKET_SIZE;			
			continue;		
		}
		packet_size = invensense_get_fifo_data_size(invensense_get_sensor_num(id));
		if(packet_size == -1){
			INV_INFO("unknown sensor id = %d\n", id);
			ret = -1;
			INV_ERR;
			return ret;
		}
		if(dptr - d > target_bytes - packet_size){
			st->icm30628_status.fifo_unhandled_data_size = target_bytes - (dptr - d);
			memcpy(st->icm30628_status.fifo_unhandled_data, dptr, st->icm30628_status.fifo_unhandled_data_size);
			return ret;
		}		
		ret = invensense_build_data(st, dptr);
		if(ret < 0) {		
			INV_ERR;
			return ret;
		}		
		dptr += packet_size;
	}

	if( dptr - d < target_bytes ){
		st->icm30628_status.fifo_unhandled_data_size = target_bytes - (dptr - d);
		memcpy(st->icm30628_status.fifo_unhandled_data, dptr, st->icm30628_status.fifo_unhandled_data_size);
	}

	return ret;
}

//icm306xx
//GPIO0 => wakeup interrupt
//GPIO1 => normal interrupt
#ifdef MTK_PLATFORM
static void invensense_irq_normal_thread(struct work_struct *work)
#else
irqreturn_t invensense_irq_normal_thread(int irq, void *dev_id)
#endif
{
#ifndef	POLLING_MODE
	int ret = 0;
	u8 interrupt_status_0;
	u8 interrupt_status_1;
#endif

	INV_DBG_FUNC_NAME;

#ifndef	POLLING_MODE
#if 0
	icm30628_wakelock(true);
	ret = invensense_get_interrupt_status(GARNET_SCRATCH_INT1_STATUS_B0, &interrupt_status);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		icm30628_wakelock(false);
		return IRQ_HANDLED;
	}					
	if(!interrupt_status){
		icm30628_wakelock(false);
		return IRQ_HANDLED;
	}
#else//test
	icm30628_wakelock(true);
	ret = invensense_get_interrupt_status(GARNET_SCRATCH_INT0_STATUS_B0, &interrupt_status_0);
	ret = invensense_get_interrupt_status(GARNET_SCRATCH_INT1_STATUS_B0, &interrupt_status_1);
  printk("%s,(%x,%x)\n", __func__, interrupt_status_0, interrupt_status_1);
	if((!interrupt_status_0) && (!interrupt_status_1)){
		icm30628_wakelock(false);
		return IRQ_HANDLED;
	}
#endif
	invensense_fifo_read();
	icm30628_wakelock(false);
#endif

	return IRQ_HANDLED;
}

irqreturn_t invensense_irq_normal_handler(int irq, void *dev_id)
{
#ifndef	POLLING_MODE
	struct icm30628_state_t * st = icm30628_state;
#endif

	INV_DBG_FUNC_NAME;

#ifndef	POLLING_MODE
	st->watchdog_time = invensense_get_time_ns();
#endif

#ifdef MTK_PLATFORM
	schedule_work(&eint_normal_work);
#endif

	return IRQ_WAKE_THREAD;
}

//icm306xx
//GPIO0 => wakeup interrupt
//GPIO1 => normal interrupt
#ifdef MTK_PLATFORM
static void invensense_irq_wakeup_thread(struct work_struct *work)
#else
irqreturn_t invensense_irq_wakeup_thread(int irq, void *dev_id)
#endif
{
#ifndef	POLLING_MODE
	int ret = 0;
	u8 interrupt_status_0;
	u8 interrupt_status_1;
#endif

	INV_DBG_FUNC_NAME;

#ifndef	POLLING_MODE
#if 0
	icm30628_wakelock(true);
	ret = invensense_get_interrupt_status(GARNET_SCRATCH_INT0_STATUS_B0, &interrupt_status_0);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		icm30628_wakelock(false);
		return IRQ_HANDLED;
	}					
	if(!interrupt_status){
		icm30628_wakelock(false);
		return IRQ_HANDLED;
	}
#else//test
	icm30628_wakelock(true);
	ret = invensense_get_interrupt_status(GARNET_SCRATCH_INT0_STATUS_B0, &interrupt_status_0);
	ret = invensense_get_interrupt_status(GARNET_SCRATCH_INT1_STATUS_B0, &interrupt_status_1);
  printk("%s,(%x,%x)\n", __func__, interrupt_status_0, interrupt_status_1);
	if((!interrupt_status_0) && (!interrupt_status_1)){
		icm30628_wakelock(false);
		return IRQ_HANDLED;
	}
#endif
	invensense_fifo_read();
		icm30628_wakelock(false);
#endif

	return IRQ_HANDLED;
}

irqreturn_t invensense_irq_wakeup_handler(int irq, void *dev_id)
{
#ifndef	POLLING_MODE
	struct icm30628_state_t * st = icm30628_state;
#endif

	INV_DBG_FUNC_NAME;

#ifndef	POLLING_MODE
	st->watchdog_time = invensense_get_time_ns();
#endif

#ifdef MTK_PLATFORM
#ifdef MTK_EINT_WAKE_ENABLE
	schedule_work(&eint_wakeup_work);
#endif
#endif
	return IRQ_WAKE_THREAD;
}

int invensense_interrupt_configuration(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef SPI_INTERFACE
	if(UNLIKELY(st->icm30628_irq  != 0)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
#ifdef MTK_PLATFORM
	st->icm30628_irq = CUST_EINT_SENSORHUB_NUM;
	#ifdef MTK_EINT_WAKE_ENABLE
    st->icm30628_wakeup_irq = CUST_EINT_SENSORHUB_WAKE_UP_NUM;
  #endif
#else
	st->icm30628_irq = st->icm30628_spi_device->irq;
#endif
#endif

#ifdef I2C_INTERFACE
	if(UNLIKELY(st->icm30628_irq  != 0)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
#endif

#ifndef POLLING_MODE
	ret = invensense_interrupt_normal_initialize(&st->icm30628_irq,
		(void*) st, 
		invensense_irq_normal_handler, 
		invensense_irq_normal_thread);
	if(UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	ret = invensense_interrupt_wakeup_initialize(&st->icm30628_wakeup_irq, 
		(void*) st,
		invensense_irq_wakeup_handler, 
		invensense_irq_wakeup_thread);
	if(UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}
#endif
	return ret;
}

int invensense_power_configuration(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef SPI_INTERFACE
	ret = invensense_power_initialize(&st->icm30628_spi_device->dev);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}
#endif
	
#ifdef I2C_INTERFACE
	ret = invensense_power_initialize(&st->icm30628_i2c_client->dev);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}
#endif

	return ret;
}

struct icm30628_state_t * invensense_allocation(void)
{
	struct icm30628_state_t * st = NULL;

	INV_DBG_FUNC_NAME;

	st = kzalloc(sizeof(struct icm30628_state_t), GFP_KERNEL);

	return st;
}

int invensense_set_configuration(struct icm30628_state_t * st)
{
	int ret = 0;
	int i = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef I2C_INTERFACE
	st->icm30628_i2c_client = NULL;
#endif
#ifdef SPI_INTERFACE
	st->icm30628_spi_device = NULL;
#endif
	st->vdd_ana =  NULL;
	st->vdd_1_8 = NULL;
	st->icm30628_irq = 0;
	st->icm30628_wakeup_irq = 0;
	st->kfifo_buffer = NULL;
#ifdef USING_WORKQUEUE_FOR_SENDING_COMMAND
	st->kfifo_command_buffer = NULL;
#endif
	st->daemon_t = NULL;
#ifdef ENABLE_LP_EN	
	st->is_lp_enabled = false;
#endif
	for(i = 0 ; i < INV_SENSOR_MAX_NUM ; i++){
		st->icm30628_status.is_sensor_enabled[i] = false;
		st->icm30628_status.is_sensor_in_batchmode[i] = false;
		st->icm30628_status.sensor_sampling_rate[i] = LONG_MAX;
		st->icm30628_status.sensor_batching_timeout[i] = 0; //LONG_MAX;
		st->icm30628_status.is_sensor_enabled_internally[i] = false;
		st->icm30628_status.sensor_internal_sampling_rate[i] = LONG_MAX;
		st->icm30628_status.sensor_internal_batching_timeout[i] = 0; //LONG_MAX;
	}

	st->icm30628_status.fifo_size = 0;
	st->is_download_done = false;
	st->is_accel_calibrated = false;
	st->is_gyro_calibrated = false;
	st->is_mag_calibrated = false;

	invensense_orientation_matrix(st);

#ifdef USING_WORKQUEUE_FOR_SENDING_COMMAND
	INIT_WORK(&st->command_work, invensense_command_work);	
#endif
	INIT_WORK(&st->clear_int0_work, invensense_clear_INT0_status_work);	
	INIT_WORK(&st->clear_int1_work, invensense_clear_INT1_status_work);	
	INIT_DELAYED_WORK(&st->watchdog_work, invensense_watchdog_work_func);
	st->is_watchdog_running = false;
#ifdef DMP_DOWNLOAD
	st->dmp3_initiated = false;	
	st->dmp4_initiated = false;	
	st->dmp3_address = 0x00;
	st->dmp4_address = 0x00;
	st->dmp3_checked = false;
	st->dmp4_checked = false;
#endif
	st->is_firmware_download_done = false;

	return ret;
}

int invensense_mutex_initialize(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	mutex_init(&st->fifolock);
	wake_lock_init(&st->sleep_lock, WAKE_LOCK_SUSPEND, "icm30628_wakelock");

	return ret;
}

int invensense_comm_initialize(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef I2C_INTERFACE
	st->icm30628_i2c_client = invensense_i2c_initialize();
#ifdef MTK_PLATFORM
#else
	if (UNLIKELY(st->icm30628_i2c_client == NULL)) {
		INV_ERR;
		return ret;
	}
#endif
#endif
#ifdef SPI_INTERFACE
	st->icm30628_spi_device = invensense_spi_initialize();
#ifdef MTK_PLATFORM
#else
	if (UNLIKELY(st->icm30628_spi_device == NULL)) {
		INV_ERR;
		return ret;
	}
#endif
#endif

	return ret;
}

int invensense_comm_terminate(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

#ifdef I2C_INTERFACE
	invensense_i2c_terminate();
	st->icm30628_i2c_client = NULL;

#endif
#ifdef SPI_INTERFACE
	invensense_spi_terminate();
	st->icm30628_spi_device = NULL;
#endif

	return ret;
}

int invensense_initialize(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = invensense_set_configuration(st);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

int invensense_post_initialize(struct icm30628_state_t * st)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if(UNLIKELY(!st)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}

	ret = invensense_kfifo_allocation(st);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}	

	ret = invensense_mutex_initialize(st);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}	

#ifdef MTK_PLATFORM
#else
	ret = invensense_power_configuration(st);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}

	ret = invensense_power_onoff(true);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}
#endif 

	ret = invensense_interrupt_configuration(st);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}

	ret = icm30628_driver_add();
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}

	return ret;
}

#ifdef CONFIG_PM
static int inv_mpu_resume(struct device *dev)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_power(INV_DUMMY_SENSOR_ID, INV_POWER_ON);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}	
	
	return ret;
}

static int inv_mpu_resume_noirq(struct device *dev)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	return ret;
}

static int inv_mpu_suspend(struct device *dev)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	ret = icm30628_set_power(INV_DUMMY_SENSOR_ID, INV_POWER_SUSPEND);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}	

	return ret;
}

static int inv_mpu_suspend_noirq(struct device *dev)
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	return ret;
}

static const struct dev_pm_ops inv_mpu_pmops = {
	.suspend       = inv_mpu_suspend,
	.suspend_noirq = inv_mpu_suspend_noirq,
	.resume        = inv_mpu_resume,
	.resume_noirq = inv_mpu_resume_noirq,
};

#define INV_MPU_PMOPS (&inv_mpu_pmops)
#else
#define INV_MPU_PMOPS NULL
#endif /* CONFIG_PM */

static int open(struct inode *inode, struct file *file) 
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

	if (!try_module_get(THIS_MODULE)) return -ENODEV;

	return ret; 
}

static int release(struct inode *inode, struct file *file) 
{
	int ret  = 0;

	INV_DBG_FUNC_NAME;

	file->private_data = (void*)NULL;
	module_put(THIS_MODULE);

	return ret; 
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	const size_t nBufSize = 0;

	INV_DBG_FUNC_NAME;

	return nBufSize;
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	INV_DBG_FUNC_NAME;

	*ppos = 0;
	
	return count;
}

static long unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct icm30628_state_t * st = icm30628_state;
	void __user *data = (void __user*)arg;
	int pid = 0;
	unsigned int firmware_size;
	unsigned char * firmware = NULL;
#ifdef DMP_DOWNLOAD
	unsigned int dmp_size;
	unsigned char * dmp = NULL;
	u32 dmp_address;
#endif	
	char string[128];
#if 0
	int i;
#endif

	INV_DBG_FUNC_NAME;
	
	switch (cmd)
	{
		case ICM30628_WRITE_DAEMON_PID:
			ret = copy_from_user(&pid, data, sizeof(int));
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}
			rcu_read_lock();
			st->daemon_t = pid_task(find_pid_ns(pid, &init_pid_ns), PIDTYPE_PID);		
			if(st->daemon_t == NULL){
				INV_ERR;
				rcu_read_unlock();
				return -ENODEV;
			}
			rcu_read_unlock();			
			break;
		case ICM30628_DOWNLOAD_FIRMWARE:
			if(!firmware) {
				firmware = kzalloc(firmware_size, GFP_KERNEL);
			} else {
				return -1;
			}
			ret = copy_from_user(firmware, data, firmware_size);
			if(ret != 0){
				if(firmware){
					kfree(firmware);
					firmware = NULL;
				}
				INV_ERR;
				return -EINVAL;
			}
			ret = icm30628_load_flash_image(firmware, firmware_size);
			if(ret < 0){
				if(firmware){
					kfree(firmware);
					firmware = NULL;
				}
				INV_ERR;
				return -EINVAL;
			}			
			if(firmware){
				kfree(firmware);
				firmware = NULL;
			}
			break;
		case ICM30628_FIRMWARE_SIZE:
			ret = copy_from_user(&firmware_size, data, sizeof(firmware_size));
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
			break;
#ifdef DMP_DOWNLOAD
		case ICM30628_DOWNLOAD_DMP3:
			if(!dmp){
				dmp = kzalloc(dmp_size, GFP_KERNEL);		
			} else {
				return -1;
			}
			ret = copy_from_user(dmp, data, dmp_size);
			if(ret != 0){
				if(dmp){
					kfree(dmp);
					dmp = NULL;
				}
				INV_ERR;
				return -EINVAL;
			}
			
			ret = icm30628_initiate_dmp_load(INV_DMP_TYPE_DMP3, dmp_size, &dmp_address);
			if(ret < 0){
				if(dmp){
					kfree(dmp);
					dmp = NULL;
				}
				INV_ERR;
				return -EINVAL;
			}		
			ret = icm30628_load_dmp_code(INV_DMP_TYPE_DMP3, dmp_address, dmp, dmp_size);
			if(ret < 0){
				if(dmp){
					kfree(dmp);
					dmp = NULL;
				}
				INV_ERR;
				return -EINVAL;
			}
			if(dmp){
				kfree(dmp);
				dmp = NULL;
			}
			
			break;
		case ICM30628_DMP3_SIZE:
			ret = copy_from_user(&dmp_size, data, sizeof(dmp_size));
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}
			break;
		case ICM30628_DOWNLOAD_DMP4:
			if(!dmp) {
				dmp = kzalloc(dmp_size, GFP_KERNEL);	
			} else {
				return -1;
			}
			ret = copy_from_user(dmp, data, dmp_size);
			if(ret != 0){
				if(dmp){
					kfree(dmp);
					dmp = NULL;
				}
				INV_ERR;
				return -EINVAL;
			}
			
			ret = icm30628_initiate_dmp_load(INV_DMP_TYPE_DMP4, dmp_size, &dmp_address);
			if(ret < 0){
				if(dmp){
					kfree(dmp);
					dmp = NULL;
				}
				INV_ERR;
				return -EINVAL;
			}	
			ret = icm30628_load_dmp_code(INV_DMP_TYPE_DMP4, dmp_address, dmp, dmp_size);
			if(ret < 0){
				if(dmp){
					kfree(dmp);
					dmp = NULL;
				}
				INV_ERR;
				return -EINVAL;
			}
			if(dmp){
				kfree(dmp);
				dmp = NULL;
			}
#if 0 //initialize sensor hub
			for( i=0 ; i<INV_SENSOR_MAX_NUM ; i++)
			{
				u8 interrupt_status = 0;
				u8 interrupt;
				u8 packet[10] = {0,};
				do{
					packet[0] = invensense_get_sensor_id(i);
					if (UNLIKELY(packet[0] < 0)) {
						ret = -1;
						INV_ERR;
						return ret;
					}
					packet[1] = INV_CMD_PING;
					ret = invensense_fifo_write(st, 2, packet);	
					if(UNLIKELY(ret <0)){
						INV_ERR;
						return ret;
					}
					invensense_get_interrupt_status(GARNET_SCRATCH_INT0_STATUS_B0, &interrupt_status);
					interrupt = interrupt_status;
					invensense_get_interrupt_status(GARNET_SCRATCH_INT1_STATUS_B0, &interrupt_status);
					interrupt |= interrupt_status;
					msleep(10);
				} while(interrupt == 0);				
#ifdef POLLING_MODE 
				invensense_fifo_read();
#endif
				msleep(5);
			}
#endif

			ret = icm30628_set_LP_enable(false);
			if(UNLIKELY(ret < 0)){
				INV_ERR;
				return ret;
			}
			
			msleep(5);
			
			ret = icm30628_set_deep_sleep();
			if(UNLIKELY(ret < 0)){
				INV_ERR;
				return ret;
			}
			
			msleep(5);
		
			ret = icm30628_set_sleep();
			if(UNLIKELY(ret < 0)){
				INV_ERR;
				return ret;
			}	

			st->is_download_done = true;
			invensense_start_command_workqueue(st);
			icm30628_get_firmware_info();		
			if(ret >= 0) ret = 0; //test
			break;
		case ICM30628_DMP4_SIZE:
			ret = copy_from_user(&dmp_size, data, sizeof(dmp_size));
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
			break;
#endif
		case ICM30628_GET_ORIENTATION:
			ret = copy_to_user(data, st->icm30628_orientation, sizeof(st->icm30628_orientation));
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
			break;
		case ICM30628_SEND_ORIENTATION:
#ifdef M0_ORIENTATION_MATRIX
			icm30628_configure_reference_frame();
#else
			ret = copy_from_user(st->icm30628_quat_chip_to_body, data, sizeof(st->icm30628_quat_chip_to_body));
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
#endif			
			break;			
#ifndef PIXART_LIBRARY_IN_FIRMWARE
		case ICM30628_GET_HRM_DATA:
			ret = copy_to_user(data, st->icm30628_status.hrm_current_raw_data, sizeof(st->icm30628_status.hrm_current_raw_data) + SIZE_STAMP);
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
			break;
		case ICM30628_SEND_HRM_DATA:
			ret = copy_from_user(st->icm30628_status.hrm_current_data, data, sizeof(st->icm30628_status.hrm_current_data) + SIZE_STAMP);
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
			if(st->icm30628_status.is_sensor_in_batchmode[INV_HEART_RATE_NUM] == true){
				u8 temp[SIZE_HDR + SIZE_STAMP + SIZE_HRM] = {0};
				temp[0] = INV_HEART_RATE;
				temp[1] = 0;
				memcpy(&temp[SIZE_HDR], &st->icm30628_status.hrm_current_data[1], SIZE_STAMP);
				memcpy(&temp[SIZE_HDR + SIZE_STAMP], st->icm30628_status.hrm_current_data, sizeof(st->icm30628_status.hrm_current_data));
				ret = invensense_kfifo_in(st, temp);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			INV_DATA("heart rate %d\n", st->icm30628_status.hrm_current_data[0]);
			break;
#endif			
#ifdef HOST_LINEAR_GRAVITY_ORIENTATION
		case ICM30628_GET_ORIENTATION_DATA:			
			ret = copy_to_user(data, st->icm30628_status.orientation_raw_data, sizeof(st->icm30628_status.orientation_raw_data) + SIZE_STAMP);
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
			break;
		case ICM30628_SEND_ORIENTATION_DATA:
			ret = copy_from_user(st->icm30628_status.orientation_current_data, data, sizeof(st->icm30628_status.orientation_current_data) + SIZE_STAMP);
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
			INV_DATA("orientation %d, %d, %d, batchmode = %d\n", st->icm30628_status.la_current_data[0] ,st->icm30628_status.la_current_data[1] , st->icm30628_status.la_current_data[2], st->icm30628_status.is_sensor_in_batchmode[INV_ORIENTATION_NUM]);
			if(st->icm30628_status.is_sensor_in_batchmode[INV_ORIENTATION_NUM] == true){
				u8 temp[SIZE_HDR + SIZE_STAMP + CACHE_SIZE_3AXIS] = {0};
				temp[0] = INV_ORIENTATION;
				temp[1] = st->icm30628_status.rv_current_accuracy;
				memcpy(&temp[SIZE_HDR], &st->icm30628_status.orientation_current_data[3], SIZE_STAMP);
				memcpy(&temp[SIZE_HDR + SIZE_STAMP], st->icm30628_status.orientation_current_data, sizeof(st->icm30628_status.orientation_current_data));
				ret = invensense_kfifo_in(st, temp);
				if(UNLIKELY(ret < 0)){
					INV_ERR;
					return ret;
				}
			}
			break;
#endif
		case ICM30628_LOAD_CAL:
			ret = copy_from_user((void*)&st->calibration, data, sizeof(st->calibration));
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}		
			ret = icm30628_set_calibration_offsets(INV_ACCELEROMETER_NUM, st->calibration.accel_calibration_offset);
			if(ret < 0){
				INV_ERR;
				return -EINVAL;
			}			
			ret = icm30628_set_calibration_offsets(INV_GYROSCOPE_NUM, st->calibration.gyro_calibration_offset);
			if(ret < 0){
				INV_ERR;
				return -EINVAL;
			}			
			ret = icm30628_set_calibration_offsets(INV_MAGNETIC_FIELD_NUM, st->calibration.mag_calibration_offset);
			if(ret < 0){
				INV_ERR;
				return -EINVAL;
			}
			break;
		case ICM30628_STORE_CAL:
			ret = copy_to_user(data, (void*)&st->calibration, sizeof(st->calibration));
			if(ret != 0){
				INV_ERR;
				return -EINVAL;
			}			
			break;
		case ICM30628_KERNEL_LOG:
			ret = copy_from_user(&string, data, 128);			
			INV_INFO("%s",string);
			break;

		default:
			break;
	}
	
	return ret ;
}

static struct file_operations fops = 
{
	.owner =			THIS_MODULE,
	.read =			read,
	.write =			write,
	.unlocked_ioctl =	unlocked_ioctl,
	.open =			open,
	.release =		release,
	.llseek =			default_llseek
};

static struct miscdevice miscdev = 
{
	.minor =	MISC_DYNAMIC_MINOR,
	.name =	MODULE_NAME,
	.fops =	&fops
};

static int __init icm30628_init(void)
{
	int ret = 0;
	struct icm30628_state_t * st = NULL;

	INV_DBG_FUNC_NAME;

#ifdef MTK_PLATFORM
	hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_3300, "sensor");
	hwPowerOn(MT6323_POWER_LDO_VGP3, VOL_1800, "sensor");
	INV_INFO("init VGP1 VPG3 LDO \n");
#endif		
	icm30628_state = invensense_allocation();
	if(UNLIKELY(!icm30628_state)){
		ret = -EINVAL;
		INV_ERR;
		return ret;
	}
	st = icm30628_state;

	ret = invensense_initialize(st);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}

	ret = invensense_comm_initialize(st);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

#ifdef MTK_PLATFORM
#else
	ret = invensense_post_initialize(st);
	if(UNLIKELY(ret < 0)){
		INV_ERR;
		return ret;
	}
#endif
	ret = misc_register(&miscdev);
	if (UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

#ifdef MTK_PLATFORM
	init_factory_node();
#endif

	return ret;
}

static void __exit icm30628_exit(void)
{
	struct icm30628_state_t * st = icm30628_state;

	INV_DBG_FUNC_NAME;

	invensense_comm_terminate(st);

}

module_init(icm30628_init);
module_exit(icm30628_exit);

MODULE_AUTHOR("Invensense Corporation");
MODULE_DESCRIPTION("Invensense device driver");
MODULE_LICENSE("GPL");
