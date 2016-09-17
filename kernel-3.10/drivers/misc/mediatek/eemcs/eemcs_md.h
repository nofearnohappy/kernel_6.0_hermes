#ifndef __EEMCS_MD_H__
#define __EEMCS_MD_H__

#include "eemcs_kal.h"

#define NORMAL_BOOT_ID 0
#define META_BOOT_ID 1
#define UART_MAX_PORT_NUM 8


/* MD Message, this is for user space deamon use */
enum {
     CCCI_MD_MSG_BOOT_READY          = 0xFAF50001,
     CCCI_MD_MSG_BOOT_UP             = 0xFAF50002,
     CCCI_MD_MSG_EXCEPTION           = 0xFAF50003,
     CCCI_MD_MSG_RESET               = 0xFAF50004,
     CCCI_MD_MSG_RESET_RETRY         = 0xFAF50005,
     CCCI_MD_MSG_READY_TO_RESET      = 0xFAF50006,
     CCCI_MD_MSG_BOOT_TIMEOUT        = 0xFAF50007,
     CCCI_MD_MSG_STOP_MD_REQUEST     = 0xFAF50008,
     CCCI_MD_MSG_START_MD_REQUEST    = 0xFAF50009,
     CCCI_MD_MSG_ENTER_FLIGHT_MODE   = 0xFAF5000A,
     CCCI_MD_MSG_LEAVE_FLIGHT_MODE   = 0xFAF5000B,
     CCCI_MD_MSG_POWER_ON_REQUEST    = 0xFAF5000C,
     CCCI_MD_MSG_POWER_DOWN_REQUEST  = 0xFAF5000D,
     CCCI_MD_MSG_SEND_BATTERY_INFO   = 0xFAF5000E,
     CCCI_MD_MSG_NOTIFY              = 0xFAF5000F,
     CCCI_MD_MSG_STORE_NVRAM_MD_TYPE = 0xFAF50010,
};

typedef enum LOGGING_MODE_e {
    MODE_UNKNOWN = -1,      // -1
    MODE_IDLE,              // 0
    MODE_USB,               // 1
    MODE_SD,                // 2
    MODE_POLLING,           // 3
    MODE_WAITSD,            // 4
} LOGGING_MODE;

typedef struct RUNTIME_BUFF_st {
    KAL_UINT32 len;
    KAL_UINT8  buf[0];
} RUNTIME_BUFF;

typedef struct MODEM_RUNTIME_st {
    unsigned int Prefix;             // "CCIF"
    unsigned int Platform_L;         // Hardware Platform String ex: "TK6516E0"
    unsigned int Platform_H;
    unsigned int DriverVersion;      // 0x00000923 since W09.23
    unsigned int BootChannel;        // Channel to ACK AP with boot ready
    unsigned int BootingStartID;     // MD is booting. NORMAL_BOOT_ID or META_BOOT_ID 
    unsigned int BootAttributes;     // Attributes passing from AP to MD Booting
    unsigned int BootReadyID;        // MD response ID if boot successful and ready   
    unsigned int FileShareMemBase;
    unsigned int FileShareMemSize;
    unsigned int ExceShareMemBase;
    unsigned int ExceShareMemSize;   // 512 Bytes Required 
    unsigned int CCIFShareMemBase;
    unsigned int CCIFShareMemSize;
    unsigned int TotalShareMemBase;
    unsigned int TotalShareMemSize;
    unsigned int CheckSum;
    unsigned int Postfix;            //"CCIF" 
#if defined (_RUNTIME_MISC_INFO_SUPPORT_) // misc region
    unsigned int misc_prefix;	// "MISC"
    unsigned int support_mask;
    unsigned int index;
    unsigned int next;
    unsigned int feature_0_val[4];
    unsigned int feature_1_val[4];
    unsigned int feature_2_val[4];
    unsigned int feature_3_val[4];
    unsigned int feature_4_val[4];
    unsigned int feature_5_val[4];
    unsigned int feature_6_val[4];
    unsigned int feature_7_val[4];
    unsigned int feature_8_val[4];
    unsigned int feature_9_val[4];
    unsigned int feature_10_val[4];
    unsigned int feature_11_val[4];
    unsigned int feature_12_val[4];
    unsigned int feature_13_val[4];
    unsigned int feature_14_val[4];
    unsigned int feature_15_val[4];
    unsigned int reserved_2[3];
    unsigned int misc_postfix;	// "MISC"
#endif
} MODEM_RUNTIME;

typedef enum
{
    FEATURE_NOT_EXIST = 0,
    FEATURE_NOT_SUPPORT,
    FEATURE_SUPPORT,
    FEATURE_PARTIALLY_SUPPORT,
} MISC_FEATURE_STATE; 

typedef enum
{
    MISC_DMA_ADDR = 0,
    MISC_32K_LESS,
    MISC_RAND_SEED,
    MISC_MD_COCLK_SETTING,
    MISC_MD_SBP_SETTING,
    MISC_MD_CCCI_DEBUG,
} MISC_FEATURE_ID;

typedef enum
{
    CCCI_DBG_ADD_CCCI_SEQNO = 0,
    CCCI_DBG_POLL_MD_STA,
} CCCI_DEBUG_FEATURE_ID;

KAL_UINT32 eemcs_md_gen_runtime_data(void **data);
void eemcs_md_destroy_runtime_data(void *runtime_data);

#endif // __EEMCS_MD_H__
