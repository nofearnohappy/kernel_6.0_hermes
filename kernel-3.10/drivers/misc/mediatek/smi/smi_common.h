#ifndef __SMI_COMMON_H__
#define __SMI_COMMON_H__

#undef pr_fmt
#define pr_fmt(fmt) "[SMI]" fmt

#include <linux/aee.h>
#ifdef CONFIG_MTK_CMDQ
#include "cmdq_core.h"
#endif

#define SMIMSG(string, args...) pr_warn("[pid=%d]"string, current->tgid, ##args)
#define SMIMSG2(string, args...) pr_warn(string, ##args)

#ifdef CONFIG_MTK_CMDQ
#define SMIMSG3(onoff, string, args...) {\
if ((onoff) == 1) \
	cmdq_core_save_first_dump(string, ##args);\
	SMIMSG(string, ##args);\
}
#else
#define SMIMSG3(string, args...) SMIMSG(string, ##args)
#endif


#define SMITMP(string, args...) pr_warn("[pid=%d]"string, current->tgid, ##args)
#define SMIERR(string, args...) do {\
	pr_err("error: "string, ##args); \
	aee_kernel_warning(SMI_LOG_TAG, "error: "string, ##args);  \
} while (0)

#define smi_aee_print(string, args...) do {\
	char smi_name[100];\
	snprintf(smi_name, 100, "["SMI_LOG_TAG"]"string, ##args);\
	aee_kernel_warning(smi_name, "["SMI_LOG_TAG"]error:"string, ##args);\
} while (0)

#define SMI_PORT_NAME_MAX 20
#define SMI_LARB_NR_MAX 6
#define SMI_ERROR_ADDR  0


#if defined(SMI_K2) || defined(SMI_RO)
#define SMI_LARB_NR     5

#define SMI_LARB0_PORT_NUM  14
#define SMI_LARB1_PORT_NUM  9
#define SMI_LARB2_PORT_NUM  21
#define SMI_LARB3_PORT_NUM  19
#define SMI_LARB4_PORT_NUM  4


typedef struct {
	spinlock_t SMI_lock;
	unsigned int pu4ConcurrencyTable[SMI_BWC_SCEN_CNT];	/* one bit represent one module */
} SMI_struct;

/* for slow motion force 30 fps */
extern int primary_display_force_set_vsync_fps(unsigned int fps);
extern unsigned int primary_display_get_fps(void);

extern void register_base_dump(void);
#elif defined(SMI_92)
#define MAU_ENTRY_NR    3
#define SMI_LARB_NR     6
/* Please use the function to instead gLarbBaseAddr to prevent the NULL pointer access error */
/* when the corrosponding larb is not exist */
/* extern unsigned int gLarbBaseAddr[SMI_LARB_NR]; */
extern char *smi_port_name[][SMI_PORT_NAME_MAX];
extern void DISP_HalfVsync(bool enable);

#elif defined(SMI_82)
#define MAU_ENTRY_NR    3
#define SMI_LARB_NR     3
extern char *smi_port_name[][SMI_PORT_NAME_MAX];
/* Only extern the gLarbBaseAddr in 82 */
extern unsigned int gLarbBaseAddr[SMI_LARB_NR_MAX];
#endif

#if defined(SMI_K2)
#define SMI_CLIENT_DISP 0
#define SMI_CLIENT_WFD 1

#define SMI_EVENT_DIRECT_LINK  (0x1 << 0)
#define SMI_EVENT_DECOUPLE     (0x1 << 1)
#define SMI_EVENT_OVL_CASCADE  (0x1 << 2)
#define SMI_EVENT_OVL1_EXTERNAL  (0x1 << 3)

void register_base_dump(void);
extern void smi_client_status_change_notify(int module, int mode);
/* module: */
/* 0: DISP */
/* 1: WFD */
/* mode: */
/* DISP: */
/* SMI_EVENT_DIRECT_LINK - directlink mode */
/* SMI_EVENT_DECOUPLE - decouple mode */
/* SMI_EVENT_OVL_CASCADE - OVL cascade */
/* SMI_EVENT_OVL1_EXTERNAL - OVL 1 for external display */
#endif
extern void SMI_DBG_Init(void);

/* output_gce_buffer = 1, pass log to CMDQ error dumping messages */
extern int smi_debug_bus_hanging_detect_ext(unsigned int larbs, int show_dump,
					    int output_gce_buffer);
extern void smi_dumpDebugMsg(void);
extern void smi_hanging_debug(int dump_count);
extern int is_smi_bus_idle(void);
extern int is_smi_larb_busy(unsigned int u4Index);
int larb_clock_on(int larb_id);
int larb_clock_off(int larb_id);
extern unsigned long get_larb_base_addr(int larb_id);
void smi_dumpDebugMsg(void);
#endif
