#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/delay.h>
/* 92-phase in: start */
#include <linux/earlysuspend.h>
#include <linux/types.h>
/* 92-phase in: end */

#include <mach/mt_clkmgr.h>
#include <asm/io.h>

#include <mach/m4u.h>
#include <mach/mt_smi.h>
#include "smi_reg.h"
#include "smi_common.h"
#include "smi_info_util.h"

#define SMI_LOG_TAG "SMI"

/* SMI HW Configurtion: 82 */
#if defined(SMI_82)
#define LARB0_PORT_NUM 10
#define LARB1_PORT_NUM 7
#define LARB2_PORT_NUM 17
#define LARB3_PORT_NUM 0
#define LARB4_PORT_NUM 0
#define LARB5_PORT_NUM 0
char *smi_port_name[][SMI_PORT_NAME_MAX] = {
	{
	 "disp_ovl", "disp_rdma", "disp_wdma", "mm_cmdq", "mdp_rdma",
	 "mdp_wdma", "mdp_rot_y", "mdp_rot_u", "mdp_rot_v",
	 },
	{
	 "vdec_mc", "vdec_pp", "vdec_avc_mv", "vdec_pred_rd", "vdec_pred_wr",
	 "vdec_vld", "vdec_ppwrap",
	 },
	{
	 "cam_imgo", "cam_img2o", "cam_lsci", "cam_imgi", "cam_esfko", "cam_aao",
	 "jpgenc_rdma", "jpgenc_bsdma", "venc_rd_comv", "venc_sv_comv", "venc_rcpu",
	 "venc_rec_frm", "venc_ref_luma", "venc_ref_chroma", "venc_bsdma", "venc_cur_luma",
	 "venc_cur_chroma",
	 },
};

#define initSetting() initSetting82()
#define vpSetting() vpSetting82()
#define vrSetting() vrSetting82()
#define vencSetting() vrSetting82()

#elif defined(SMI_92)

/* SMI HW Configurtion: 82 */
#define LARB0_PORT_NUM 9
#define LARB1_PORT_NUM 7
#define LARB2_PORT_NUM 19
#define LARB3_PORT_NUM 0
#define LARB4_PORT_NUM 0
#define LARB5_PORT_NUM 4
char *smi_port_name[][20] = {
	{
	 "disp_ovl", "disp_rdma1", "disp_rdma", "disp_wdma", "mm_cmdq",
	 "mdp_rdma", "mdp_wdma", "mdp_rot_y", "mdp_rot_u", "mdp_rot_v",
	 },
	{
	 "vdec_mc", "vdec_pp", "vdec_avc_mv", "vdec_pred_rd", "vdec_pred_wr",
	 "vdec_vld", "vdec_ppwrap",
	 },
	{
	 "cam_imgo", "cam_img2o", "cam_lsci", "cam_imgi", "cam_esfko",
	 "cam_aao", "cam_lcei", "cam_lcso", "jpgenc_rdma", "jpgenc_bsdma",
	 "venc_rd_comv", "venc_sv_comv", "venc_rcpu", "venc_rec_frm", "venc_ref_luma",
	 "venc_ref_chroma", "venc_bsdma", "venc_cur_luma", "venc_cur_chroma",
	 },
	{NULL},
	{NULL},
	{"mjc_mvr", "mjc_mvw", "mjc_rdma", "mjc_wdma",
	 }
};

#define initSetting() initSetting92()
#define vpSetting() vpSetting92()
#define vrSetting() vrSetting92()
#define vencSetting() vencSetting92()

#endif

/* GPU limitation selection */
#define GPU_SMI_L1_LIMIT_0 (0)
#define GPU_SMI_L1_LIMIT_1 ((0x1 << 23) + (0x2<<18) + (0x6 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_2 ((0x1 << 23) + (0x2<<18) + (0x3 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_3 ((0x1 << 23) + (0x2<<18) + (0x2 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_4 ((0x1 << 23) + (0x1<<18) + (0x1 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)

#define GPU_SMI_L1_LIMIT_OPT_0 ((0x1 << 23) + (0x3<<18) + (0x9 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_OPT_1 ((0x1 << 23) + (0x3<<18) + (0x7 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_OPT_2 ((0x1 << 23) + (0x3<<18) + (0x6 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_OPT_3 ((0x1 << 23) + (0x3<<18) + (0x5 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)

#define GPU_SMI_L1_LIMIT_VP_GP_OPT ((0x1 << 23) + (0x2<<18) + (0x1 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_VP_PP_OPT ((0x1 << 23) + (0x2<<18) + (0x7 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)

#define GPU_SMI_L1_LIMIT_WFDVP_GP_OPT ((0x1 << 23) + (0x2<<18) + (0x1 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_WFDVP_PP_OPT ((0x1 << 23) + (0x2<<18) + (0x5 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)

#define GPU_SMI_L1_LIMIT_VR_GP_OPT ((0x1 << 23) + (0x2<<18) + (0x2 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)
#define GPU_SMI_L1_LIMIT_VR_PP_OPT ((0x1 << 23) + (0x2<<18) + (0x4 << 13) + (0x1 << 12) + (0x0 << 11) + 0x0)

#define SMIDBG(level, x...)            \
do {                        \
	if (smi_debug_level >= (level))    \
		SMIMSG(x);            \
} while (0)

typedef struct {
	spinlock_t SMI_lock;
	/* use int as the concurrency table instead of long */
	unsigned int pu4ConcurrencyTable[SMI_BWC_SCEN_CNT];	/* one bit represent one module */
} SMI_struct;

MTK_SMI_BWC_MM_INFO g_smi_bwc_mm_info = { 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0},
0, 0, 0, 1920 * 1080 * 4
};

static void smi_slow_motoin_control(int enable);
static int smi_larb_init(unsigned int larb, int force_init);


static void smi_slow_motoin_control(int enable)
{
#if defined(SMI_92)
	bool bool_enable = enable;

	DISP_HalfVsync(bool_enable);
#else
	SMIMSG("Doesn't support slow motion control in this platform");
#endif
}

static SMI_struct g_SMIInfo;

unsigned int gLarbBaseAddr[SMI_LARB_NR_MAX] = {
#if !defined(LARB0_PORT_NUM) || LARB0_PORT_NUM == 0
	SMI_ERROR_ADDR,
#else
	LARB0_BASE,
#endif

#if !defined(LARB1_PORT_NUM) || LARB1_PORT_NUM == 0
	SMI_ERROR_ADDR,
#else
	LARB1_BASE,
#endif

#if !defined(LARB2_PORT_NUM) || LARB2_PORT_NUM == 0
	SMI_ERROR_ADDR,
#else
	LARB2_BASE,
#endif

#if !defined(LARB3_PORT_NUM) || LARB3_PORT_NUM == 0
	SMI_ERROR_ADDR,
#else
	LARB3_BASE,
#endif

#if !defined(LARB4_PORT_NUM) || LARB4_PORT_NUM == 0
	SMI_ERROR_ADDR,
#else
	LARB4_BASE,
#endif

#if !defined(LARB5_PORT_NUM) || LARB5_PORT_NUM == 0
	SMI_ERROR_ADDR,
#else
	LARB5_BASE,
#endif
};

