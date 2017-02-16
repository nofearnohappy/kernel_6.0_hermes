/*
 * Gas_Gauge driver for CW2015/2013
 * Copyright (C) 2012, CellWise
 * Copyright (C) 2016 XiaoMi, Inc.
 *
 * Revised for Xiaomi Redmi Note 3 MTK by:
 *   Smosia 
 *
 * Authors: ChenGang <ben.chen@cellwise-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.And this driver depends on
 * I2C and uses IIC bus for communication with the host.
 *
 */
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <cust_charging.h>
#include <mach/charging.h>
#include <mach/mt_gpio.h>

/***************************************
 *      define 
 ***************************************/
#define REG_VERSION             0x0
#define REG_VCELL               0x2
#define REG_SOC                 0x4
#define REG_RRT_ALERT           0x6
#define REG_CONFIG              0x8
#define REG_MODE                0xA
#define REG_BATINFO             0x10

#define MODE_SLEEP_MASK         (0x3<<6)
#define MODE_SLEEP              (0x3<<6)
#define MODE_NORMAL             (0x0<<6)
#define MODE_QUICK_START        (0x3<<4)
#define MODE_RESTART            (0xf<<0)

#define CONFIG_UPDATE_FLG       (0x1<<1)
#define ATHD                    (0x0<<3)

#define BATTERY_UP_MAX_CHANGE                   420
#define BATTERY_DOWN_CHANGE                     60
#define BATTERY_DOWN_MIN_CHANGE_RUN             30
#define BATTERY_DOWN_MIN_CHANGE_SLEEP           1800
#define BATTERY_DOWN_MAX_CHANGE_RUN_AC_ONLINE   1800

#define SIZE_BATINFO            64

#define USB_CHARGER_MODE        1
#define AC_CHARGER_MODE         2

#define I2C_BUSNUM              4
#define CW_I2C_SPEED            100000          // default i2c speed set 100khz
#define CW2015_DEV_NAME         "CW2015"

/***************************************
 *      global 
 ***************************************/
int cw_capacity;

static int battery_type_id = 1;

extern int FG_charging_type;
extern int FG_charging_status;

