#ifndef __TOUCHPANEL_H__
#define __TOUCHPANEL_H__

#include <linux/hrtimer.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
//#include <linux/io.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/byteorder/generic.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/rtpm_prio.h>

#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>

#include <cust_eint.h>
#include <linux/jiffies.h>


/**********************Custom define begin**********************************************/


#define TPD_POWER_SOURCE_CUSTOM         10
#define IIC_PORT                   					2				//MT6572: 1  MT6589:0 , Based on the I2C index you choose for TPM
/*
///// ***** virtual key  definition  ***** /////

Below are the recommend  virtual key definition for different resolution TPM.

HVGA  320x480     2key ( (80,530);(240,530) )           3key  ( (80,530);(160;530);(240,530) )          4key   ( (40,530);(120;530);(200,530);(280,530)  )
WVGA  480x800     2key ( (80,900);(400,900) )           3key  ( (80,900);(240,900);(400,900) )          4key   ( (60,900);(180;900);(300,900);(420,900)  )
FWVGA 480x854     2key ( (80,900);(400,900) )           3key  ( (80,900);(240,900);(400,900) )          4key   ( (60,900);(180;900);(300,900);(420,900)  )
QHD   540x960     2key ( (90,1080);(450,1080) )         3key  ( (90,1080);(270,1080);(450,1080) )       4key   ( (90,1080);(180;1080);(360,1080);(450,1080)  )
HD    1280x720    2key ( (120,1350);(600,1350) )        3key  ( (120,1350);(360,1350);(600,1350) )      4key   ( (120,1080);(240;1080);(480,1080);(600,1080)  )
FHD   1920x1080   2key ( (160,2100);(920,2100) )        3key  ( (160,2100);(540,2100);(920,2100) )      4key   ( (160,2100);(320;1080);(600,1080);(920,2100)  )
*/
#define TPD_HAVE_BUTTON									// if have virtual key,need define the MACRO
#define TPD_BUTTON_HEIGH        				(40)  			//100
#define TPD_KEY_COUNT           				3    				//  4
#define TPD_KEYS                					{ KEY_MENU, KEY_HOMEPAGE, KEY_BACK}
#define TPD_KEYS_DIM            	{{160,2100,20,TPD_BUTTON_HEIGH},{540,2100,20,TPD_BUTTON_HEIGH},{920,2100,20,TPD_BUTTON_HEIGH}}
/*********************Custom Define end*************************************************/

#define TPD_NAME    "ft5x46"

/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE

#define TPD_I2C_NUMBER           		2
#define TPD_WAKEUP_TRIAL         		60
#define TPD_WAKEUP_DELAY         		100

#define VELOCITY_CUSTOM
#define TPD_VELOCITY_CUSTOM_X 			15
#define TPD_VELOCITY_CUSTOM_Y 			20

#define TPD_DELAY                		(2*HZ/100)
#define TPD_RES_X                		1080
#define TPD_RES_Y                		1920
#define TPD_CALIBRATION_MATRIX  		{962,0,0,0,1600,0,0,0};

//#define TPD_HAVE_CALIBRATION
//#define TPD_HAVE_TREMBLE_ELIMINATION

/******************************************************************************/
/*Chip Device Type*/
#define IC_FT5X06						0	/*x=2,3,4*/
#define IC_FT5606						1	/*ft5506/FT5606/FT5816*/
#define IC_FT5316						2	/*ft5x16*/
#define IC_FT6208						3  	/*ft6208*/
#define IC_FT6x06     					4	/*ft6206/FT6306*/
#define IC_FT5x06i     					5	/*ft5306i*/
#define IC_FT5x36     					6	/*ft5336/ft5436/FT5436i*/
#define IC_FT5x46     					7	/*ft5346/ft5446*/

/*register address*/
#define FT_REG_CHIP_ID				0xA3    //chip ID 
#define FT_REG_FW_VER				0xA6    //FW version 
#define FT_REG_VENDOR_ID			0xA8    //TP vendor ID 
#define FT_REG_LCM_ID			    0xAB    //lcm ID 

#define FT_APP_INFO_ADDR	        0xd7f8
#define FT_VENDOR_ID_ADDR	        0xd784
#define FT_LCM_ID_ADDR	            0xd786

/*max point*/
#define TPD_MAX_POINTS_2            2
#define TPD_MAX_POINTS_5            5
#define TPD_MAX_POINTS_10           10

#define FTS_MAX_TOUCH TPD_MAX_POINTS_10

/*calibration option*/
#define AUTO_CLB_NEED               1
#define AUTO_CLB_NONEED             0

/*debug fuction*/
#define TPD_SYSFS_DEBUG
#ifndef USER_BUILD_KERNEL
#define FTS_CTL_IIC
#define FTS_APK_DEBUG
#define FTS_DEBUG
#endif

/*tp power down when suspend*/
//#define TPD_CLOSE_POWER_IN_SLEEP

// include ft5x46 self test interface
#define FTS_MCAP_TEST

/*download*/
// #define TPD_AUTO_DOWNLOAD				// if need download CTP FW when POWER ON,pls enable this MACRO

/*apgrade*/
//#define TPD_AUTO_UPGRADE				// if need upgrade CTP FW when POWER ON,pls enable this MACRO
//#define TPD_HW_REST

/*proximity*/
//#define TPD_PROXIMITY					// if need the PS funtion,enable this MACRO

/*gesture*/
#define FTS_GESTURE                     // if need gesture funtion,enable this MARCO
#ifdef FTS_GESTURE
#undef TPD_CLOSE_POWER_IN_SLEEP
#define FTS_GESTURE_DBG                 // if define it,open gesture in suspend every time
//#define FTS_ESD_CHECKER		// enable esd checker

#define MZ_UNKNOWN	0
#define MZ_HAND_MODE	1
#define MZ_GESTURE_MODE	2
#define MZ_COVER_MODE	3
#define MZ_SLEEP_MODE	4

#define MZ_NO_ACTION	0
#define MZ_FAILED	1
#define MZ_SUCCESS	2

#define GESTURE_SWITCH_OPEN          0x50
#define GESTURE_SWITCH_CLOSE         0x51
#endif //FTS_GESTURE

/*glove*/
//#define FTS_GLOVE
#ifdef FTS_GLOVE
#define GLOVE_SWITCH_OPEN            0x53
#define GLOVE_SWITCH_CLOSE           0x54
#endif

#define MZ_HALL_MODE

/*trace*/
#define pr_fmt(fmt) "[FTS]" fmt

#ifdef FTS_DEBUG
#define FTS_DBG		pr_debug
#else
#define FTS_DBG(fmt, args...)				do{}while(0)
#endif

/*current chip information for upgrade*/
struct Upgrade_Info
{
	u8  CHIP_ID;
	u8  FTS_NAME[20];
	u8  TPD_MAX_POINTS;
	u8  AUTO_CLB;
	u16 delay_aa;		    /*delay of write FT_UPGRADE_AA */
	u16 delay_55;		    /*delay of write FT_UPGRADE_55 */
#ifdef TPD_AUTO_DOWNLOAD
	u8  download_id_1;	    /*download id 1 */
	u8  download_id_2;	    /*download id 2 */
#endif
	u8  upgrade_id_1;	    /*upgrade id 1 */
	u8  upgrade_id_2;	    /*upgrade id 2 */
	u16 delay_readid;	    /*delay of read id */
	u16 delay_earse_flash;  /*delay of earse flash*/
};

#endif /* TOUCHPANEL_H__ */