/* Port numbers infromration */
static const unsigned int larb_port_num[SMI_LARB_NR_MAX] = { LARB0_PORT_NUM,
	LARB1_PORT_NUM, LARB2_PORT_NUM, LARB3_PORT_NUM, LARB4_PORT_NUM, LARB5_PORT_NUM
};

/* Sapce for resgister backup */
#if defined(LARB0_PORT_NUM) && LARB0_PORT_NUM != 0
static unsigned short int larb0_port_backup[LARB0_PORT_NUM];
#endif

#if defined(LARB1_PORT_NUM) && LARB1_PORT_NUM != 0
static unsigned short int larb1_port_backup[LARB1_PORT_NUM];
#endif

#if defined(LARB2_PORT_NUM) && LARB2_PORT_NUM != 0
static unsigned short int larb2_port_backup[LARB2_PORT_NUM];
#endif

#if defined(LARB3_PORT_NUM) && LARB3_PORT_NUM != 0
static unsigned short int larb3_port_backup[LARB3_PORT_NUM];
#endif

#if defined(LARB4_PORT_NUM) && LARB4_PORT_NUM != 0
static unsigned short int larb4_port_backup[LARB4_PORT_NUM];
#endif

#if defined(LARB5_PORT_NUM) && LARB5_PORT_NUM != 0
static unsigned short int larb5_port_backup[LARB5_PORT_NUM];
#endif

/* Reference of bacup spaces */
static unsigned short int *larb_port_backup[SMI_LARB_NR_MAX] = {
#if !defined(LARB0_PORT_NUM) || LARB0_PORT_NUM == 0
	NULL,
#else
	larb0_port_backup,
#endif

#if !defined(LARB1_PORT_NUM) || LARB1_PORT_NUM == 0
	NULL,
#else
	larb1_port_backup,
#endif

#if !defined(LARB2_PORT_NUM) || LARB2_PORT_NUM == 0
	NULL,
#else
	larb2_port_backup,
#endif

#if !defined(LARB3_PORT_NUM) || LARB3_PORT_NUM == 0
	NULL,
#else
	larb3_port_backup,
#endif

#if !defined(LARB4_PORT_NUM) || LARB4_PORT_NUM == 0
	NULL,
#else
	larb4_port_backup,
#endif

#if !defined(LARB5_PORT_NUM) || LARB5_PORT_NUM == 0
	NULL,
#else
	larb5_port_backup,
#endif
};

/* For IPO init */
static int ipo_force_init;
/* To keep the HW's init value */
static int is_default_value_saved;

static unsigned int default_val_smi_l1arb[SMI_LARB_NR_MAX] = { 0 };

/* For user space debugging */
static unsigned int wifi_disp_transaction;

static unsigned int smi_debug_level;

static unsigned int smi_tuning_mode;

static unsigned int smi_profile = SMI_BWC_SCEN_NORMAL;

static void initSetting92(void);
static void vpSetting92(void);
static void vrSetting92(void);
static void initSetting82(void);
static void vpSetting82(void);
static void vrSetting82(void);

#if defined(SMI_92)
static void vencSetting92(void);
#endif				/*SMI_92 */

/* Register backup and restore utilties */
static void backup_larb_smi(int index)
{

	int port_index = 0;
	unsigned short int *backup_ptr = NULL;
	unsigned int larb_base = get_larb_base_addr(index);
	unsigned int larb_offset = 0x200;
	int total_port_num = 0;

	if (index < 0 || index >= SMI_LARB_NR)
		return;

	total_port_num = larb_port_num[index];
	backup_ptr = larb_port_backup[index];

	if (total_port_num <= 0 || backup_ptr == NULL)
		return;

	for (port_index = 0; port_index < total_port_num; port_index++) {
		*backup_ptr = (unsigned short int)(M4U_ReadReg32(larb_base, larb_offset));
		backup_ptr++;
		larb_offset += 4;
	}
	SMIDBG(1, "Backup smi larb[%d]: 0x%x - 0x%x", index, 0x200, (larb_offset - 4));
	return;
}

static void restore_larb_smi(int index)
{

	int port_index = 0;
	unsigned short int *backup_ptr = NULL;
	unsigned int larb_base = get_larb_base_addr(index);
	unsigned int larb_offset = 0x200;
	unsigned int backup_value = 0;
	int total_port_num = 0;

	if (index < 0 || index >= SMI_LARB_NR)
		return;

	total_port_num = larb_port_num[index];
	backup_ptr = larb_port_backup[index];

	if (total_port_num <= 0 || backup_ptr == NULL)
		return;


	for (port_index = 0; port_index < total_port_num; port_index++) {
		backup_value = *backup_ptr;
		M4U_WriteReg32(larb_base, larb_offset, backup_value);
		backup_ptr++;
		larb_offset += 4;
	}
	SMIDBG(1, "Restored smi larb[%d]: 0x%x - 0x%x", index, 0x200, (larb_offset - 4));
	return;
}

/* Use this function to get base address of Larb resgister
 to support error checking */

unsigned long get_larb_base_addr(int larb_id)
{
	if (larb_id > SMI_LARB_NR || larb_id < 0)
		return SMI_ERROR_ADDR;
	else
		return (unsigned long)gLarbBaseAddr[larb_id];

	return SMI_ERROR_ADDR;
}

int larb_clock_on(int larb_id)
{

#ifndef CONFIG_MTK_FPGA
	char name[30];

	sprintf(name, "smi+%d", larb_id);

	switch (larb_id) {
	case 0:
		enable_clock(MT_CG_DISP0_SMI_COMMON, name);
		enable_clock(MT_CG_DISP0_SMI_LARB0, name);
		break;
	case 1:
		enable_clock(MT_CG_DISP0_SMI_COMMON, name);
		enable_clock(MT_CG_VDEC1_LARB, name);
		break;
	case 2:
		enable_clock(MT_CG_DISP0_SMI_COMMON, name);
		enable_clock(MT_CG_IMAGE_LARB2_SMI, name);
		break;
#if defined(SMI_92)
	case 5:		/* Added MJC since MT6592 */
		enable_clock(MT_CG_DISP0_SMI_COMMON, name);
		enable_clock(MT_CG_MJC_SMI_LARB, name);
#endif				/* SMI_92 */
	default:
		break;
	}
#endif

	return 0;
}

