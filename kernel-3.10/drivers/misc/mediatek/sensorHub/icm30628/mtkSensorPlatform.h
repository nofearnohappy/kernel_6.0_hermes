#ifndef ICM30628_MTK_H
#define ICM30628_MTK_H

#ifdef MTK_PLATFORM
#include <accel.h>
#include <activity.h>
#include <bringtosee.h>
#include <grv.h>
#include <gmrv.h>
#include <gravity.h>
#include <gyroscope.h>
#include <heart_rate.h>
#include <linearacceleration.h>
#include <mag.h>
#include <barometer.h>
#include <alsps.h>
#include <rotationvector.h>
#include <shake.h>
#include <step_counter.h>
#include <linux/batch.h>
#endif

#define ICM30628_ACC_DEV_NAME "ICM30628_ACC" 
int  icm30628_local_accelerometer_init(void);
int  icm30628_local_accelerometer_remove(void);

#define ICM30628_GYRO_DEV_NAME "ICM30628_GYRO"
int  icm30628_local_gyroscope_init(void);
int  icm30628_local_gyroscope_remove(void);

#define ICM30628_MAG_DEV_NAME "ICM30628_MAG"
int  icm30628_local_magnetic_init(void);
int  icm30628_local_magnetic_remove(void);

#define ICM30628_GRAV_DEV_NAME "ICM30628_GRAV"
int  icm30628_local_gravity_init(void);
int  icm30628_local_gravity_remove(void);

#define ICM30628_LA_DEV_NAME "ICM30628_LA"
int  icm30628_local_linearaccel_init(void);
int  icm30628_local_linearaccel_remove(void);

#define ICM30628_GRV_DEV_NAME "ICM30628_GRV" 
int  icm30628_local_grv_init(void);
int  icm30628_local_grv_remove(void);

#define ICM30628_GMRV_DEV_NAME "ICM30628_GMRV" 
int  icm30628_local_gmrv_init(void);
int  icm30628_local_gmrv_remove(void);

#define ICM30628_RV_DEV_NAME "ICM30628_RV" 
int  icm30628_local_rotationvector_init(void);
int  icm30628_local_rotationvector_remove(void);

#define ICM30628_PRESSURE_DEV_NAME "ICM30628_PRESSURE" 
int  icm30628_local_pressure_init(void);
int  icm30628_local_pressure_remove(void);

#define ICM30628_PROXIMITY_DEV_NAME "ICM30628_PROXIMITY" 
int  icm30628_local_proximity_init(void);
int  icm30628_local_proximity_remove(void);

#define ICM30628_HRM_DEV_NAME "ICM30628_HR" 
int  icm30628_local_heartrate_init(void);
int  icm30628_local_heartrate_remove(void);

#define ICM30628_SHAKE_DEV_NAME "ICM30628_SHAKE" 
int  icm30628_local_shake_init(void);
int  icm30628_local_shake_remove(void);

#define ICM30628_BTS_DEV_NAME "ICM30628_BTS" 
int  icm30628_local_bringtosee_init(void);
int  icm30628_local_bringtosee_remove(void);

#define ICM30628_ACTIVITY_DEV_NAME "ICM30628_ACTIVITY" 
int  icm30628_local_activity_init(void);
int  icm30628_local_activity_remove(void);

#define ICM30628_STEP_C_DEV_NAME "ICM30628_STEP_C" 
int  icm30628_local_step_c_init(void);
int  icm30628_local_step_c_remove(void);

#define ICM30628_BATCH_DEV_NAME "ICM30628_BATCH"
int  icm30628_local_batch_init(void);
int  icm30628_local_batch_remove(void);

#ifndef MTK_PLATFORM
// accelerometer
struct acc_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
	int (*acc_calibration)(int type, int cali[3]);
};

struct acc_data_path
{
	int (*get_data)(int *x,int *y, int *z,int *status);
	int (*get_raw_data)(int *x,int *y, int *z);
	int vender_div;
};

struct acc_init_info
{
    char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_driver_addr;
};

extern int acc_driver_add(struct acc_init_info* obj) ;
extern int acc_data_report(int x, int y, int z,int status);
extern int acc_register_control_path(struct acc_control_path *ctl);
extern int acc_register_data_path(struct acc_data_path *data);


// gyroscope
struct gyro_init_info
{
    char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_driver_addr;
};

struct gyro_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
	int (*gyro_calibration)(int type, int cali[3]);
};

struct gyro_data_path
{
	int (*get_data)(int *x,int *y, int *z,int *status);
	int (*get_raw_data)(int *x,int *y, int *z);
	int vender_div;
};

