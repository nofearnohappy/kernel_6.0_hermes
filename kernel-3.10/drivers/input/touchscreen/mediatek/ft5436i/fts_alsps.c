#include "tpd.h"
#include "tpd_custom_fts.h"
#include "cust_gpio_usage.h"
#include <cust_eint.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/dma-mapping.h>
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>

#include <alsps.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>


/*----------------------------------------------------------------------------*/
static int fts_remove(void)
{
	
	return 0;
}
extern int register_alsps(void);
static int fts_local_init(void) 
{
int ret;
	ret = register_alsps();
	if(ret != 0)
	{
		return ret;
	}
	return 0;
}
static struct alsps_init_info fts_init_info = {
		.name = "fts_alsps",
		.init = fts_local_init,
		.uninit = fts_remove,
	
};
/*----------------------------------------------------------------------------*/

static int __init fts_init(void)
{
#ifdef TPD_PROXIMITY
	alsps_driver_add(&fts_init_info);
#endif
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit fts_exit(void)
{
	
}


module_init(fts_init);
module_exit(fts_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("YC Hou");
MODULE_DESCRIPTION("fts driver");
MODULE_LICENSE("GPL");