int larb_clock_off(int larb_id)
{
#ifndef CONFIG_MTK_FPGA

	char name[30];

	sprintf(name, "smi+%d", larb_id);

	switch (larb_id) {
	case 0:
		disable_clock(MT_CG_DISP0_SMI_LARB0, name);
		disable_clock(MT_CG_DISP0_SMI_COMMON, name);
		break;
	case 1:
		disable_clock(MT_CG_VDEC1_LARB, name);
		disable_clock(MT_CG_DISP0_SMI_COMMON, name);
		break;
	case 2:
		disable_clock(MT_CG_IMAGE_LARB2_SMI, name);
		disable_clock(MT_CG_DISP0_SMI_COMMON, name);
		break;
#if defined(SMI_92)
	case 5:		/* Added MJC since MT6592 */
		disable_clock(MT_CG_MJC_SMI_LARB, name);
		disable_clock(MT_CG_DISP0_SMI_COMMON, name);
#endif				/* SMI_92 */
	default:
		break;
	}
#endif

	return 0;

}

#define LARB_BACKUP_REG_SIZE 128
static unsigned int *pLarbRegBackUp[SMI_LARB_NR];
static int g_bInited;

int larb_reg_backup(int larb)
{
#if defined(SMI_92)
	unsigned int *pReg = NULL;
	unsigned int larb_base = SMI_ERROR_ADDR;
	int i = 0;

	SMIDBG(1, "+larb_reg_backup(), larb_idx=%d\n", larb);

	larb_base = get_larb_base_addr(larb);
	pReg = pLarbRegBackUp[larb];

	/* If SMI can't find the corrosponded larb address, skip the backup for the larb */
	if (larb_base == SMI_ERROR_ADDR) {
		SMIMSG("Can't find the base address for Larb%d\n", larb);
		return 0;
	}
	SMIDBG(1, "m4u part backup, larb_idx=%d\n", larb);

	*(pReg++) = M4U_ReadReg32(larb_base, SMI_LARB_CON);
	*(pReg++) = M4U_ReadReg32(larb_base, SMI_SHARE_EN);

	for (i = 0; i < 3; i++) {
		*(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_START(i));
		*(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_END(i));
		*(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_GID(i));
	}
	/* Backup SMI related registers */
	SMIDBG(1, "+backup_larb_smi(), larb_idx=%d\n", larb);
	backup_larb_smi(larb);
	SMIDBG(1, "-backup_larb_smi(), larb_idx=%d\n", larb);
#endif				/* SMI_92 */
	if (0 == larb)
		g_bInited = 0;

	SMIDBG(1, "-larb_reg_backup(), larb_idx=%d\n", larb);
	return 0;
}

int larb_reg_restore(int larb, int force_init)
{
	unsigned int larb_base = SMI_ERROR_ADDR;
	unsigned int regval = 0;
	int i = 0;
	unsigned int *pReg = NULL;

	larb_base = get_larb_base_addr(larb);

	if (larb_base == SMI_ERROR_ADDR) {
		SMIMSG("Can't find the base address for Larb%d\n", larb);
		return 0;
	}

	pReg = pLarbRegBackUp[larb];

	SMIDBG(1, "+larb_reg_restore(), larb_idx=%d\n", larb);
	SMIDBG(1, "m4u part restore, larb_idx=%d\n", larb);
	regval = *(pReg++);
	M4U_WriteReg32(larb_base, SMI_LARB_CON_CLR, ~(regval));
	M4U_WriteReg32(larb_base, SMI_LARB_CON_SET, (regval));

	M4U_WriteReg32(larb_base, SMI_SHARE_EN, *(pReg++));

	for (i = 0; i < 3; i++) {
		M4U_WriteReg32(larb_base, SMI_MAU_ENTR_START(i), *(pReg++));
		M4U_WriteReg32(larb_base, SMI_MAU_ENTR_END(i), *(pReg++));
		M4U_WriteReg32(larb_base, SMI_MAU_ENTR_GID(i), *(pReg++));
	}
	SMIDBG(1, "+smi_larb_init(), larb_idx=%d\n", larb);
	smi_larb_init(larb, force_init);
	SMIDBG(1, "-smi_larb_init(), larb_idx=%d\n", larb);
	restore_larb_smi(larb);
	SMIDBG(1, "-larb_reg_restore(), larb_idx=%d\n", larb);
	return 0;
}

static int smi_larb_init(unsigned int larb, int force_init)
{

	unsigned int regval = 0;
	unsigned int regval1 = 0;
	unsigned int regval2 = 0;
	unsigned int larb_base = get_larb_base_addr(larb);

	/* Clock manager enable LARB clock before call back restore already, it will be
	   disabled after restore call back returns
	   Got to enable OSTD before engine starts */

	regval = M4U_ReadReg32(larb_base, SMI_LARB_STAT);
	regval1 = M4U_ReadReg32(larb_base, SMI_LARB_MON_BUS_REQ0);
	regval2 = M4U_ReadReg32(larb_base, SMI_LARB_MON_BUS_REQ1);

	if (0 == regval) {
		int retry_count = 0;

		SMIMSG("Init OSTD for larb_base: 0x%x\n", larb_base);
		/* Write 0x60 = 0xFFFF_FFFF, enable BW limiter */
		M4U_WriteReg32(larb_base, 0x60, 0xffffffff);
		/* Polling 0x600 = 0xaaaa */
		for (retry_count = 0; retry_count < 64; retry_count++) {
			if (M4U_ReadReg32(larb_base, 0x600) == 0xaaaa) {
				/* Step3.   Once it is found 0x600 == 0xaaaa, we can start
				to enable outstanding limiter and set outstanding limit */
				break;
			}
			SMIMSG("Larb: 0x%x busy : waiting for idle\n", larb_base);
			udelay(500);
		}

		/*  Write 0x60 = 0x0, disable BW limiter */
		M4U_WriteReg32(larb_base, 0x60, 0x0);
		/* enable ISTD */

		M4U_WriteReg32(larb_base, SMI_LARB_OSTD_CTRL_EN, 0xffffffff);
	} else {
		if (force_init == 1 || ipo_force_init == 1) {
			SMIMSG
			    ("Larb: 0x%x is busy : 0x%x , port:0x%x,0x%x , didn't set OSTD in force init mode\n",
			     larb_base, regval, regval1, regval2);
		} else {
			SMIMSG("Larb: 0x%x is busy : 0x%x , port:0x%x,0x%x ,fail to set OSTD\n",
			       larb_base, regval, regval1, regval2);
			smi_dumpDebugMsg();
			if (smi_debug_level >= 1) {
				SMIERR
				    ("DISP_MDP LARB  0x%x OSTD cannot be set:0x%x,port:0x%x,0x%x\n",
				     larb_base, regval, regval1, regval2);
			} else {
				dump_stack();
			}
		}
	}

	if (0 == g_bInited) {
		initSetting();
		g_bInited = 1;
		SMIMSG("SMI init\n");
	}

	return 0;
}

void on_larb_power_on(struct larb_monitor *h, int larb_idx)
{
	SMIDBG(1, "on_larb_power_on(), larb_idx=%d\n", larb_idx);
	SMIDBG(1, "+larb_reg_restore(), larb_idx=%d\n", larb_idx);
	larb_reg_restore(larb_idx, 0);

	SMIDBG(1, "-larb_reg_restore(), larb_idx=%d\n", larb_idx);

	return;
}