extern int gyro_driver_add(struct gyro_init_info* obj) ;
extern int gyro_register_control_path(struct gyro_control_path *ctl);
extern int gyro_register_data_path(struct gyro_data_path *data);

// magnetic / orientation
struct mag_init_info
{
    char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

struct mag_data_path
{
	int div_m;
	int div_o;	
	int (*get_data_m)(int *x,int *y, int *z,int *status);
	int (*get_data_o)(int *x,int *y, int *z,int *status);
	int (*get_raw_data)(int *x,int *y, int *z);
	
};

struct mag_control_path
{
	int (*m_open_report_data)(int en);
	int (*m_set_delay)(u64 delay);
	int (*m_enable)(int en);
	int (*o_open_report_data)(int en);
	int (*o_set_delay)(u64 delay);
	int (*o_enable)(int en);
	bool is_report_input_direct;
	bool is_support_batch;
};

extern int mag_driver_add(struct mag_init_info* obj) ;
extern int mag_register_control_path(struct mag_control_path *ctl);
extern int mag_register_data_path(struct mag_data_path *data);

// gravity
struct grav_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
	int (*grav_calibration)(int type, int cali[3]);
};

struct grav_data_path
{
	int (*get_data)(int *x,int *y, int *z,int *status);
	int (*get_raw_data)(int *x,int *y, int *z);
	int vender_div;
};

struct grav_init_info
{
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int grav_driver_add(struct grav_init_info* obj) ;
extern int grav_register_control_path(struct grav_control_path *ctl);
extern int grav_register_data_path(struct grav_data_path *data);

// linear acceleration
struct la_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
	int (*la_calibration)(int type, int cali[3]);
};

struct la_data_path
{
	int (*get_data)(int *x,int *y, int *z,int *status);
	int (*get_raw_data)(int *x,int *y, int *z);
	int vender_div;
};

struct la_init_info
{
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int la_driver_add(struct la_init_info* obj) ;
extern int la_register_control_path(struct la_control_path *ctl);
extern int la_register_data_path(struct la_data_path *data);


// game rotation vector
struct grv_init_info
{
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_driver_addr;
};

struct grv_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
	int (*grv_calibration)(int type, int cali[3]);
};

struct grv_data_path
{
	int (*get_data)(int *x,int *y, int *z, int *scalar, int *status);
	int (*get_raw_data)(int *x,int *y, int *z, int *scalar);
	int vender_div;
};

extern int grv_driver_add(struct grv_init_info* obj) ;
extern int grv_register_control_path(struct grv_control_path *ctl);
extern int grv_register_data_path(struct grv_data_path *data);

// game rotation vector
struct gmrv_init_info
{
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_driver_addr;
};

struct gmrv_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
	int (*gmrv_calibration)(int type, int cali[3]);
};

struct gmrv_data_path
{
	int (*get_data)(int *x,int *y, int *z, int *scalar, int *status);
	int (*get_raw_data)(int *x,int *y, int *z, int *scalar);
	int vender_div;
};

extern int gmrv_driver_add(struct gmrv_init_info* obj) ;
extern int gmrv_register_control_path(struct gmrv_control_path *ctl);
extern int gmrv_register_data_path(struct gmrv_data_path *data);

// game rotation vector
struct rotationvector_init_info
{
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_driver_addr;
};

struct rotationvector_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
	int (*rotationvector_calibration)(int type, int cali[3]);
};

struct rotationvector_data_path
{
	int (*get_data)(int *x,int *y, int *z, int *scalar, int *status);
	int (*get_raw_data)(int *x,int *y, int *z, int *scalar);
	int vender_div;
};

extern int rotationvector_driver_add(struct rotationvector_init_info* obj) ;
extern int rotationvector_register_control_path(struct rotationvector_control_path *ctl);
extern int rotationvector_register_data_path(struct rotationvector_data_path *data);

// pressure
struct baro_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
};

struct baro_data_path
{
	int (*get_data)(int *value, int *status);
	int (*get_raw_data)(int type, int *value);
	int vender_div;
};

struct baro_init_info
{
    	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int baro_driver_add(struct baro_init_info* obj) ;
extern int baro_register_control_path(struct baro_control_path *ctl);
extern int baro_register_data_path(struct baro_data_path *data);

// proximity
struct ps_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);
	int (*ps_calibration)(int type, int value);
	int (*ps_threshold_setting)(int type, int value[2]);
	bool is_report_input_direct;
	bool is_support_batch;
	bool is_polling_mode;
};

