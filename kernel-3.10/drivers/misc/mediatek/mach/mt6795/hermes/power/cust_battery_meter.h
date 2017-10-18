#ifndef _CUST_BATTERY_METER_H
#define _CUST_BATTERY_METER_H

#include <mach/mt_typedefs.h>

/* ============================================================ */
/* define */
/* ============================================================ */
/* #define SOC_BY_AUXADC */			//not used
#define SOC_BY_HW_FG				//checked
#define HW_FG_FORCE_USE_SW_OCV 		//checked
/* #define SOC_BY_SW_FG */			//not used

/* #define CONFIG_DIS_CHECK_BATTERY */ 	//not used
/* #define FIXED_TBAT_25 */				//not used

/* ADC resistor  */
// #define R_BAT_SENSE 4	//not used
// #define R_I_SENSE 4		//not used
// #define R_CHARGER_1 330	//not used
// #define R_CHARGER_2 39	//not used

#define TEMPERATURE_T0             110 	//checked
#define TEMPERATURE_T1             0	//checked
#define TEMPERATURE_T2             25	//checked
#define TEMPERATURE_T3             50	//checked
#define TEMPERATURE_T              255	//checked

#define FG_METER_RESISTANCE	0		//checked

/* Qmax for battery  */
#define Q_MAX_POS_50			3152 //2628	//checked
#define Q_MAX_POS_25			3181 //2660	//checked
#define Q_MAX_POS_0			3107 //2544	//checked
#define Q_MAX_NEG_10			74 //2404	//checked

#define Q_MAX_POS_50_H_CURRENT		3117 //2696 //checked
#define Q_MAX_POS_25_H_CURRENT		2524 //2706	//checked
#define Q_MAX_POS_0_H_CURRENT		2202 //2430	//checked
#define Q_MAX_NEG_10_H_CURRENT		322 //1763	//checked


/* Discharge Percentage */
#define OAM_D5		 1	/* 1 : D5,   0: D2 */	//not used 


/* battery meter parameter */
#define CHANGE_TRACKING_POINT		//checked
#define CUST_TRACKING_POINT		0
#define CUST_R_SENSE         68		//checked
// #define CUST_HW_CC		    0	//not used
#define AGING_TUNING_VALUE	100
#define CUST_R_FG_OFFSET    0		//checked

#define OCV_BOARD_COMPESATE	0	/* mV */	//checked
#define R_FG_BOARD_BASE		1000
#define R_FG_BOARD_SLOPE	1000	/* slope */
#define CAR_TUNE_VALUE		100	/* 1.00 */


/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG	10	/* 1mA */ 				//checked
#define MinErrorOffset       1000
#define FG_VBAT_AVERAGE_SIZE 18							//checked
#define R_FG_VALUE			10	/* mOhm, base is 20 */ 	//checked

/* fg 2.0 */
// #define DIFFERENCE_HWOCV_RTC		30		//not used
// #define DIFFERENCE_HWOCV_SWOCV		10	//not used
// #define DIFFERENCE_SWOCV_RTC		10		//not used
// #define MAX_SWOCV			3			//not used

// #define DIFFERENCE_VOLTAGE_UPDATE	20	//not used
// #define AGING1_LOAD_SOC			70		//not used
// #define AGING1_UPDATE_SOC		30		//not used
// #define BATTERYPSEUDO100		95			//not used
// #define BATTERYPSEUDO1			4 		//not used

//#define Q_MAX_BY_SYS		/* 8. Qmax varient by system drop voltage. */ //not used
//#define Q_MAX_SYS_VOLTAGE		3400 		//not used
// #define SHUTDOWN_GAUGE0					//not used
// #define SHUTDOWN_GAUGE1_XMINS			//not used
// #define SHUTDOWN_GAUGE1_MINS		60		//not used

#define SHUTDOWN_SYSTEM_VOLTAGE		3400 	//checked
// #define CHARGE_TRACKING_TIME		60		//not used
// #define DISCHARGE_TRACKING_TIME		10	//not used

// #define RECHARGE_TOLERANCE		10		//not used
/* SW Fuel Gauge */
// #define MAX_HWOCV			5			//not used
// #define MAX_VBAT			90				//not used
// #define DIFFERENCE_HWOCV_VBAT		30	//not used

/* fg 1.0 */
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	40 	//checked
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE		5
#define CUST_POWERON_MAX_VBAT_TOLRANCE			90	//checked
#define CUST_POWERON_DELTA_VBAT_TOLRANCE		30 	//checked

/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP		3600	/* 3.6V */ 				//checked
#define VBAT_LOW_POWER_WAKEUP		3500	/* 3.5v */ 			//checked
#define NORMAL_WAKEUP_PERIOD		5400	/* 90 * 60 = 90 min */
#define LOW_POWER_WAKEUP_PERIOD		300	/* 5 * 60 = 5 min */	//checked
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30	/* 30 s */			//checked

#define INIT_SOC_BY_SW_SOC				//checked
/* #define SYNC_UI_SOC_IMM              //not used  //3. UI SOC sync to FG SOC immediately */
#define MTK_ENABLE_AGING_ALGORITHM		//checked 	/* 6. Q_MAX aging algorithm */
// #define MD_SLEEP_CURRENT_CHECK		//not used 	/* 5. Gauge Adjust by OCV 9. MD sleep current check */
/*#define Q_MAX_BY_CURRENT*/ 			//not used	/* 7. Qmax varient by current loading. */

#endif				/* #ifndef _CUST_BATTERY_METER_H */