void on_larb_power_off(struct larb_monitor *h, int larb_idx)
{
	SMIDBG(1, "on_larb_power_off(), larb_idx=%d\n", larb_idx);
	SMIDBG(1, "+larb_reg_backup(), larb_idx=%d\n", larb_idx);
	larb_reg_backup(larb_idx);
	SMIDBG(1, "-larb_reg_backup(), larb_idx=%d\n", larb_idx);
}

/* DO NOT REMOVE the function, it is used by MHL driver */
/* Dynamic Adjustment for SMI profiles */
void smi_dynamic_adj_hint_mhl(int mhl_enable)
{
	SMIDBG(1, "MHL operation detected: %d", mhl_enable);
}

/* DO NOT REMOVE the function, it is used by MHL driver */
/* Dynamic Adjustment for SMI profiles */
void smi_dynamic_adj_hint(unsigned int dsi2smi_total_pixel)
{
	/* Since we have OVL decoupling now, we don't need to adjust the bandwidth limitation by frame */
	return;

}

/* Fake mode check, e.g. WFD */
static int fake_mode_handling(MTK_SMI_BWC_CONFIG *p_conf, unsigned int *pu4LocalCnt)
{
	if (p_conf->scenario == SMI_BWC_SCEN_WFD) {
		if (p_conf->b_on_off) {
			wifi_disp_transaction = 1;
			SMIMSG("Enable WFD in profile: %d\n", smi_profile);
		} else {
			wifi_disp_transaction = 0;
			SMIMSG("Disable WFD in profile: %d\n", smi_profile);
		}

		return 1;
	} else {
		return 0;
	}
}