struct ps_data_path
{
	int (*get_data)(int *ps_value, int *status);
	int (*ps_get_raw_data)(int *ps_value);
	int vender_div;
};

struct alsps_init_info
{
    	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int alsps_driver_add(struct alsps_init_info* obj) ;
extern int ps_report_interrupt_data(int value);
extern int ps_register_control_path(struct ps_control_path *ctl);
extern int ps_register_data_path(struct ps_data_path *data);

// heart rate
struct hr_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*set_delay)(u64 delay);
	int (*hress_data_fifo)(void);
	bool is_report_input_direct;
	bool is_support_batch;
};

struct hr_data_path
{
	int (*get_data)(int *value, int *status);
	int (*get_raw_data)(int type, int *value);
	int vender_div;
};

struct hr_init_info
{
    	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int hr_driver_add(struct hr_init_info* obj) ;
extern int hr_register_control_path(struct hr_control_path *ctl);
extern int hr_register_data_path(struct hr_data_path *data);

// shake
struct shk_control_path
{
	int (*open_report_data)(int open);
};

struct shk_data_path
{
	int (*get_data)(u16 *value, int *status);
};

struct shk_init_info
{
    	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int shk_notify(void);
extern int shk_driver_add(struct shk_init_info* obj) ;
extern int shk_register_control_path(struct shk_control_path *ctl);
extern int shk_register_data_path(struct shk_data_path *data);

// bring to see
struct bts_control_path
{
	int (*open_report_data)(int open);
};

struct bts_data_path
{
	int (*get_data)(u16 *value, int *status);
};

struct bts_init_info
{
    	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int bts_notify(void);
extern int bts_driver_add(struct bts_init_info* obj) ;
extern int bts_register_control_path(struct bts_control_path *ctl);
extern int bts_register_data_path(struct bts_data_path *data);

// activity
struct act_control_path
{
	int (*open_report_data)(int open);//open data rerport to HAL
	int (*enable_nodata)(int en);//only enable not report event to HAL
	int (*set_delay)(u64 delay);
	bool is_report_input_direct;
	bool is_support_batch;
};

struct act_data_path
{
	int (*get_data)(u16 *value, int *status);
	int vender_div;
};

struct act_init_info
{
  	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int act_driver_add(struct act_init_info* obj) ;
extern int act_register_control_path(struct act_control_path *ctl);
extern int act_register_data_path(struct act_data_path *data);


//step counter / step detector / significant motion detector
typedef enum {
	TYPE_STEP_NON   = 0,
	TYPE_STEP_DETECTOR  = 1,
	TYPE_SIGNIFICANT=2
} STEP_NOTIFY_TYPE;

struct step_c_control_path
{
	int (*open_report_data)(int open);
	int (*enable_nodata)(int en);
	int (*enable_step_detect)(int en);
	int (*enable_significant)(int en);
	int (*set_delay)(u64 delay);
	bool is_report_input_direct;
	bool is_support_batch;
};

struct step_c_data_path
{
	int (*get_data)(u64 *value, int *status);
	int (*get_data_step_d)(int *value );
	int (*get_data_significant)(int *value );
	int vender_div;
};

struct step_c_init_info
{
    	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

extern int step_c_driver_add(struct step_c_init_info* obj) ;
extern int step_c_register_control_path(struct step_c_control_path *ctl);
extern int step_c_register_data_path(struct step_c_data_path *data);
extern int  step_notify(STEP_NOTIFY_TYPE type);

// batch driver
typedef enum BATCH_NOTIFY_TYPE
{
	TYPE_BATCHFULL = 0,
	TYPE_BATCHTIMEOUT
} BATCH_NOTIFY_TYPE;

struct batch_init_info
{
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

struct batch_control_path
{
	int (*flush)(int handle);
	int (*enable_hw_batch)(int handle, int flag, long long rate, long long latency);
};

struct batch_data_path
{
	int (*get_data)(int handle, void *data);
	int (*get_fifo_status)(int * fifo_len, int * fifo_status, int idx);
	int samplingPeriodMs;
	int flags;
	int is_batch_supported;
	int div;	
};

extern int batch_driver_add(struct batch_init_info* obj) ;
extern int batch_register_control_path(int handle, struct batch_control_path *ctl);
extern int batch_register_data_path(int handle, struct batch_data_path *data);
extern int batch_register_support_info(int handle, int support,int div, bool flag);
extern int  batch_notify(BATCH_NOTIFY_TYPE type);
#endif //#ifndef MTK_PLATFORM

#endif //ICM30628_MTK_H
