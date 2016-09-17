#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_mag.h>


static struct mag_hw cust_mag_hw = {
    .i2c_num = 3,
    .i2c_addr = {0x0D,0,0,0},
    .direction = 0,
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
    .is_batch_supported = false,
};
struct mag_hw* akm09911_get_cust_mag_hw(void) 
{
    return &cust_mag_hw;
}