static int smi_bwc_config(MTK_SMI_BWC_CONFIG *p_conf, unsigned int *pu4LocalCnt)
{
	int i;
	int result = 0;
	unsigned long u4Concurrency = 0;
	MTK_SMI_BWC_SCEN eFinalScen;
	static MTK_SMI_BWC_SCEN ePreviousFinalScen = SMI_BWC_SCEN_CNT;

	if (smi_tuning_mode == 1) {
		SMIMSG("Doesn't change profile in tunning mode");
		return 0;
	}

	spin_lock(&g_SMIInfo.SMI_lock);
	result = fake_mode_handling(p_conf, pu4LocalCnt);
	spin_unlock(&g_SMIInfo.SMI_lock);

	/* Fake mode is not a real SMI profile, so we need to return here */
	if (result == 1)
		return 0;

	if ((SMI_BWC_SCEN_CNT <= p_conf->scenario) || (0 > p_conf->scenario)) {
		SMIERR("Incorrect SMI BWC config : 0x%x, how could this be...\n", p_conf->scenario);
		return -1;
	}

	spin_lock(&g_SMIInfo.SMI_lock);

	if (p_conf->b_on_off) {
		g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario] += 1;

		if (NULL != pu4LocalCnt)
			pu4LocalCnt[p_conf->scenario] += 1;
	} else {
		/* turn off certain scenario */
		if (0 == g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario])
			SMIMSG("Too many turning off for global SMI profile:%d,%d\n",
			       p_conf->scenario, g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario]);
		else
			g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario] -= 1;

		if (NULL != pu4LocalCnt) {
			if (0 == pu4LocalCnt[p_conf->scenario])
				SMIMSG
				    ("Process : %s did too many turning off for local SMI profile:%d,%d\n",
				     current->comm, p_conf->scenario,
				     pu4LocalCnt[p_conf->scenario]);
			else
				pu4LocalCnt[p_conf->scenario] -= 1;
		}
	}

	for (i = 0; i < SMI_BWC_SCEN_CNT; i++) {
		if (g_SMIInfo.pu4ConcurrencyTable[i])
			u4Concurrency |= (1 << i);
	}

	if ((1 << SMI_BWC_SCEN_MM_GPU) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_MM_GPU;
	else if ((1 << SMI_BWC_SCEN_VR_SLOW) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VR_SLOW;
	else if ((1 << SMI_BWC_SCEN_VR) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VR;
	else if ((1 << SMI_BWC_SCEN_VP) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VP;
	else if ((1 << SMI_BWC_SCEN_SWDEC_VP) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_SWDEC_VP;
	else if ((1 << SMI_BWC_SCEN_VENC) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VENC;
	else
		eFinalScen = SMI_BWC_SCEN_NORMAL;


	if (ePreviousFinalScen == eFinalScen) {
		SMIMSG("Scen equal%d,don't change\n", eFinalScen);
		spin_unlock(&g_SMIInfo.SMI_lock);
		return 0;
	} else {
		ePreviousFinalScen = eFinalScen;
	}

	/*turn on larb clock */
	for (i = 0; i < SMI_LARB_NR; i++)
		larb_clock_on(i);

	/*Bandwidth Limiter */
	switch (eFinalScen) {
	case SMI_BWC_SCEN_VP:
		SMIMSG("[SMI_PROFILE] : %s\n", "SMI_BWC_VP");
		smi_profile = SMI_BWC_SCEN_VP;
		vpSetting();
		smi_slow_motoin_control(false);
		break;
	case SMI_BWC_SCEN_VR_SLOW:
		SMIMSG("[SMI_PROFILE] : %s\n", "SMI_BWC_VR");
		smi_profile = SMI_BWC_SCEN_VR_SLOW;
		vrSetting();
		smi_slow_motoin_control(true);
		break;
	case SMI_BWC_SCEN_SWDEC_VP:
		SMIMSG("[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_SWDEC_VP");
		smi_profile = SMI_BWC_SCEN_SWDEC_VP;
		vpSetting();
		smi_slow_motoin_control(false);
		break;
	case SMI_BWC_SCEN_VR:
		SMIMSG("[SMI_PROFILE] : %s\n", "SMI_BWC_VR");
		smi_profile = SMI_BWC_SCEN_VR;
		vrSetting();
		smi_slow_motoin_control(false);
		break;
	case SMI_BWC_SCEN_VENC:
		SMIMSG("[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_VENC");
		smi_profile = SMI_BWC_SCEN_VENC;
		vencSetting();
		smi_slow_motoin_control(false);
		break;
	case SMI_BWC_SCEN_NORMAL:
		SMIMSG("[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_NORMAL");
		smi_profile = SMI_BWC_SCEN_NORMAL;
		initSetting();
		smi_slow_motoin_control(false);
		break;
	case SMI_BWC_SCEN_MM_GPU:
		SMIMSG("[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_MM_GPU");
		smi_profile = SMI_BWC_SCEN_MM_GPU;
		initSetting();
		smi_slow_motoin_control(false);
	default:
		break;
	}

	/*turn off larb clock */
	for (i = 0; i < SMI_LARB_NR; i++)
		larb_clock_off(i);

	spin_unlock(&g_SMIInfo.SMI_lock);

	SMIMSG("ScenTo:%d,turn %s,Curr Scen:%d,%d,%d,%d\n", p_conf->scenario,
	       (p_conf->b_on_off ? "on" : "off"), eFinalScen,
	       g_SMIInfo.pu4ConcurrencyTable[SMI_BWC_SCEN_NORMAL],
	       g_SMIInfo.pu4ConcurrencyTable[SMI_BWC_SCEN_VR],
	       g_SMIInfo.pu4ConcurrencyTable[SMI_BWC_SCEN_VP]);

	return 0;

}

struct larb_monitor larb_monitor_handler = {.level = LARB_MONITOR_LEVEL_HIGH,
	.backup = on_larb_power_off, .restore = on_larb_power_on
};

int smi_common_init(void)
{
	int i;

	for (i = 0; i < SMI_LARB_NR; i++) {
		pLarbRegBackUp[i] =
		    kmalloc(LARB_BACKUP_REG_SIZE, GFP_KERNEL | __GFP_ZERO);
		if (pLarbRegBackUp[i] == NULL)
			SMIERR("pLarbRegBackUp kmalloc fail %d\n", i);
	}

	/** make sure all larb power is on before we register callback func.
	 then, when larb power is first off, default register value will be backed up.
	 **/

	for (i = 0; i < SMI_LARB_NR; i++)
		larb_clock_on(i);


#ifndef CONFIG_MTK_FPGA
	SMIMSG("Register SMI larb monitor handlers\n");
	register_larb_monitor(&larb_monitor_handler);
#endif

	for (i = 0; i < SMI_LARB_NR; i++)
		larb_clock_off(i);


	return 0;
}

static int smi_open(struct inode *inode, struct file *file)
{
	file->private_data = kmalloc_array(SMI_BWC_SCEN_CNT, sizeof(unsigned long), GFP_ATOMIC);

	if (NULL == file->private_data) {
		SMIMSG("Not enough entry for DDP open operation\n");
		return -ENOMEM;
	}

	memset(file->private_data, 0, SMI_BWC_SCEN_CNT * sizeof(unsigned long));

	return 0;
}

static int smi_release(struct inode *inode, struct file *file)
{

	if (NULL != file->private_data) {
		kfree(file->private_data);
		file->private_data = NULL;
	}

	return 0;
}

static long smi_ioctl(struct file *pFile, unsigned int cmd, unsigned long param)
{
	int ret = 0;

	/*    unsigned long * pu4Cnt = (unsigned long *)pFile->private_data; */

	switch (cmd) {
	case MTK_IOC_SMI_BWC_CONFIG:{
			MTK_SMI_BWC_CONFIG cfg;

			ret = copy_from_user(&cfg, (void *)param, sizeof(MTK_SMI_BWC_CONFIG));
			if (ret) {
				SMIMSG(" SMI_BWC_CONFIG, copy_from_user failed: %d\n", ret);
				return -EFAULT;
			}
			ret = smi_bwc_config(&cfg, NULL);

		}
		break;
	case MTK_IOC_SMI_BWC_INFO_SET:
		ret = smi_set_mm_info_ioctl_wrapper(pFile, cmd, param);
		break;
	case MTK_IOC_SMI_BWC_INFO_GET:
		ret = smi_get_mm_info_ioctl_wrapper(pFile, cmd, param);
		break;

	default:
		return -1;
	}

	return ret;
}

static const struct file_operations smiFops = {.owner = THIS_MODULE, .open =
	    smi_open, .release = smi_release, .unlocked_ioctl = smi_ioctl,
};

static struct cdev *pSmiDev;
static dev_t smiDevNo = MKDEV(MTK_SMI_MAJOR_NUMBER, 0);
static inline int smi_register(void)
{
	if (alloc_chrdev_region(&smiDevNo, 0, 1, "MTK_SMI")) {
		SMIERR("Allocate device No. failed");
		return -EAGAIN;
	}
	/* Allocate driver */
	pSmiDev = cdev_alloc();

	if (NULL == pSmiDev) {
		unregister_chrdev_region(smiDevNo, 1);
		SMIERR("Allocate mem for kobject failed");
		return -ENOMEM;
	}

	/* Attatch file operation. */
	cdev_init(pSmiDev, &smiFops);
	pSmiDev->owner = THIS_MODULE;

	/* Add to system */
	if (cdev_add(pSmiDev, smiDevNo, 1)) {
		SMIERR("Attatch file operation failed");
		unregister_chrdev_region(smiDevNo, 1);
		return -EAGAIN;
	}
	return 0;
}

static struct class *pSmiClass;
static int smi_probe(struct platform_device *pdev)
{
	struct device *smiDevice = NULL;

	if (NULL == pdev) {
		SMIERR("platform data missed");
		return -ENXIO;
	}

	if (smi_register()) {
		dev_err(&pdev->dev, "register char failed\n");
		return -EAGAIN;
	}

	pSmiClass = class_create(THIS_MODULE, "MTK_SMI");
	if (IS_ERR(pSmiClass)) {
		int ret = PTR_ERR(pSmiClass);

		SMIERR("Unable to create class, err = %d", ret);
		return ret;
	}
	smiDevice = device_create(pSmiClass, NULL, smiDevNo, NULL, "MTK_SMI");

	smi_common_init();

	return 0;
}

static int smi_remove(struct platform_device *pdev)
{
	cdev_del(pSmiDev);
	unregister_chrdev_region(smiDevNo, 1);
	device_destroy(pSmiClass, smiDevNo);
	class_destroy(pSmiClass);
	return 0;
}

static int smi_suspend(struct device *device)
{
	return 0;
}

static int smi_resume(struct device *device)
{
	return 0;
}

int SMI_common_pm_restore_noirq(struct device *device)
{

	SMIDBG(1, "SMI_common_pm_restore_noirq, do nothing\n");
	return 0;
}

const struct dev_pm_ops SMI_common_helper_pm_ops = {.suspend = smi_suspend, .resume =
	    smi_resume, .restore_noirq = SMI_common_pm_restore_noirq,
};

static struct platform_driver smiDrv = {.probe = smi_probe, .remove =
	    smi_remove, .driver = {.name = "MTK_SMI",
#ifdef CONFIG_PM
				  .pm = &SMI_common_helper_pm_ops,
#endif
				  .owner = THIS_MODULE,}
};

/* Earily suspend call backs */
#ifdef CONFIG_HAS_EARLYSUSPEND
static void SMI_common_early_suspend(struct early_suspend *h)
{
	SMIMSG("[%s]\n", __func__);
	return;
}

static void SMI_common_late_resume(struct early_suspend *h)
{
	/* This is a workaround for IPOH block issue. Somebody write REG_SMI_L1LEN
	   before SMI late_resume, need to fix it as soon as possible */
	SMIMSG("[%s] reinit REG_SMI_L1LEN reg.\n", __func__);
	M4U_WriteReg32(REG_SMI_L1LEN, 0, 0x1B);
	return;
}

static struct early_suspend mt_SMI_common_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	.suspend = SMI_common_early_suspend,
	.resume = SMI_common_late_resume,
};
#endif				/* CONFIG_HAS_EARLYSUSPEND */

static int __init smi_init(void)
{
	spin_lock_init(&g_SMIInfo.SMI_lock);

	memset(g_SMIInfo.pu4ConcurrencyTable, 0, SMI_BWC_SCEN_CNT * sizeof(unsigned long));

	if (platform_driver_register(&smiDrv)) {
		SMIERR("failed to register MAU driver");
		return -ENODEV;
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&mt_SMI_common_early_suspend_handler);
#endif

	return 0;
}

static void __exit smi_exit(void)
{
	platform_driver_unregister(&smiDrv);

}

void smi_dumpCommonDebugMsg(void)
{
	unsigned int u4Base;

	SMIMSG("===SMI common reg dump===\n");

	u4Base = SMI_COMMON_EXT_BASE;
	SMIMSG("[0x200,0x204,0x208]=[0x%x,0x%x,0x%x]\n",
	       M4U_ReadReg32(u4Base, 0x200), M4U_ReadReg32(u4Base, 0x204),
	       M4U_ReadReg32(u4Base, 0x208));
	SMIMSG("[0x20C,0x210,0x214]=[0x%x,0x%x,0x%x]\n",
	       M4U_ReadReg32(u4Base, 0x20C), M4U_ReadReg32(u4Base, 0x210),
	       M4U_ReadReg32(u4Base, 0x214));
	SMIMSG("[0x218,0x230,0x234]=[0x%x,0x%x,0x%x]\n",
	       M4U_ReadReg32(u4Base, 0x218), M4U_ReadReg32(u4Base, 0x230),
	       M4U_ReadReg32(u4Base, 0x234));
	SMIMSG("[0x400,0x404]=[0x%x,0x%x]\n", M4U_ReadReg32(u4Base, 0x400),
	       M4U_ReadReg32(u4Base, 0x404));

	/* For VA and PA check:
	   0x1000C5C0 , 0x1000C5C4, 0x1000C5C8, 0x1000C5CC, 0x1000C5D0 */
	u4Base = SMI_COMMON_AO_BASE;
	SMIMSG("===SMI always on reg dump===\n");
	SMIMSG("[0x5C0,0x5C4,0x5C8]=[0x%x,0x%x,0x%x]\n",
	       M4U_ReadReg32(u4Base, 0x5C0), M4U_ReadReg32(u4Base, 0x5C4),
	       M4U_ReadReg32(u4Base, 0x5C8));
	SMIMSG("[0x5CC,0x5D0]=[0x%x,0x%x]\n", M4U_ReadReg32(u4Base, 0x5CC),
	       M4U_ReadReg32(u4Base, 0x5D0));

}

void smi_dumpLarbDebugMsg(unsigned int u4Index)
{
	unsigned int u4Base;

	if (0 == u4Index) {
		if (0x3 & M4U_ReadReg32(0xF4000000, 0x100)) {
			SMIMSG("===SMI%d is off===\n", u4Index);
			return;
		}
	} else if (1 == u4Index) {
		if (0x1 & M4U_ReadReg32(0xF6000000, 0x4)) {
			SMIMSG("===SMI%d is off===\n", u4Index);
			return;
		}
	} else if (2 == u4Index) {
		if (0x1 & M4U_ReadReg32(0xF5000000, 0)) {
			SMIMSG("===SMI%d is off===\n", u4Index);
			return;
		}
	}
	u4Base = get_larb_base_addr(u4Index);

	if (u4Base == SMI_ERROR_ADDR) {
		SMIMSG("Doesn't support reg dump for Larb%d\n", u4Index);
		return;
	} else {
		SMIMSG("===SMI%d reg dump===\n", u4Index);

		SMIMSG("[0x0,0x10,0x60]=[0x%x,0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x0), M4U_ReadReg32(u4Base, 0x10),
		       M4U_ReadReg32(u4Base, 0x60));
		SMIMSG("[0x64,0x8c,0x450]=[0x%x,0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x64), M4U_ReadReg32(u4Base, 0x8c),
		       M4U_ReadReg32(u4Base, 0x450));
		SMIMSG("[0x454,0x600,0x604]=[0x%x,0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x454), M4U_ReadReg32(u4Base, 0x600),
		       M4U_ReadReg32(u4Base, 0x604));
		SMIMSG("[0x610,0x614]=[0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x610), M4U_ReadReg32(u4Base, 0x614));

		SMIMSG("[0x200,0x204,0x208]=[0x%x,0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x200), M4U_ReadReg32(u4Base, 0x204),
		       M4U_ReadReg32(u4Base, 0x208));
		SMIMSG("[0x20c,0x210,0x214]=[0x%x,0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x20c), M4U_ReadReg32(u4Base, 0x210),
		       M4U_ReadReg32(u4Base, 0x214));
		SMIMSG("[0x218,0x21c,0x220]=[0x%x,0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x218), M4U_ReadReg32(u4Base, 0x21c),
		       M4U_ReadReg32(u4Base, 0x220));
		SMIMSG("[0x224,0x228,0x22c]=[0x%x,0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x224), M4U_ReadReg32(u4Base, 0x228),
		       M4U_ReadReg32(u4Base, 0x22c));
		SMIMSG("[0x230,0x234,0x238]=[0x%x,0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x230), M4U_ReadReg32(u4Base, 0x234),
		       M4U_ReadReg32(u4Base, 0x238));
		SMIMSG("[0x23c,0x240]=[0x%x,0x%x]\n",
		       M4U_ReadReg32(u4Base, 0x23c), M4U_ReadReg32(u4Base, 0x240));
	}
}

void smi_dumpDebugMsg(void)
{
	unsigned int u4Index = 0;

	smi_dumpCommonDebugMsg();

	for (u4Index = 0; u4Index < SMI_LARB_NR; u4Index++)
		smi_dumpLarbDebugMsg(u4Index);

}

/* Check if MMSYS LARB is idle
 Return value:
 1: MMSYS larb is idle
 0: Bus is not idle  */
int is_smi_bus_idle(void)
{
	unsigned int reg_value = 0;
	int mmsys_larb_idle = 0;
	int smi_common_idle = 0;

	/* 1. SMI_LARB0_BASE + 0x0 [0] == 1b0     MMSYS LARB is idle */
	reg_value = M4U_ReadReg32(LARB0_BASE, 0x0);
	mmsys_larb_idle = reg_value & 0x0000001;

	/* 2.   SMI_COMMON_BASE + 0x404 [12] == 1b1           smi_common is idle */
	reg_value = 0;
	reg_value = M4U_ReadReg32(SMI_COMMON_EXT_BASE, 0x404);
	smi_common_idle = reg_value & (1 << 12);

	if ((mmsys_larb_idle == 0) && (smi_common_idle != 0))
		return 1;
	else
		return 0;
}

/* Check if specified MMSYS LARB is busy
 Return value:
 1: MMSYS larb is busy
 0: Bus is not busy */

int is_smi_larb_busy(unsigned int u4Index)
{
	unsigned int reg_value_0 = 0;
	unsigned int reg_value_1 = 0;

	unsigned int u4Base;

	u4Base = get_larb_base_addr(u4Index);

	if (u4Base == SMI_ERROR_ADDR) {
		SMIMSG("Doesn't support reg dump for Larb%d\n", u4Index);
		return 0;
	}

	/* 1.   0x0 == 1 */
	reg_value_0 = M4U_ReadReg32(u4Base, 0x0);
	if (0x0 != 1)
		return 0;

	/* 2.   (0x450 | 0x454) == 1 */
	reg_value_0 = M4U_ReadReg32(u4Base, 0x450);
	reg_value_1 = M4U_ReadReg32(u4Base, 0x454);
	if ((reg_value_0 | reg_value_1) != 1)
		return 0;

	/* 3.   0x600 == 0xaaaa */
	reg_value_0 = M4U_ReadReg32(u4Base, 0x600);
	if (reg_value_0 != 0xaaaa)
		return 0;

	/* 4.   0x604 == 0x2a8 */
	reg_value_0 = M4U_ReadReg32(u4Base, 0x604);
	if (reg_value_0 != 0x2a8)
		return 0;

	/* 5.   0x610 == 0x3e0 */
	reg_value_0 = M4U_ReadReg32(u4Base, 0x610);
	if (reg_value_0 != 0x3e0)
		return 0;

	/* 6.   0x614 == 0x3e0 */
	reg_value_0 = M4U_ReadReg32(u4Base, 0x614);
	if (reg_value_0 != 0x3e0)
		return 0;

	return 1;
}

module_init(smi_init);
module_exit(smi_exit);

module_param_named(tuning_mode, smi_tuning_mode, uint, S_IRUGO | S_IWUSR);

module_param_named(wifi_disp_transaction, wifi_disp_transaction, uint, S_IRUGO | S_IWUSR);

module_param_named(debug_level, smi_debug_level, uint, S_IRUGO | S_IWUSR);

module_param_named(smi_profile, smi_profile, uint, S_IRUSR);

/* Performance tunning for each chips */

static void initSetting82(void)
{
	M4U_WriteReg32(REG_SMI_M4U_TH, 0, ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));
	M4U_WriteReg32(REG_SMI_L1LEN, 0, 0xB);
	M4U_WriteReg32(REG_SMI_READ_FIFO_TH, 0, 0xC8F);

	M4U_WriteReg32(REG_SMI_L1ARB0, 0, 0);
	M4U_WriteReg32(REG_SMI_L1ARB1, 0, 0x9F7);
	M4U_WriteReg32(REG_SMI_L1ARB2, 0, 0x961);
	M4U_WriteReg32(REG_SMI_L1ARB3, 0, 0xA11FFF);

	M4U_WriteReg32(LARB0_BASE, 0x200, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x204, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x208, 0x3);
	M4U_WriteReg32(LARB0_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x210, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x218, 0x4);
	M4U_WriteReg32(LARB0_BASE, 0x21C, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x220, 0x2);

	M4U_WriteReg32(LARB1_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x218, 0x1);

	M4U_WriteReg32(LARB2_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x218, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x21C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x220, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x224, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x228, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x22C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x230, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x234, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x238, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x23C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x240, 0x1);
}

void vpSetting82(void)
{
	M4U_WriteReg32(REG_SMI_M4U_TH, 0, ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));
	M4U_WriteReg32(REG_SMI_L1LEN, 0, 0xB);
	M4U_WriteReg32(REG_SMI_READ_FIFO_TH, 0, 0xC8F);

	M4U_WriteReg32(REG_SMI_L1ARB0, 0, 0xC57);
	M4U_WriteReg32(REG_SMI_L1ARB1, 0, 0x9F7);
	M4U_WriteReg32(REG_SMI_L1ARB2, 0, 0x961);
	M4U_WriteReg32(REG_SMI_L1ARB3, 0, 0xA11FFF);

	M4U_WriteReg32(LARB0_BASE, 0x200, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x204, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x208, 0x3);
	M4U_WriteReg32(LARB0_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x210, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x218, 0x4);
	M4U_WriteReg32(LARB0_BASE, 0x21C, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x220, 0x2);

	M4U_WriteReg32(LARB1_BASE, 0x200, 0x6);
	M4U_WriteReg32(LARB1_BASE, 0x204, 0x2);
	M4U_WriteReg32(LARB1_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x20C, 0x3);
	M4U_WriteReg32(LARB1_BASE, 0x210, 0x3);
	M4U_WriteReg32(LARB1_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x218, 0x1);

	M4U_WriteReg32(LARB2_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x218, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x21C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x220, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x224, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x228, 0x4);
	M4U_WriteReg32(LARB2_BASE, 0x22C, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x230, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x234, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x238, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x23C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x240, 0x1);
}

void vrSetting82(void)
{
	M4U_WriteReg32(REG_SMI_M4U_TH, 0, ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));
	M4U_WriteReg32(REG_SMI_L1LEN, 0, 0xB);
	M4U_WriteReg32(REG_SMI_READ_FIFO_TH, 0, 0xC8F);

	M4U_WriteReg32(REG_SMI_L1ARB0, 0, 0xC57);
	M4U_WriteReg32(REG_SMI_L1ARB1, 0, 0x9F7);
	M4U_WriteReg32(REG_SMI_L1ARB2, 0, 0xD4F);
	M4U_WriteReg32(REG_SMI_L1ARB3, 0, 0xA11FFF);

	M4U_WriteReg32(LARB0_BASE, 0x200, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x204, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x210, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x214, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x218, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x21C, 0x4);
	M4U_WriteReg32(LARB0_BASE, 0x220, 0x1);

	M4U_WriteReg32(LARB1_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x218, 0x1);

	M4U_WriteReg32(LARB2_BASE, 0x200, 0x6);
	M4U_WriteReg32(LARB2_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x20C, 0x4);
	M4U_WriteReg32(LARB2_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x218, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x21C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x220, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x224, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x228, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x22C, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x230, 0x4);
	M4U_WriteReg32(LARB2_BASE, 0x234, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x238, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x23C, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x240, 0x1);
}

static void vrSetting92(void)
{
	M4U_WriteReg32(REG_SMI_M4U_TH, 0, ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));
	M4U_WriteReg32(REG_SMI_L1LEN, 0, 0xB);
	M4U_WriteReg32(REG_SMI_READ_FIFO_TH, 0, ((0x6 << 11) + (0x8 << 6) + 0x3F));

	M4U_WriteReg32(REG_SMI_L1ARB0, 0, 0xC26);
	M4U_WriteReg32(REG_SMI_L1ARB1, 0, 0x943);
	M4U_WriteReg32(REG_SMI_L1ARB2, 0, 0xD4F);

	M4U_WriteReg32(REG_SMI_L1ARB3, 0, GPU_SMI_L1_LIMIT_1);
	M4U_WriteReg32(REG_SMI_L1ARB4, 0, GPU_SMI_L1_LIMIT_1);
	M4U_WriteReg32(REG_SMI_L1ARB5, 0, 0xAA8);

	M4U_WriteReg32(LARB0_BASE, 0x200, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x204, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x208, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x214, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x218, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x21C, 0x4);
	M4U_WriteReg32(LARB0_BASE, 0x220, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x224, 0x2);

	M4U_WriteReg32(LARB1_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x218, 0x1);

	M4U_WriteReg32(LARB2_BASE, 0x200, 0x6);
	M4U_WriteReg32(LARB2_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x20C, 0x4);
	M4U_WriteReg32(LARB2_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x218, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x21C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x220, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x224, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x228, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x22C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x230, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x234, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x238, 0x4);
	M4U_WriteReg32(LARB2_BASE, 0x23C, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x244, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x248, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x24C, 0x1);

	M4U_WriteReg32(LARB5_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x20C, 0x1);
}

static void vpSetting92(void)
{
	M4U_WriteReg32(REG_SMI_M4U_TH, 0, ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));
	M4U_WriteReg32(REG_SMI_L1LEN, 0, 0x1B);
	M4U_WriteReg32(REG_SMI_READ_FIFO_TH, 0, 0x323F);

}

