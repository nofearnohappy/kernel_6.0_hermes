#ifndef __BTS_H__
#define __BTS_H__


#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/hwmsensor.h>
#include <linux/earlysuspend.h> 
#include <linux/hwmsen_dev.h>


#define BTS_TAG		"<BRINGTOSEE> "
#define BTS_FUN(f)		printk(BTS_TAG"%s\n", __func__)
#define BTS_ERR(fmt, args...)	printk(BTS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define BTS_LOG(fmt, args...)	printk(BTS_TAG fmt, ##args)
#define BTS_VER(fmt, args...)  printk(BTS_TAG"%s: "fmt, __func__, ##args) //((void)0)

//#define OP_BTS_DELAY		0X01
#define	OP_BTS_ENABLE		0X02
//#define OP_BTS_GET_DATA	0X04

#define BTS_INVALID_VALUE -1

#define EVENT_TYPE_BTS_VALUE		REL_X

#define BTS_VALUE_MAX (32767)
#define BTS_VALUE_MIN (-32768)
#define BTS_STATUS_MIN (0)
#define BTS_STATUS_MAX (64)
#define BTS_DIV_MAX (32767)
#define BTS_DIV_MIN (1)

typedef enum {
	BTS_DEACTIVATE,
	BTS_ACTIVATE,
	BTS_SUSPEND,
	BTS_RESUME
} bts_state_e;

struct bts_control_path
{
//	int (*enable_nodata)(int en);//only enable not report event to HAL
	int (*open_report_data)(int open);//open data rerport to HAL
//	int (*enable)(int en);
	//bool is_support_batch;//version2.used for batch mode support flag
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

struct bts_data{
	hwm_sensor_data bts_data ;
	int data_updata;
	//struct mutex lock;
};

struct bts_drv_obj {
    void *self;
	int polling;
	int (*bts_operate)(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout);
};

struct bts_context {
	struct input_dev   *idev;
	struct miscdevice   mdev;
	struct work_struct  report;
	struct mutex bts_op_mutex;
	atomic_t            wake;  /*user-space request to wake-up, used with stop*/
	atomic_t            trace;

	struct early_suspend    early_drv;
	atomic_t                early_suspend;
	atomic_t                suspend;

	struct bts_data       drv_data;
	struct bts_control_path   bts_ctl;
	struct bts_data_path   bts_data;
	bool			is_active_nodata;		// Active, but HAL don't need data sensor. such as orientation need
	bool			is_active_data;		// Active and HAL need data .
	bool 		is_batch_enable;	//version2.this is used for judging whether sensor is in batch mode
};

extern int bts_notify(void);
extern int bts_driver_add(struct bts_init_info* obj) ;
extern int bts_register_control_path(struct bts_control_path *ctl);
extern int bts_register_data_path(struct bts_data_path *data);

#endif