static const struct i2c_device_id FG_CW2015_i2c_id[] = {{CW2015_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_FG_CW2015 = { I2C_BOARD_INFO("CW2015", 0x62)};

static u8 config_info[SIZE_BATINFO] = {
    0x17, 0xF3, 0x63, 0x6A, 0x6A, 0x68, 0x68, 0x65, 0x63, 0x60, 
    0x5B, 0x59, 0x65, 0x5B, 0x46, 0x41, 0x36, 0x31, 0x28, 0x27, 
    0x31, 0x35, 0x43, 0x51, 0x1C, 0x3B, 0x0B, 0x85, 0x22, 0x42, 
    0x5B, 0x82, 0x99, 0x92, 0x98, 0x96, 0x3D, 0x1A, 0x66, 0x45, 
    0x0B, 0x29, 0x52, 0x87, 0x8F, 0x91, 0x94, 0x52, 0x82, 0x8C, 
    0x92, 0x96, 0x54, 0xC2, 0xBA, 0xCB, 0x2F, 0x7D, 0x72, 0xA5, 
    0xB5, 0xC1, 0xA5, 0x49
};
static u8 config_info_des[SIZE_BATINFO] = {
    0x17, 0xF9, 0x6D, 0x6D, 0x6B, 0x67, 0x65, 0x64, 0x58, 0x6D, 
    0x6D, 0x48, 0x57, 0x5D, 0x4A, 0x43, 0x37, 0x31, 0x2B, 0x20, 
    0x24, 0x35, 0x44, 0x55, 0x20, 0x37, 0x0B, 0x85, 0x2A, 0x4A, 
    0x56, 0x68, 0x74, 0x6B, 0x6D, 0x6E, 0x3C, 0x1A, 0x5C, 0x45, 
    0x0B, 0x30, 0x52, 0x87, 0x8F, 0x91, 0x94, 0x52, 0x82, 0x8C, 
    0x92, 0x96, 0x64, 0xB4, 0xDB, 0xCB, 0x2F, 0x7D, 0x72, 0xA5, 
    0xB5, 0xC1, 0xA5, 0x42
};

struct cw_bat_platform_data {
    int (*io_init)(void);

    int is_usb_charge;
    int is_dc_charge;
    u8 *cw_bat_config_info;
    u32 irq_flags;
    u32 dc_det_pin;
    u32 dc_det_level;

    u32 bat_low_pin;
    u32 bat_low_level;
    u32 chg_ok_pin;
    u32 chg_ok_level;
    u32 chg_mode_sel_pin;
    u32 chg_mode_sel_level;
};

static struct cw_bat_platform_data cw_bat_platdata = {    
    .bat_low_pin    = 0,
    .bat_low_level  = 0,   
    .chg_ok_pin   = 0,
    .chg_ok_level = 0,
    
    .is_usb_charge = 0,    
    .cw_bat_config_info = config_info,
};

struct cw_battery {
    struct i2c_client *client;
    struct workqueue_struct *battery_workqueue;
    struct delayed_work battery_delay_work;
    struct delayed_work dc_wakeup_work;
    struct delayed_work bat_low_wakeup_work;
    struct cw_bat_platform_data *plat_data;

    struct power_supply rk_bat;
    struct power_supply rk_ac;
    struct power_supply rk_usb;

    long sleep_time_capacity_change;
    long run_time_capacity_change;

    long sleep_time_charge_start;
    long run_time_charge_start;

    int dc_online;
    int usb_online;
    int charger_mode;
    int charger_init_mode;
    int capacity;
    int voltage;
    int status;
    int time_to_empty;
    int alt;

    int bat_change;
};

/***************************************
 *      prototypes 
 ***************************************/
static int cw_write(struct i2c_client *client, u8 reg, u8 const buf[]);
static int cw_read(struct i2c_client *client, u8 reg, u8 buf[]);
static int cw_read_word(struct i2c_client *client, u8 reg, u8 buf[]);
static int cw_update_config_info(struct cw_battery *cw_bat);
static int cw_check_ic(struct cw_battery *cw_bat);
static int cw_init(struct cw_battery *cw_bat);
static void cw_update_time_member_charge_start(struct cw_battery *cw_bat);
static void cw_update_time_member_capacity_change(struct cw_battery *cw_bat);
static int cw_quickstart(struct cw_battery *cw_bat);
static int cw_get_capacity(struct cw_battery *cw_bat);
static int cw_get_vol(struct cw_battery *cw_bat);
static int cw_get_time_to_empty(struct cw_battery *cw_bat);
static void rk_bat_update_capacity(struct cw_battery *cw_bat);
static void rk_bat_update_vol(struct cw_battery *cw_bat);
static int rk_usb_update_online(struct cw_battery *cw_bat);
static void rk_bat_update_time_to_empty(struct cw_battery *cw_bat);
static void cw_bat_work(struct work_struct *work);
static int cw2015_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int cw2015_i2c_remove(struct i2c_client *client);
static int cw2015_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int cw2015_i2c_suspend(struct i2c_client *client, pm_message_t mesg);
static int cw2015_i2c_resume(struct i2c_client *client);

extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);


/***************************************
 *      functions 
 ***************************************/
static int liuchao_test_hmi_battery_version = 1;

static void hmi_get_battery_version()
{
    int i = 1;
    i = simple_strtol(strstr(saved_command_line, "batversion=")+12, 0, 10);
    liuchao_test_hmi_battery_version = i; //COS = 1, DES = 2
}

static int cw_write(struct i2c_client *client, u8 reg, u8 const buf[])
{
    int ret = 0;
    ret = i2c_smbus_write_byte_data(client, reg, buf[0]);
    return ret;
}

static int cw_read(struct i2c_client *client, u8 reg, u8 buf[])
{
    int ret = 0;
    ret = i2c_smbus_read_byte_data(client, reg);
    //printk("[CW2015] cw_read buf2 = %d\n", ret); 
    if (ret < 0)
        return ret;
    else
    {
        buf[0] = ret;
        ret = 0;
    }
    return ret;
}

static int cw_read_word(struct i2c_client *client, u8 reg, u8 buf[])
{
    int ret = 0;
    unsigned int data = 0;
    data = i2c_smbus_read_word_data(client, reg);
    buf[0] = data & 0x00FF;
    buf[1] = (data & 0xFF00)>>8;
    return ret;
}

static int cw_update_config_info(struct cw_battery *cw_bat)
{
    int ret;
    u8 reg_val;
    int i;
    u8 reset_val;

    printk("[CW2015] [cw_update_config_info]: start\n");

    /* make sure no in sleep mode */
    ret = cw_read(cw_bat->client, REG_MODE, &reg_val);
    if (ret < 0)
        return ret;

    reset_val = reg_val;
    if ((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP) 
    {
        printk("[CW2015] Error, device in sleep mode, cannot update battery info\n");
        return -EPERM;
    }

    /* update new battery info */
    for (i = 0; i < SIZE_BATINFO; i++) 
    {
        printk("[CW2015] cw_bat->plat_data->cw_bat_config_info[%d] = 0x%x\n", i, \
                cw_bat->plat_data->cw_bat_config_info[i]);
        ret = cw_write(cw_bat->client, REG_BATINFO + i, &cw_bat->plat_data->cw_bat_config_info[i]);

        if (ret < 0)
            return ret;
    }

    /* readback & check */
    for (i = 0; i < SIZE_BATINFO; i++) 
    {
        ret = cw_read(cw_bat->client, REG_BATINFO + i, &reg_val);
        if (reg_val != cw_bat->plat_data->cw_bat_config_info[i])
            return -EPERM;
    }

    /* set cw2015/cw2013 to use new battery info */
    ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
    if (ret < 0)
        return ret;

    reg_val |= CONFIG_UPDATE_FLG;   /* set UPDATE_FLAG */
    reg_val &= 0x07;                /* clear ATHD */
    reg_val |= ATHD;                /* set ATHD */
    ret = cw_write(cw_bat->client, REG_CONFIG, &reg_val);
    if (ret < 0)
        return ret;

    /* check 2015/cw2013 for ATHD & update_flag */
    ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
    if (ret < 0)
        return ret;

    if (!(reg_val & CONFIG_UPDATE_FLG)) 
    {
        printk("[CW2015] update flag for new battery info have not set..\n");
        reg_val = MODE_SLEEP;
        ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
        printk("[CW2015] report battery capacity error\n");
        return -EPERM;
    }

    if ((reg_val & 0xf8) != ATHD) 
        printk("[CW2015] the new ATHD have not set..\n");
    
    /* reset */
    reset_val &= ~(MODE_RESTART);
    reg_val = reset_val | MODE_RESTART;
    ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
    if (ret < 0)
        return ret;

    msleep(10);
    ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
    if (ret < 0)
        return ret;

    return 0;
}

static int cw_check_ic(struct cw_battery *cw_bat)
{
    int ret = 1 ;
    u8 reg_val = 0;

    ret = cw_read(cw_bat->client, REG_MODE/*REG_VERSION*/, &reg_val);

    if (ret < 0)
        return ret;

    if ((reg_val == 0xC0) || (reg_val == 0x00))
        ret = 0;

    return ret;
}

static int cw_init(struct cw_battery *cw_bat)
{
    int ret;
    int i;
    u8 reg_val = MODE_SLEEP;
    hmi_get_battery_version();

    if ((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP)
    {
        reg_val = MODE_NORMAL;
        ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
        if (ret < 0)
            return ret;
    }

    ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
    if (ret < 0)
        return ret;

    if ((reg_val & 0xf8) != ATHD)
    {
        printk("[CW2015] the new ATHD have not set\n");
        reg_val &= 0x07;    /* clear ATHD */
        reg_val |= ATHD;    /* set ATHD */
        ret = cw_write(cw_bat->client, REG_CONFIG, &reg_val);
        if (ret < 0)
            return ret;
    }

    ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
    if (ret < 0)
        return ret;

    if (!(reg_val & CONFIG_UPDATE_FLG))
    {
        printk("[CW2015] update flag for new battery info have not set\n");
        ret = cw_update_config_info(cw_bat);
        if (ret < 0)
            return ret;
    }
    else 
    {
        for (i = 0; i < SIZE_BATINFO; i++) 
        {
            ret = cw_read(cw_bat->client, (REG_BATINFO + i), &reg_val);
            if (ret < 0)
                return ret;

            if (cw_bat->plat_data->cw_bat_config_info[i] != reg_val)
                break;
        }

        if (i != SIZE_BATINFO) 
        {
            printk("[CW2015] update flag for new battery info have not set\n");
            ret = cw_update_config_info(cw_bat);
            if (ret < 0)
                return ret;
        }
    }

    for (i = 0; i < 30; i++) 
    {
        msleep(100);
        ret = cw_read(cw_bat->client, REG_SOC, &reg_val);
        if (ret < 0)
            return ret;
        else if (reg_val <= 0x64)
            return 0;
            //break;

        if (i > 25)
            printk("[CW2015] cw2015/cw2013 input unvalid power error\n");
    }

    reg_val = MODE_SLEEP;
    ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
    printk("[CW2015] report battery capacity error\n");

    return -EPERM;

}

static void cw_update_time_member_charge_start(struct cw_battery *cw_bat)
{
    struct timespec ts;
    int new_run_time;
    int new_sleep_time;

    ktime_get_ts(&ts);
    new_run_time = ts.tv_sec;

    get_monotonic_boottime(&ts);
    new_sleep_time = ts.tv_sec - new_run_time;

    cw_bat->run_time_charge_start = new_run_time;
    cw_bat->sleep_time_charge_start = new_sleep_time;
}

static void cw_update_time_member_capacity_change(struct cw_battery *cw_bat)
{
    struct timespec ts;
    int new_run_time;
    int new_sleep_time;

    ktime_get_ts(&ts);
    new_run_time = ts.tv_sec;

    get_monotonic_boottime(&ts);
    new_sleep_time = ts.tv_sec - new_run_time;

    cw_bat->run_time_capacity_change = new_run_time;
    cw_bat->sleep_time_capacity_change = new_sleep_time;
}

static int cw_quickstart(struct cw_battery *cw_bat)
{
    int ret = 0;
    u8 reg_val = MODE_QUICK_START;

    ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
    if (ret < 0) 
    {
        printk("[CW2015] Error quick start1\n");
        return ret;
    }

    reg_val = MODE_NORMAL;
    ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
    if (ret < 0) 
    {
        printk("[CW2015] Error quick start2\n");
        return ret;
    }
    return 1;
}

static int cw_get_capacity(struct cw_battery *cw_bat)
{

    int ret;
    u8 reg_val[2];

    struct timespec ts;
    long new_run_time;
    long new_sleep_time;
    long capacity_or_aconline_time;
    int allow_change;
    int allow_capacity;
    static int if_quickstart;
    static int jump_flag;
    static int jump_flag2;
    static int reset_loop;
    int charge_time;
    u8 reset_val;

    ret = cw_read_word(cw_bat->client, REG_SOC, reg_val);
    if (ret < 0)
        return ret;

    cw_capacity = reg_val[0];
    if ((cw_capacity < 0) || (cw_capacity > 100)) 
    {
        printk("[CW2015] get cw_capacity error; cw_capacity = %d\n", cw_capacity);
        reset_loop++;

        if (reset_loop > 5) 
        {
            reset_val = MODE_SLEEP;
            ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
            if (ret < 0)
                return ret;
            reset_val = MODE_NORMAL;
            msleep(10);
            ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
            if (ret < 0)
                return ret;
            printk("[CW2015] report battery capacity error\n");
            ret = cw_update_config_info(cw_bat);
            if (ret)
                return ret;
            reset_loop = 0;
        }

        return cw_capacity;
    } 
    else 
        reset_loop = 0;

    if (cw_capacity == 0)
        printk("[CW2015] the cw201x capacity is 0 !!!!!!!, funciton: %s, line: %d\n", __func__, __LINE__);
    else
        printk("[CW2015] the cw201x capacity is %d, funciton: %s\n", cw_capacity, __func__);

    ktime_get_ts(&ts);
    new_run_time = ts.tv_sec;

    get_monotonic_boottime(&ts);
    new_sleep_time = ts.tv_sec - new_run_time;

    if (((cw_bat->charger_mode > 0) && (cw_capacity <= (cw_bat->capacity - 1)) && (cw_capacity > (cw_bat->capacity - 9/*30*/)))
                || ((cw_bat->charger_mode == 0) && (cw_capacity == (cw_bat->capacity + 1)))) 
    {
        if (!(cw_capacity == 0 && cw_bat->capacity <= 2)) 
            cw_capacity = cw_bat->capacity;
    }

    if ((cw_bat->charger_mode > 0) && (cw_capacity >= 95) && (cw_capacity <= cw_bat->capacity)) 
    {
        capacity_or_aconline_time = (cw_bat->sleep_time_capacity_change > cw_bat->sleep_time_charge_start) ? cw_bat->sleep_time_capacity_change : cw_bat->sleep_time_charge_start;
        capacity_or_aconline_time += (cw_bat->run_time_capacity_change > cw_bat->run_time_charge_start) ? cw_bat->run_time_capacity_change : cw_bat->run_time_charge_start;
        allow_change = (new_sleep_time + new_run_time - capacity_or_aconline_time) / BATTERY_UP_MAX_CHANGE;
        if (allow_change > 0) 
        {
            allow_capacity = cw_bat->capacity + allow_change;
            cw_capacity = (allow_capacity <= 100) ? allow_capacity : 100;
            jump_flag = 1;
        } 
        else if (cw_capacity <= cw_bat->capacity) 
            cw_capacity = cw_bat->capacity;
    } 
    else if ((cw_bat->charger_mode == 0) && cw_bat->capacity == 100 && cw_capacity < cw_bat->capacity && jump_flag2 == 0) 
    {
        cw_capacity = cw_bat->capacity;
        jump_flag2 = 1;
    } 
    else if ((cw_bat->charger_mode == 0) && (cw_capacity <= cw_bat->capacity) && (cw_capacity >= 90) && ((jump_flag == 1) || (jump_flag2 == 1))) 
    {
        capacity_or_aconline_time = (cw_bat->sleep_time_capacity_change > cw_bat->sleep_time_charge_start) ? cw_bat->sleep_time_capacity_change : cw_bat->sleep_time_charge_start;
        capacity_or_aconline_time += (cw_bat->run_time_capacity_change > cw_bat->run_time_charge_start) ? cw_bat->run_time_capacity_change : cw_bat->run_time_charge_start;
        allow_change = (new_sleep_time + new_run_time - capacity_or_aconline_time) / BATTERY_DOWN_CHANGE;
        if (allow_change > 0) 
        {
            allow_capacity = cw_bat->capacity - allow_change;
            if (cw_capacity >= allow_capacity) 
            {
                jump_flag = 0;
                jump_flag2 = 0;
            } 
            else 
                cw_capacity = (allow_capacity <= 100) ? allow_capacity : 100;
        } 
        else if (cw_capacity <= cw_bat->capacity) 
            cw_capacity = cw_bat->capacity;
    }

    if ((cw_capacity == 0) && (cw_bat->capacity > 1)) 
    {
        allow_change = ((new_run_time - cw_bat->run_time_capacity_change) / BATTERY_DOWN_MIN_CHANGE_RUN);
        allow_change += ((new_sleep_time - cw_bat->sleep_time_capacity_change) / BATTERY_DOWN_MIN_CHANGE_SLEEP);
        allow_capacity = cw_bat->capacity - allow_change;
        cw_capacity = (allow_capacity >= cw_capacity) ? allow_capacity : cw_capacity;
        printk("[CW2015] report GGIC POR happened\n");
        reg_val[0] = MODE_NORMAL;
        ret = cw_write(cw_bat->client, REG_MODE, reg_val);
        if (ret < 0)
            return ret;
        printk("[CW2015] report battery capacity jump 0\n");
    }

    if ((cw_bat->charger_mode > 0) && (cw_capacity == 0)) 
    {
        charge_time = new_sleep_time + new_run_time - cw_bat->sleep_time_charge_start - cw_bat->run_time_charge_start;
        if ((charge_time > BATTERY_DOWN_MAX_CHANGE_RUN_AC_ONLINE) && (if_quickstart == 0)) 
        {
            cw_quickstart(cw_bat);
            reset_val = MODE_SLEEP;
            ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
            if (ret < 0)
                return ret;
                reset_val = MODE_NORMAL;
                msleep(10);
                ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
                if (ret < 0)
                    return ret;
                printk("[CW2015] report battery capacity error\n");
                ret = cw_update_config_info(cw_bat);
                if (ret)
                    return ret;
                printk("[CW2015] report battery capacity still 0 if in changing\n");
                if_quickstart = 1;
        }
    } 
    else if ((if_quickstart == 1) && (cw_bat->charger_mode == 0)) 
        if_quickstart = 0;

    return cw_capacity;
}

static int cw_get_vol(struct cw_battery *cw_bat)
{
    int ret;
    u8 reg_val[2];
    u16 value16, value16_1, value16_2, value16_3;
    int voltage;

    ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
    if (ret < 0)
        return ret;
    value16 = (reg_val[0] * 256) + reg_val[1];

    ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
    if (ret < 0)
        return ret;
    value16_1 = (reg_val[0] << 8) + reg_val[1];

    ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
    if (ret < 0)
        return ret;
    value16_2 = (reg_val[0] << 8) + reg_val[1];

    if (value16 > value16_1) 
    {
        value16_3 = value16;
        value16 = value16_1;
        value16_1 = value16_3;
    }

    if (value16_1 > value16_2) 
    {
        value16_3 = value16_1;
        value16_1 = value16_2;
        value16_2 = value16_3;
    }

    if (value16 > value16_1) 
    {
        value16_3 = value16;
        value16 = value16_1;
        value16_1 = value16_3;
    }

    voltage = value16_1 * 312 / 1024;
    voltage = voltage * 1000;

    printk("[CW2015] [cw_get_vol]: cw_voltage = %d\n", voltage);

    return voltage;
}


static int cw_get_time_to_empty(struct cw_battery *cw_bat)
{
    int ret;
    u8 reg_val;
    u16 value16;

    ret = cw_read(cw_bat->client, REG_RRT_ALERT, &reg_val);
    if (ret < 0)
        return ret;

    value16 = reg_val;

    ret = cw_read(cw_bat->client, REG_RRT_ALERT + 1, &reg_val);
    if (ret < 0)
        return ret;

    value16 = ((value16 << 8) + reg_val) & 0x1fff;
    return value16;
}

static void rk_bat_update_capacity(struct cw_battery *cw_bat)
{
    cw_capacity = cw_get_capacity(cw_bat);
    if ((cw_capacity >= 0) && (cw_capacity <= 100) && (cw_bat->capacity != cw_capacity))
    {
        cw_bat->capacity = cw_capacity;
        cw_bat->bat_change = 1;
        cw_update_time_member_capacity_change(cw_bat);

        if (cw_bat->capacity == 0)
            printk("[CW2015] report battery capacity 0 and will shutdown if no changing\n");
    }
}

static void rk_bat_update_vol(struct cw_battery *cw_bat)
{
    int ret;
    ret = cw_get_vol(cw_bat);

    if ((ret >= 0) && (cw_bat->voltage != ret))
    {
        cw_bat->voltage = ret;
        cw_bat->bat_change = 1;
    }
}


static int rk_usb_update_online(struct cw_battery *cw_bat)
{
    
    printk("[CW2015] [rk_usb_update_online] FG_charging_status = %d\n", FG_charging_status);
    printk("[CW2015] [rk_usb_update_online] FG_charging_type = %d\n", FG_charging_type);
    
    if (FG_charging_status)
    {
        if (FG_charging_type == STANDARD_HOST)
        {
            cw_bat->charger_mode = USB_CHARGER_MODE;
            if (cw_bat->charger_mode == 1)
            {
                if (cw_bat->usb_online == 1)
                    return 0;
                else
                {
                    cw_bat->usb_online = 1;
                    cw_update_time_member_charge_start(cw_bat);
                    return 0;
                }
            }
            else
            {
                cw_bat->charger_mode = USB_CHARGER_MODE;
                if (cw_bat->usb_online == 1)
                    return 1;
                else
                {
                    cw_bat->usb_online = 1;
                    cw_update_time_member_charge_start(cw_bat);
                    return 1;
                }
            }
        }
        else
        {
            cw_bat->charger_mode = AC_CHARGER_MODE;
            if (cw_bat->charger_mode == 2)
            {
                if (cw_bat->usb_online == 1)
                    return 0;
                else
                {
                    cw_bat->usb_online = 1;
                    cw_update_time_member_charge_start(cw_bat);
                    return 0;
                }
            }
            else
            {
                cw_bat->charger_mode = AC_CHARGER_MODE;
                if (cw_bat->usb_online == 1)
                    return 1;
                else
                {
                    cw_bat->usb_online = 1;
                    cw_update_time_member_charge_start(cw_bat);
                    return 1;
                }
            }
        }
    }
    else
    {
        cw_bat->charger_mode = FG_charging_status;
        if (cw_bat->usb_online == 0) 
            return 0;
        else
        {
            cw_bat->charger_mode = FG_charging_status;
            cw_update_time_member_charge_start(cw_bat);
            cw_bat->usb_online = 0;
            return 1;
        }
    }
}

static void rk_bat_update_time_to_empty(struct cw_battery *cw_bat)
{
    int ret;
    ret = cw_get_time_to_empty(cw_bat);
    if ((ret >= 0) && (cw_bat->time_to_empty != ret))
    {
        cw_bat->time_to_empty = ret;
        cw_bat->bat_change = 1;
    }
}

static void cw_bat_work(struct work_struct *work)
{
    int ret;
    struct delayed_work *delay_work;
    struct cw_battery *cw_bat;

    printk("[CW2015] cw_bat_work started\n");

    delay_work = container_of(work, struct delayed_work, work);
    cw_bat = container_of(delay_work, struct cw_battery, battery_delay_work);

    ret = rk_usb_update_online(cw_bat);
    if (ret == 1) 
        rk_usb_update_online(cw_bat);

    if (cw_bat->usb_online == 1) 
        ret = rk_usb_update_online(cw_bat);

    rk_bat_update_capacity(cw_bat);
    rk_bat_update_vol(cw_bat);
    rk_bat_update_time_to_empty(cw_bat);

    if (cw_bat->bat_change) 
    {
        //printk("[CW2015] power supply changed\n");
        cw_bat->bat_change = 0;
    }

    queue_delayed_work(cw_bat->battery_workqueue, &cw_bat->battery_delay_work, msecs_to_jiffies(10000));

    printk("[CW2015] cw_bat->bat_change = %d, cw_bat->time_to_empty = %d, cw_bat->capacity = %d, cw_bat->voltage = %d\n", \
        cw_bat->bat_change, cw_bat->time_to_empty, cw_bat->capacity, cw_bat->voltage);
}


static int cw2015_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct cw_battery *cw_bat;
    int ret;
    int loop = 0;

    printk("[CW2015] [cw2015_i2c_probe]: start\n");
    
    mt_set_gpio_mode(GPIO_I2C4_SDA_PIN, GPIO_I2C4_SDA_PIN_M_SDA);
    mt_set_gpio_mode(GPIO_I2C4_SCA_PIN, GPIO_I2C4_SCA_PIN_M_SCL);
    
    cw_bat = kzalloc(sizeof(struct cw_battery), GFP_KERNEL);
    if (!cw_bat) 
    {
        printk("[CW2015] fail to allocate memory\n");
        return -ENOMEM;
    }

    switch (battery_type_id)
    {
        case 1:
            cw_bat_platdata.cw_bat_config_info = config_info;
            break;
        case 2:
            cw_bat_platdata.cw_bat_config_info = config_info_des;
            break;
        default:
            printk("[CW2015] Battery type ID not match\n");
            cw_bat_platdata.cw_bat_config_info = config_info;
    }

    memset(cw_bat, 0, sizeof(*cw_bat));    

    i2c_set_clientdata(client, cw_bat);
    cw_bat->client = client;
    cw_bat->plat_data = &cw_bat_platdata;

    ret = cw_check_ic(cw_bat);

    while ((loop++ < 5) && (ret != 0)) 
    {
        printk("[CW2015] check ret is %d, loop is %d \n", ret, loop);
        ret = cw_check_ic(cw_bat);
    }
    if (ret != 0) 
    {
        printk("[CW2015] wc_check_ic fail , return  ENODEV \n");
        return -ENODEV;
    }

    ret = cw_init(cw_bat);
    while ((loop++ < 2000) && (ret != 0)) 
    {
        ret = cw_init(cw_bat);
    }

    if (ret) 
        return ret;

    cw_bat->charger_mode = 0;
    cw_bat->capacity = 0;
    cw_bat->voltage = 0;
    cw_bat->status = 0;
    cw_bat->time_to_empty = 0;
    cw_bat->bat_change = 0;

    cw_update_time_member_capacity_change(cw_bat);
    cw_update_time_member_charge_start(cw_bat);

    cw_bat->battery_workqueue = create_singlethread_workqueue("rk_battery");
    INIT_DELAYED_WORK(&cw_bat->battery_delay_work, cw_bat_work);

    queue_delayed_work(cw_bat->battery_workqueue, &cw_bat->battery_delay_work, msecs_to_jiffies(10));

    printk("cw2015/cw2013 driver v1.2 probe sucess\n");
    return 0;
}

static int cw2015_i2c_remove(struct i2c_client *client)
{
    struct cw_battery *data = i2c_get_clientdata(client); 
    printk("[CW2015] cw2015_i2c_remove\n");  
    cancel_delayed_work(&data->battery_delay_work);
    i2c_unregister_device(client);
    kfree(data);
    return 0;
}

static int cw2015_i2c_detect(struct i2c_client *client, struct i2c_board_info *info) 
{    
    printk("[CW2015] cw2015_i2c_detect\n");
    strcpy(info->type, CW2015_DEV_NAME);
    return 0;
}

static int cw2015_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{ 
    struct cw_battery *cw_bat = i2c_get_clientdata(client);
    printk("[CW2015] cw2015_i2c_suspend\n");
    cancel_delayed_work(&cw_bat->battery_delay_work);
    return 0;
}

static int cw2015_i2c_resume(struct i2c_client *client)
{
    struct cw_battery *cw_bat = i2c_get_clientdata(client);
    printk("[CW2015] cw2015_i2c_resume\n");
    queue_delayed_work(cw_bat->battery_workqueue, &cw_bat->battery_delay_work, msecs_to_jiffies(100));
    return 0;
}

static struct i2c_driver cw2015_i2c_driver = 
{  
    .probe      = cw2015_i2c_probe,
    .remove     = cw2015_i2c_remove,
    .detect     = cw2015_i2c_detect,
    .suspend    = cw2015_i2c_suspend,
    .resume     = cw2015_i2c_resume,
    .id_table   = FG_CW2015_i2c_id,
    .driver = 
    {
        .name   = CW2015_DEV_NAME,
    },
};

static int __init cw_bat_init(void)
{
    i2c_register_board_info(I2C_BUSNUM, &i2c_FG_CW2015, 1);   
    if(i2c_add_driver(&cw2015_i2c_driver))
    {
        printk("[CW2015] add driver error\n");
        return -1;
    }
    printk("[CW2015] cw_bat_init done\n");
    return 0;
}

static void __exit cw_bat_exit(void)
{
    printk("[CW2015] cw_bat_exit\n");
    i2c_del_driver(&cw2015_i2c_driver);
}

module_init(cw_bat_init);
module_exit(cw_bat_exit);

MODULE_AUTHOR("ben<ben.chen@cellwise-semi.com>");
MODULE_DESCRIPTION("cw2015/cw2013 battery driver");
MODULE_LICENSE("GPL");