static void vencSetting92(void)
{
	M4U_WriteReg32(REG_SMI_M4U_TH, 0, ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));
	M4U_WriteReg32(REG_SMI_L1LEN, 0, 0xB);
	M4U_WriteReg32(REG_SMI_READ_FIFO_TH, 0, ((0x6 << 11) + (0x8 << 6) + 0x3F));

	M4U_WriteReg32(REG_SMI_L1ARB0, 0, 0xC26);
	M4U_WriteReg32(REG_SMI_L1ARB1, 0, 0x9E8);
	M4U_WriteReg32(REG_SMI_L1ARB2, 0, 0xD4F);

	M4U_WriteReg32(REG_SMI_L1ARB3, 0, default_val_smi_l1arb[3]);
	M4U_WriteReg32(REG_SMI_L1ARB4, 0, default_val_smi_l1arb[4]);
	M4U_WriteReg32(REG_SMI_L1ARB5, 0, 0xAA8);

	M4U_WriteReg32(LARB0_BASE, 0x200, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x204, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x208, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x218, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x21C, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x220, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x224, 0x1);

	M4U_WriteReg32(LARB1_BASE, 0x200, 0x3);
	M4U_WriteReg32(LARB1_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x218, 0x1);

	M4U_WriteReg32(LARB2_BASE, 0x200, 0x6);
	M4U_WriteReg32(LARB2_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x20C, 0x4);
	M4U_WriteReg32(LARB2_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x218, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x21C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x220, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x224, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x228, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x22C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x230, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x234, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x238, 0x2);
	M4U_WriteReg32(LARB2_BASE, 0x23C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x244, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x248, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x24C, 0x1);

	M4U_WriteReg32(LARB5_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x20C, 0x1);
}

