#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 3,
    .polling_mode_ps =0,
    .polling_mode_als =1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    .i2c_addr   = {0x72, 0x48, 0x78, 0x00},
    .als_level  = {1, 4, 10, 30, 120, 150, 255, 380, 650, 900, 1350, 2200, 3500, 6000, 10000, 20000, 50000},
    .als_value  = {0, 5, 10, 30, 80, 120, 225, 320, 550, 800, 1250, 2000, 3000, 5000, 5500, 7500, 13000, 24000},
    .ps_threshold_high = 1050,
    .ps_threshold_low = 980,
};
struct alsps_hw *get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}

