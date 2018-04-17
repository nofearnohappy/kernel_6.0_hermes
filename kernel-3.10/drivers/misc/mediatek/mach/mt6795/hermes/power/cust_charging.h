#ifndef _CUST_BAT_H_
#define _CUST_BAT_H_

/* stop charging while in talking mode */
#define STOP_CHARGING_IN_TAKLING
#define TALKING_RECHARGE_VOLTAGE 3800   //checked
#define TALKING_SYNC_TIME		   60	//checked

/* Battery Temperature Protection */
#define MTK_TEMPERATURE_RECHARGE_SUPPORT 			//checked
#define MAX_CHARGE_TEMPERATURE  54					//checked
#define MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE	47	//checked
#define MIN_CHARGE_TEMPERATURE  1					//checked
#define MIN_CHARGE_TEMPERATURE_PLUS_X_DEGREE	3	//checked
#define ERR_CHARGE_TEMPERATURE  0xFF				//checked

/* Linear Charging Threshold */
#define V_PRE2CC_THRES	 		3400	//mV 	//checked
#define V_CC2TOPOFF_THRES		4050			
// #define RECHARGING_VOLTAGE      4110			//not used
// #define CHARGING_FULL_CURRENT    150	//mA 	//not used

/* Charging Current Setting */
//#define CONFIG_USB_IF 						   
// #define USB_CHARGER_CURRENT_SUSPEND			0						//not used  // def CONFIG_USB_IF
// #define USB_CHARGER_CURRENT_UNCONFIGURED	CHARGE_CURRENT_70_00_MA		//not used	// 70mA
// #define USB_CHARGER_CURRENT_CONFIGURED	CHARGE_CURRENT_500_00_MA	//not used 	// 500mA

#define USB_CHARGER_CURRENT					CHARGE_CURRENT_550_00_MA	//checked
#define AC_CHARGER_CURRENT					CHARGE_CURRENT_1600_00_MA   //checked

#define NON_STD_AC_CHARGER_CURRENT			CHARGE_CURRENT_500_00_MA 	//checked
#define CHARGING_HOST_CHARGER_CURRENT       CHARGE_CURRENT_650_00_MA 	//checked
#define APPLE_0_5A_CHARGER_CURRENT          CHARGE_CURRENT_500_00_MA 	//checked
#define APPLE_1_0A_CHARGER_CURRENT          CHARGE_CURRENT_650_00_MA	//checked
#define APPLE_2_1A_CHARGER_CURRENT          CHARGE_CURRENT_800_00_MA 	//checked


/* Precise Tunning */
#define BATTERY_AVERAGE_DATA_NUMBER	3	//checked
#define BATTERY_AVERAGE_SIZE 	30		//checked

/* charger error check */
#define BAT_LOW_TEMP_PROTECT_ENABLE     //checked   // stop charging if temp < MIN_CHARGE_TEMPERATURE
#define V_CHARGER_ENABLE 0				//checked 	// 1:ON , 0:OFF	
#define V_CHARGER_MAX 6500				//checked 	// 6.5 V
// #define V_CHARGER_MIN 4400			//not used	// 4.4 V

/* Tracking TIME */
#define ONEHUNDRED_PERCENT_TRACKING_TIME	10					// 10 second
#define NPERCENT_TRACKING_TIME	   			20					// 20 second
#define SYNC_TO_REAL_TRACKING_TIME  		60				 	// 60 second
#define V_0PERCENT_TRACKING					3450 	//checked  	//3450mV

/* Battery Notify */
#define BATTERY_NOTIFY_CASE_0001_VCHARGER			//checked
#define BATTERY_NOTIFY_CASE_0002_VBATTEMP			//checked
//#define BATTERY_NOTIFY_CASE_0003_ICHARGING		//not used
//#define BATTERY_NOTIFY_CASE_0004_VBAT				//not used
//#define BATTERY_NOTIFY_CASE_0005_TOTAL_CHARGINGTIME 	//not used

/* High battery support */
#define HIGH_BATTERY_VOLTAGE_SUPPORT	//checked

/* JEITA parameter */
//#define MTK_JEITA_STANDARD_SUPPORT					//not used
// #define CUST_SOC_JEITA_SYNC_TIME 30					//not used
// #define JEITA_RECHARGE_VOLTAGE  4110					//not used 		// for linear charging
// #ifdef HIGH_BATTERY_VOLTAGE_SUPPORT
// #define JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_240000_V		//not used
// #define JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_240000_V	//not used
// #define JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE		BATTERY_VOLT_04_340000_V	//not used
// #define JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE		BATTERY_VOLT_04_240000_V	//not used
// #define JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE		BATTERY_VOLT_04_040000_V	//not used
// #define JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE		BATTERY_VOLT_04_040000_V		//not used
// #else
// #define JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_100000_V		//not used
// #define JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE	BATTERY_VOLT_04_100000_V		//not used
// #define JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE	BATTERY_VOLT_04_200000_V		//not used
// #define JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE	BATTERY_VOLT_04_100000_V		//not used
// #define JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE	BATTERY_VOLT_03_900000_V		//not used
// #define JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE		BATTERY_VOLT_03_900000_V		//not used
// #endif
// /* For JEITA Linear Charging only */
// #define JEITA_NEG_10_TO_POS_0_FULL_CURRENT  120	//mA 			//not used
// #define JEITA_TEMP_POS_45_TO_POS_60_RECHARGE_VOLTAGE  4000		//not used
// #define JEITA_TEMP_POS_10_TO_POS_45_RECHARGE_VOLTAGE  4100		//not used
// #define JEITA_TEMP_POS_0_TO_POS_10_RECHARGE_VOLTAGE   4000		//not used
// #define JEITA_TEMP_NEG_10_TO_POS_0_RECHARGE_VOLTAGE   3800		//not used
// #define JEITA_TEMP_POS_45_TO_POS_60_CC2TOPOFF_THRESHOLD	4050	//not used
// #define JEITA_TEMP_POS_10_TO_POS_45_CC2TOPOFF_THRESHOLD	4050	//not used
// #define JEITA_TEMP_POS_0_TO_POS_10_CC2TOPOFF_THRESHOLD	4050	//not used
// #define JEITA_TEMP_NEG_10_TO_POS_0_CC2TOPOFF_THRESHOLD	3850	//not used

/* Disable Battery check for HQA */
// #ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION 	//not used
// #define CONFIG_DIS_CHECK_BATTERY								//not used
// #endif														//not used

// #ifdef CONFIG_MTK_FAN5405_SUPPORT		//not used
// #define FAN5405_BUSNUM 1 				//not used
// #endif									//not used

/* Pump Express support (fast charging) */
// #ifdef CONFIG_MTK_PUMP_EXPRESS_SUPPORT		//not used
// #define TA_START_VCHR_TUNUNG_VOLTAGE	3700	//not used  // for isink blink issue
// #define TA_CHARGING_CURRENT			    	//not used   CHARGE_CURRENT_1500_00_MA
// #endif

#endif /* _CUST_BAT_H_ */ 