static void initSetting92(void)
{
	SMIMSG("Current Setting: GPU - new");
	M4U_WriteReg32(REG_SMI_M4U_TH, 0, ((0x3 << 15) + (0x4 << 10) + (0x4 << 5) + 0x5));
	M4U_WriteReg32(REG_SMI_L1LEN, 0, 0xB);
	M4U_WriteReg32(REG_SMI_READ_FIFO_TH, 0, ((0x7 << 11) + (0x8 << 6) + 0x3F));

	if (!is_default_value_saved) {
		SMIMSG("Save default config:\n");
		default_val_smi_l1arb[0] = M4U_ReadReg32(REG_SMI_L1ARB0, 0);
		default_val_smi_l1arb[1] = M4U_ReadReg32(REG_SMI_L1ARB1, 0);
		default_val_smi_l1arb[2] = M4U_ReadReg32(REG_SMI_L1ARB2, 0);
		default_val_smi_l1arb[3] = M4U_ReadReg32(REG_SMI_L1ARB3, 0);
		default_val_smi_l1arb[4] = M4U_ReadReg32(REG_SMI_L1ARB4, 0);
		default_val_smi_l1arb[5] = M4U_ReadReg32(REG_SMI_L1ARB5, 0);
		SMIMSG("l1arb[0-2]= 0x%x,  0x%x, 0x%x\n",
		       default_val_smi_l1arb[0], default_val_smi_l1arb[1],
		       default_val_smi_l1arb[2]);
		SMIMSG("l1arb[3-5]= 0x%x,  0x%x, 0x%x\n",
		       default_val_smi_l1arb[3], default_val_smi_l1arb[4],
		       default_val_smi_l1arb[5]);

		is_default_value_saved = 1;
	}

	M4U_WriteReg32(REG_SMI_L1ARB0, 0, default_val_smi_l1arb[0]);
	M4U_WriteReg32(REG_SMI_L1ARB1, 0, default_val_smi_l1arb[1]);
	M4U_WriteReg32(REG_SMI_L1ARB2, 0, default_val_smi_l1arb[2]);
	M4U_WriteReg32(REG_SMI_L1ARB3, 0, default_val_smi_l1arb[3]);
	M4U_WriteReg32(REG_SMI_L1ARB4, 0, default_val_smi_l1arb[4]);
	M4U_WriteReg32(REG_SMI_L1ARB5, 0, default_val_smi_l1arb[5]);

	M4U_WriteReg32(LARB0_BASE, 0x200, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x204, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x208, 0x8);
	M4U_WriteReg32(LARB0_BASE, 0x20C, 0x2);
	M4U_WriteReg32(LARB0_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x214, 0x5);
	M4U_WriteReg32(LARB0_BASE, 0x218, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x21C, 0x3);
	M4U_WriteReg32(LARB0_BASE, 0x220, 0x1);
	M4U_WriteReg32(LARB0_BASE, 0x224, 0x1);

	M4U_WriteReg32(LARB1_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB1_BASE, 0x218, 0x1);

	M4U_WriteReg32(LARB2_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x20C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x210, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x214, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x218, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x21C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x220, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x224, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x228, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x22C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x230, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x234, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x238, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x23C, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x244, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x248, 0x1);
	M4U_WriteReg32(LARB2_BASE, 0x24C, 0x1);

	M4U_WriteReg32(LARB5_BASE, 0x200, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x204, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x208, 0x1);
	M4U_WriteReg32(LARB5_BASE, 0x20C, 0x1);

}

MODULE_DESCRIPTION("MTK SMI driver");
MODULE_AUTHOR("K_zhang<k.zhang@mediatek.com>");
MODULE_LICENSE("GPL");
