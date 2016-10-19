#include <linux/types.h>
#include "stk3x1x_cust_alsps.h"
#include <mach/mt_pm_ldo.h>


static struct alsps_hw cust_alsps_hw = {
	.i2c_num    = 3,
	.polling_mode_ps =0,
	.polling_mode_als = 1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    .i2c_addr   = {0x48, 0x00, 0x00, 0x00},	/*STK3x1x*/
        .als_level  = {1, 4, 10, 30, 80, 120, 225, 320, 550, 800, 1250, 2000, 3500, 6000, 10000, 20000, 50000},	/* als_code */
        .als_value  = {0, 5, 10, 30, 80, 120, 225, 320, 550, 800, 1250, 2000, 6000, 10000, 11000, 15000, 18000, 24000},    /* lux */
   	.state_val = 0x0,		/* disable all */
	.psctrl_val = 0x31,		/* ps_persistance=4, ps_gain=64X, PS_IT=0.391ms */
	.alsctrl_val = 0x39, 	/* als_persistance=1, als_gain=64X, ALS_IT=50ms */
	.ledctrl_val = 0xBF,	/* 100mA IRDR, 64/64 LED duty */
	.wait_val = 0x7,		/* 50 ms */
    .ps_threshold_high = 1050,
    .ps_threshold_low =  980,
};
struct alsps_hw *stk_get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}
