


/**
 * @file    mt_ptp.c
 * @brief   Driver for PTP
 *
 */

#define __MT_PTP_C__

/*=============================================================
 * Include files
 *=============================================================*/

/* system includes */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/freezer.h>
/* project includes */
#include "mach/mt_reg_base.h"
#include "mach/mt_typedefs.h"

#include "mach/irqs.h"
#include "mach/mt_irq.h"
#include "mach/mt_ptp.h"
#include "mach/mt_cpufreq.h"
#include "mach/mt_gpufreq.h"
#include "mach/mt_thermal.h"
#include "mach/mt_spm_idle.h"
#include "mach/mt_pmic_wrap.h"
#include "mach/mt_clkmgr.h"
#include "mt_freqhopping.h"
#include "mach/mtk_rtc_hal.h"
#include "mach/mt_rtc_hw.h"
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#endif
/* local includes */
#include <mach/mt_ptp.h>
#include <linux/aee.h>

#ifndef __KERNEL__
#include "kernel2ctp.h"
#include "ptp.h"
#endif

/* Global variable for slow idle*/
volatile unsigned int ptp_data[3] = {0, 0, 0};

extern u32 get_devinfo_with_index(u32 index); // TODO: FIXME #include "devinfo.h"

struct ptp_det;
struct ptp_ctrl;

static void ptp_set_ptp_volt(struct ptp_det *det);
static void ptp_restore_ptp_volt(struct ptp_det *det);

#if 0
static void ptp_init01_prepare(struct ptp_det *det);
static void ptp_init01_finish(struct ptp_det *det);
#endif

/*=============================================================
 * Macro definition
 *=============================================================*/

/*
 * CONFIG (SW releated)
 */
#define CONFIG_PTP_SHOWLOG 1

#define EN_ISR_LOG		(0)

#define PTP_GET_REAL_VAL	(1) /* get val from efuse */
#define SET_PMIC_VOLT		(1) /* apply PMIC voltage */

#define DUMP_DATA_TO_DE      (0)

#define LOG_INTERVAL		(2LL * NSEC_PER_SEC)
#define NR_FREQ			8

/*
 * 100 us, This is the PTP Detector sampling time as represented in
 * cycles of bclk_ck during INIT. 52 MHz
 */
#define DETWINDOW_VAL		0xa28

/*
 *ROME E3
 * mili Volt to config value. voltage = 600mV + val * 6.25mV
 * val = (voltage - 600) / 6.25
 * @mV:	mili volt
 */
#if 1
//from Brian Yang,1mV=>10uV
#define PTP_VOLT_TO_PMIC_VAL(volt)  (((volt) - 70000 + 625 - 1) / 625) //((((volt) - 700 * 100 + 625 - 1) / 625)

//pmic value from ptpod already add 16 steps(16*6.25=100mV) for real voltage transfer
#define PTP_PMIC_VAL_TO_VOLT(pmic)  (((pmic) * 625) + 70000) //(((pmic) * 625) / 100 + 600),
#else
#define MV_TO_VAL(MV)		((((MV) - 600) * 100 + 625 - 1) / 625) // TODO: FIXME, refer to VOLT_TO_PMIC_VAL()
#define VAL_TO_MV(VAL)		(((VAL) * 625) / 100 + 600) // TODO: FIXME, refer to PMIC_VAL_TO_VOLT()
#endif
//offset 0x10(16 steps) for CPU/GPU DVFS
#define PTPOD_PMIC_OFFSET (0x10)


#define VMAX_VAL		PTP_VOLT_TO_PMIC_VAL(112500) //ROME+
#define VMIN_VAL		PTP_VOLT_TO_PMIC_VAL(80000)  //ROME+
#define VMIN_VAL_GPU	PTP_VOLT_TO_PMIC_VAL(93000) //ROME+

//CA53
#define DTHI_VAL		0x01		/* positive */
#define DTLO_VAL		0xfe		/* negative (2's compliment) */
#define DETMAX_VAL		0xffff		/* This timeout value is in cycles of bclk_ck. */
#define AGECONFIG_VAL		0x555555	/* FIXME */
#define AGEM_VAL		0x0		/* FIXME */
#define DVTFIXED_VAL		0x6		/* FIXME */
#define VCO_VAL			0x10		/* FIXME */
#define DCCONFIG_VAL		0x555555	/* FIXME */
//GPU
#define VCO_VAL_GPU			0x25		/* FIXME */
#define DVTFIXED_VAL_GPU		0x4		/* FIXME */
//LTE
#define VCO_VAL_LTE			0x20		/* FIXME */
#define DVTFIXED_VAL_LTE		0x4		/* FIXME */
//SOC
#define VCO_VAL_SOC			0x25		/* FIXME */
#define DVTFIXED_VAL_SOC		0x4		/* FIXME */

#ifdef CONFIG_MTK_RAM_CONSOLE
#define CONFIG_PTP_AEE_RR_REC 1
#endif

#ifdef CONFIG_PTP_AEE_RR_REC
enum ptp_state {
	PTP_CPU_LITTLE_IS_SET_VOLT = 0, //CPU
	PTP_CPU_BIG_IS_SET_VOLT,		//LTE
	PTP_GPU_IS_SET_VOLT,
};

extern void aee_rr_rec_ptp_cpu_big_volt(u64 val);
extern void aee_rr_rec_ptp_cpu_little_volt(u64 val);
extern void aee_rr_rec_ptp_gpu_volt(u64 val);
extern void aee_rr_rec_ptp_temp(u64 val);
extern void aee_rr_rec_ptp_status(u8 val);

extern u64 aee_rr_curr_ptp_cpu_big_volt(void);
extern u64 aee_rr_curr_ptp_cpu_little_volt(void);
extern u64 aee_rr_curr_ptp_gpu_volt(void);
extern u64 aee_rr_curr_ptp_temp(void);
extern u8 aee_rr_curr_ptp_status(void);

static void _mt_ptp_aee_init(void)
{
    aee_rr_rec_ptp_cpu_big_volt(0xFFFFFFFFFFFFFFFF);
    aee_rr_rec_ptp_cpu_little_volt(0xFFFFFFFFFFFFFFFF);
    aee_rr_rec_ptp_gpu_volt(0xFFFFFFFFFFFFFFFF);
    aee_rr_rec_ptp_temp(0xFFFFFFFFFFFFFFFF);
    aee_rr_rec_ptp_status(0xFF);
}
#endif
/*
 * bit operation
 */
#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r:	Range in the form of MSB:LSB
 */
#define BITMASK(r)	\
	(((unsigned) -1 >> (31 - MSB(r))) & ~((1U << LSB(r)) - 1))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

/*
 * LOG
 */
#define ptp_emerg(fmt, args...)     pr_err("[PTP] " fmt, ##args)
#define ptp_alert(fmt, args...)     pr_err("[PTP] " fmt, ##args)
#define ptp_crit(fmt, args...)      pr_err("[PTP] " fmt, ##args)
#define ptp_error(fmt, args...)     pr_err("[PTP] " fmt, ##args)
#define ptp_warning(fmt, args...)   pr_warn("[PTP] " fmt, ##args)
#define ptp_notice(fmt, args...)    pr_warn("[PTP] " fmt, ##args)
#define ptp_info(fmt, args...)      pr_warn("[PTP] " fmt, ##args)
#define ptp_debug(fmt, args...)     pr_warn("[PTP] " fmt, ##args)

#if EN_ISR_LOG
#define ptp_isr_info(fmt, args...)  ptp_notice(fmt, ##args)
#else
#define ptp_isr_info(fmt, args...)  ptp_debug(fmt, ##args)
#endif

#define FUNC_LV_MODULE          BIT(0)  /* module, platform driver interface */
#define FUNC_LV_CPUFREQ         BIT(1)  /* cpufreq driver interface          */
#define FUNC_LV_API             BIT(2)  /* mt_cpufreq driver global function */
#define FUNC_LV_LOCAL           BIT(3)  /* mt_cpufreq driver lcaol function  */
#define FUNC_LV_HELP            BIT(4)  /* mt_cpufreq driver help function   */

static unsigned int func_lv_mask = 0; // (FUNC_LV_MODULE | FUNC_LV_CPUFREQ | FUNC_LV_API | FUNC_LV_LOCAL | FUNC_LV_HELP);

#if defined(CONFIG_PTP_SHOWLOG)
#define FUNC_ENTER(lv)          do { if ((lv) & func_lv_mask) ptp_debug(">> %s()\n", __func__); } while (0)
#define FUNC_EXIT(lv)           do { if ((lv) & func_lv_mask) ptp_debug("<< %s():%d\n", __func__, __LINE__); } while (0)
#else
#define FUNC_ENTER(lv)
#define FUNC_EXIT(lv)
#endif /* CONFIG_CPU_DVFS_SHOWLOG */

/*
 * REG ACCESS
 */
#ifdef __KERNEL__
#define ptp_read(addr)	DRV_Reg32(addr)
#define ptp_read_field(addr, range)	\
	((ptp_read(addr) & BITMASK(range)) >> LSB(range))

#define ptp_write(addr, val)	mt_reg_sync_writel(val, addr)
#else
#define ptp_read(addr)          (*(volatile unsigned int *)(addr))
#define ptp_write(addr, val)    (*(volatile unsigned int *)(addr) = (unsigned long)(val))
#endif
/**
 * Write a field of a register.
 * @addr:	Address of the register
 * @range:	The field bit range in the form of MSB:LSB
 * @val:	The value to be written to the field
 */
#define ptp_write_field(addr, range, val)	\
	ptp_write(addr, (ptp_read(addr) & ~BITMASK(range)) | BITS(range, val))

/**
 * Helper macros
 */

/* PTP detector is disabled by who */
enum {
	BY_PROCFS	= BIT(0),
	BY_INIT_ERROR	= BIT(1),
	BY_MON_ERROR	= BIT(2),
};

#ifdef CONFIG_OF

void __iomem *ptpod_base;
u32 ptpod_irq_number = 0;
int ptpod_phy_base;
#endif


/**
 * iterate over list of detectors
 * @det:	the detector * to use as a loop cursor.
 */
#define for_each_det(det) for (det = ptp_detectors; det < (ptp_detectors + ARRAY_SIZE(ptp_detectors)); det++)

/**
 * iterate over list of detectors and its controller
 * @det:	the detector * to use as a loop cursor.
 * @ctrl:	the ptp_ctrl * to use as ctrl pointer of current det.
 */
#define for_each_det_ctrl(det, ctrl)				\
	for (det = ptp_detectors,				\
	     ctrl = id_to_ptp_ctrl(det->ctrl_id);		\
	     det < (ptp_detectors + ARRAY_SIZE(ptp_detectors)); \
	     det++,						\
	     ctrl = id_to_ptp_ctrl(det->ctrl_id))

/**
 * iterate over list of controllers
 * @pos:	the ptp_ctrl * to use as a loop cursor.
 */
#define for_each_ctrl(ctrl) for (ctrl = ptp_ctrls; ctrl < (ptp_ctrls + ARRAY_SIZE(ptp_ctrls)); ctrl++)

/**
 * Given a ptp_det * in ptp_detectors. Return the id.
 * @det:	pointer to a ptp_det in ptp_detectors
 */
#define det_to_id(det)	((det) - &ptp_detectors[0])

/**
 * Given a ptp_ctrl * in ptp_ctrls. Return the id.
 * @det:	pointer to a ptp_ctrl in ptp_ctrls
 */
#define ctrl_to_id(ctrl)	((ctrl) - &ptp_ctrls[0])

/**
 * Check if a detector has a feature
 * @det:	pointer to a ptp_det to be check
 * @feature:	enum ptp_features to be checked
 */
#define HAS_FEATURE(det, feature)	((det)->features & feature)

#define PERCENT(numerator, denominator)	\
	(unsigned char)(((numerator) * 100 + (denominator) - 1) / (denominator))

/*=============================================================
 * Local type definition
 *=============================================================*/

/*
 * CONFIG (CHIP related)
 * PTPCORESEL.APBSEL
 */
unsigned int reg_dump_addr_off[] = {
	0x0000,
	0x0004,
	0x0008,
	0x000C,
	0x0010,
	0x0014,
	0x0018,
	0x001c,
	0x0024,
	0x0028,
	0x002c,
	0x0030,
	0x0034,
	0x0038,
	0x003c,
	0x0040,
	0x0044,
	0x0048,
	0x004c,
	0x0050,
	0x0054,
	0x0058,
	0x005c,
	0x0060,
	0x0064,
	0x0068,
	0x006c,
	0x0070,
	0x0074,
	0x0078,
	0x007c,
	0x0080,
	0x0084,
	0x0088,
	0x008c,
	0x0090,
	0x0094,
	0x0098,
	0x00a0,
	0x00a4,
	0x00a8,
	0x00B0,
	0x00B4,
	0x00B8,
	0x00BC,
	0x00C0,
	0x00C4,
	0x00C8,
	0x00CC,
	0x00F0,
	0x00F4,
	0x00F8,
	0x00FC,
	0x0200,
	0x0204,
	0x0208,
	0x020C,
	0x0210,
	0x0214,
	0x0218,
	0x021C,
	0x0220,
	0x0224,
	0x0228,
	0x022C,
	0x0230,
	0x0234,
	0x0238,
	0x023C,
	0x0240,
	0x0244,
	0x0248,
	0x024C,
	0x0250,
	0x0254,
	0x0258,
	0x025C,
	0x0260,
	0x0264,
	0x0268,
	0x026C,
	0x0270,
	0x0274,
	0x0278,
	0x027C,
	0x0280,
	0x0284,
	0x0400,
	0x0404,
	0x0408,
	0x040C,
	0x0410,
	0x0414,
	0x0418,
	0x041C,
	0x0420,
	0x0424,
	0x0428,
	0x042C,
	0x0430,
};

typedef enum {
	PTP_PHASE_INIT01,
	PTP_PHASE_INIT02,
	PTP_PHASE_MON,

	NR_PTP_PHASE,
} ptp_phase;

enum {
	PTP_VOLT_NONE    = 0,
	PTP_VOLT_UPDATE  = BIT(0),
	PTP_VOLT_RESTORE = BIT(1),
};

struct ptp_ctrl {
	const char *name;
	ptp_det_id det_id;
	//struct completion init_done;
	//atomic_t in_init;
#ifdef __KERNEL__
	/* for voltage setting thread */
	wait_queue_head_t wq;
#endif
	int volt_update;
	struct task_struct *thread;
};

struct ptp_det_ops {
	/* interface to PTP-OD */
	void (*enable)(struct ptp_det *det, int reason);
	void (*disable)(struct ptp_det *det, int reason);
	void (*disable_locked)(struct ptp_det *det, int reason);
	void (*switch_bank)(struct ptp_det *det);

	int (*init01)(struct ptp_det *det);
	int (*init02)(struct ptp_det *det);
	int (*mon_mode)(struct ptp_det *det);

	int (*get_status)(struct ptp_det *det);
	void (*dump_status)(struct ptp_det *det);

	void (*set_phase)(struct ptp_det *det, ptp_phase phase);

	/* interface to thermal */
	int (*get_temp)(struct ptp_det *det);

	/* interface to DVFS */
	int (*get_volt)(struct ptp_det *det);
	int (*set_volt)(struct ptp_det *det);
	void (*restore_default_volt)(struct ptp_det *det);
	void (*get_freq_table)(struct ptp_det *det);
};

enum ptp_features {
	FEA_INIT01	= BIT(PTP_PHASE_INIT01),
	FEA_INIT02	= BIT(PTP_PHASE_INIT02),
	FEA_MON		= BIT(PTP_PHASE_MON),
};

struct ptp_det {
	const char *name;
	struct ptp_det_ops *ops;
	int status;		/* TODO: enable/disable */
	int features;		/* enum ptp_features */
	ptp_ctrl_id ctrl_id;

	/* devinfo */
	unsigned int PTPINITEN;
	unsigned int PTPMONEN;
	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGEDELTA;
	unsigned int MTDES;

	/* constant */
	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int DCCONFIG;

	/* Generated by PTP init01. Used in PTP init02 */
	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;

	/* for debug */
	unsigned int dcvalues[NR_PTP_PHASE];

	unsigned int ptp_freqpct30[NR_PTP_PHASE];
	unsigned int ptp_26c[NR_PTP_PHASE];
	unsigned int ptp_vop30[NR_PTP_PHASE];
	unsigned int ptp_ptpen[NR_PTP_PHASE];
#if DUMP_DATA_TO_DE
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_PTP_PHASE];
#endif
	/* slope */
	unsigned int MTS;
	unsigned int BTS;

	/* dvfs */
	unsigned int num_freq_tbl; /* could be got @ the same time
				      with freq_tbl[] */
	unsigned int max_freq_khz; /* maximum frequency used to
				      calculate percentage */
	unsigned char freq_tbl[NR_FREQ]; /* percentage to maximum freq */

	unsigned int volt_tbl[NR_FREQ];
	unsigned int volt_tbl_init2[NR_FREQ];
	unsigned int volt_tbl_pmic[NR_FREQ];
	int volt_offset;

	int disabled; /* Disabled by error or sysfs */
};

struct ptp_devinfo {
	/* M_HW_RES0 */
	unsigned int LTE_DCMDET: 8;
	unsigned int CPU_DCMDET: 8;
	unsigned int GPU_DCMDET: 8;
	unsigned int SOC_DCMDET: 8;
	/* M_HW_RES1 */
	unsigned int LTE_DCBDET: 8;
	unsigned int CPU_DCBDET: 8;
	unsigned int GPU_DCBDET: 8;
	unsigned int SOC_DCBDET: 8;
	/* M_HW_RES2 */
	unsigned int LTE_AGEDELTA: 8;
	unsigned int CPU_AGEDELTA: 8;
	unsigned int GPU_AGEDELTA: 8;
	unsigned int SOC_AGEDELTA: 8;
	/* M_HW_RES3 */
	unsigned int SOC_AO_DCBDET: 8;
	unsigned int SOC_AO_DCMDET: 8;
	unsigned int SOC_AO_BDES: 8;
	unsigned int SOC_AO_MDES: 8;
	/* M_HW_RES4 */
	unsigned int PTPINITEN: 1;
	unsigned int PTPMONEN: 1;
	unsigned int Bodybias: 1;
	unsigned int PTPOD_T: 1;
	unsigned int EPS: 1;
	unsigned int M_HW_RES4_OTHERS: 27;
	/* M_HW_RES5 */
	unsigned int M_HW_RES5: 32;
	/* M_HW_RES6 */
	unsigned int M_HW_RES6: 32;
	/* M_HW_RES7 */
	unsigned int LTE_MDES: 8;
	unsigned int CPU_MDES: 8;
	unsigned int GPU_MDES: 8;
	unsigned int SOC_MDES: 8;
	/* M_HW_RES8 */
	unsigned int LTE_BDES: 8;
	unsigned int CPU_BDES: 8;
	unsigned int GPU_BDES: 8;
	unsigned int SOC_BDES: 8;
	/* M_HW_RES9 */
	unsigned int LTE_MTDES: 8;
	unsigned int CPU_MTDES: 8;
	unsigned int GPU_MTDES: 8;
	unsigned int SOC_MTDES: 8;
};

/*=============================================================
 *Local variable definition
 *=============================================================*/
static int ptp_probe(struct platform_device *pdev);
static int ptp_suspend(struct platform_device *pdev, pm_message_t state);
static int ptp_resume(struct platform_device *pdev);

#ifdef __KERNEL__
/*
 * lock
 */
static DEFINE_SPINLOCK(ptp_spinlock);
#endif
/**
 * PTP controllers
 */
struct ptp_ctrl ptp_ctrls[NR_PTP_CTRL] = {
	[PTP_CTRL_CPU] = {
		.name = __stringify(PTP_CTRL_CPU),
		.det_id = PTP_DET_CPU,
	},

	[PTP_CTRL_LTE] = {
		.name = __stringify(PTP_CTRL_LTE),
		.det_id = PTP_DET_LTE,
	},

	[PTP_CTRL_GPU] = {
		.name = __stringify(PTP_CTRL_GPU),
		.det_id = PTP_DET_GPU,
	},

	[PTP_CTRL_SOC] = {
		.name = __stringify(PTP_CTRL_SOC),
		/* AO and PDN share the same controller */
		.det_id = PTP_DET_SOC,
	},
};

/*
 * PTP detectors
 */
static void base_ops_enable(struct ptp_det *det, int reason);
static void base_ops_disable(struct ptp_det *det, int reason);
static void base_ops_disable_locked(struct ptp_det *det, int reason);
static void base_ops_switch_bank(struct ptp_det *det);

static int base_ops_init01(struct ptp_det *det);
static int base_ops_init02(struct ptp_det *det);
static int base_ops_mon_mode(struct ptp_det *det);

static int base_ops_get_status(struct ptp_det *det);
static void base_ops_dump_status(struct ptp_det *det);

static void base_ops_set_phase(struct ptp_det *det, ptp_phase phase);
static int base_ops_get_temp(struct ptp_det *det);
static int base_ops_get_volt(struct ptp_det *det);
static int base_ops_set_volt(struct ptp_det *det);
static void base_ops_restore_default_volt(struct ptp_det *det);
static void base_ops_get_freq_table(struct ptp_det *det);

static int get_volt_cpu(struct ptp_det *det);
static int set_volt_cpu(struct ptp_det *det);
static void restore_default_volt_cpu(struct ptp_det *det);
static void get_freq_table_cpu(struct ptp_det *det);

static int get_volt_lte(struct ptp_det *det);
static int set_volt_lte(struct ptp_det *det);
//static void switch_to_vcore_ao(struct ptp_det *det);
//static void switch_to_vcore_pdn(struct ptp_det *det);

static int get_volt_gpu(struct ptp_det *det);
static int set_volt_gpu(struct ptp_det *det);

static void restore_default_volt_gpu(struct ptp_det *det);
static void get_freq_table_gpu(struct ptp_det *det);

static int get_volt_vcore(struct ptp_det *det);

#ifndef __KERNEL__
static int set_volt_vcore(struct ptp_det *det);
#endif

#define BASE_OP(fn)	.fn = base_ops_ ## fn
static struct ptp_det_ops ptp_det_base_ops = {
	BASE_OP(enable),
	BASE_OP(disable),
	BASE_OP(disable_locked),
	BASE_OP(switch_bank),

	BASE_OP(init01),
	BASE_OP(init02),
	BASE_OP(mon_mode),

	BASE_OP(get_status),
	BASE_OP(dump_status),

	BASE_OP(set_phase),

	BASE_OP(get_temp),

	BASE_OP(get_volt),
	BASE_OP(set_volt),
	BASE_OP(restore_default_volt),
	BASE_OP(get_freq_table),
};

static struct ptp_det_ops cpu_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};

static struct ptp_det_ops lte_det_ops = {
	.get_volt		= get_volt_lte,
	.set_volt		= set_volt_lte,
	.restore_default_volt	= NULL,
	.get_freq_table		= NULL,
};

static struct ptp_det_ops gpu_det_ops = {
	.get_volt		= get_volt_gpu,
	.set_volt		= set_volt_gpu, // <-@@@
	.restore_default_volt	= restore_default_volt_gpu,
	.get_freq_table		= get_freq_table_gpu,
};

static struct ptp_det_ops soc_det_ops = {
	.get_volt				= get_volt_vcore,// <-@@@@@@@@@@@@
#ifndef __KERNEL__
	.set_volt				= set_volt_vcore,// <-@@@@@@@@@@@@
#else
	.set_volt				= NULL,// <-@@@@@@@@@@@@
#endif
	.restore_default_volt	= NULL,// <-@@@@@@@@@@@@
	.get_freq_table			= NULL,// <-@@@@@@@@@@@@
};

static struct ptp_det ptp_detectors[NR_PTP_DET] = {
	[PTP_DET_CPU] = {
		.name		= __stringify(PTP_DET_CPU),
		.ops		= &cpu_det_ops,
		.ctrl_id	= PTP_CTRL_CPU,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 2000000, // TODO: FIXME
		.VBOOT		= PTP_VOLT_TO_PMIC_VAL(100000), /* 1.0v: 0x30 */
		//.volt_offset	= 12, // <-@@@
	},

	[PTP_DET_LTE] = {
		.name		= __stringify(PTP_DET_LTE),
		.ops		= &lte_det_ops,
		.ctrl_id	= PTP_CTRL_LTE,
		.features	= FEA_INIT01 | FEA_INIT02,
		.max_freq_khz	= 300000, // TODO: FIXME
		.VBOOT		= PTP_VOLT_TO_PMIC_VAL(100000), /* 1.0v: 0x30 */
		//.volt_offset	= 12, // <-@@@
	},

	[PTP_DET_GPU] = {
		.name		= __stringify(PTP_DET_GPU),
		.ops		= &gpu_det_ops,
		.ctrl_id	= PTP_CTRL_GPU,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON, // <-@@@
		.max_freq_khz	= 700000, // 700MHz
		.VBOOT		= PTP_VOLT_TO_PMIC_VAL(100000), /* 1.0v: 0x30 */
		//.volt_offset	= 12, // <-@@@
	},

	[PTP_DET_SOC] = {
		.name		= __stringify(PTP_DET_SOC),
		.ops		= &soc_det_ops,
		.ctrl_id	= PTP_CTRL_SOC,
		.features	= FEA_INIT01 | FEA_INIT02,
		.VBOOT		= PTP_VOLT_TO_PMIC_VAL(112500), /* 1.125v: 0x44 */
	},
};

static struct ptp_devinfo ptp_devinfo;

static unsigned int ptp_level; /* debug info */
#ifdef __KERNEL__
/**
 * timer for log
 */
static struct hrtimer ptp_log_timer;
#endif
/*=============================================================
 * Local function definition
 *=============================================================*/

static struct ptp_det *id_to_ptp_det(ptp_det_id id)
{
	if (likely(id < NR_PTP_DET))
		return &ptp_detectors[id];
	else
		return NULL;
}

static struct ptp_ctrl *id_to_ptp_ctrl(ptp_ctrl_id id)
{
	if (likely(id < NR_PTP_CTRL))
		return &ptp_ctrls[id];
	else
		return NULL;
}

static void base_ops_enable(struct ptp_det *det, int reason)
{
	/* FIXME: UNDER CONSTRUCTION */
	FUNC_ENTER(FUNC_LV_HELP);
	det->disabled &= ~reason;
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_switch_bank(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	ptp_write_field(PTP_PTPCORESEL, APBSEL, det->ctrl_id);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable_locked(struct ptp_det *det, int reason)
{
	FUNC_ENTER(FUNC_LV_HELP);

	/* disable PTP */
	ptp_write(PTP_PTPEN, 0x0);

	/* Clear PTP interrupt PTPINTSTS */
	ptp_write(PTP_PTPINTSTS, 0x00ffffff);

	switch (reason) {
	case BY_MON_ERROR:
		/* set init2 value to DVFS table (PMIC) */
		memcpy(det->volt_tbl, det->volt_tbl_init2, sizeof(det->volt_tbl_init2));
		ptp_set_ptp_volt(det);
		break;

	case BY_INIT_ERROR:
	case BY_PROCFS:
	default:
		/* restore default DVFS table (PMIC) */
		ptp_restore_ptp_volt(det);
		break;
	}

	ptp_notice("Disable PTP-OD[%s] done.\n", det->name);
	det->disabled |= reason;

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable(struct ptp_det *det, int reason)
{
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det);
	det->ops->disable_locked(det, reason);
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_init01(struct ptp_det *det)
{
	//struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id);

	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT01))) {
		ptp_notice("det %s has no INIT01\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		ptp_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	ptp_notice("%s(%s) start (ptp_level = 0x%08X).\n", __func__, det->name, ptp_level);
	//atomic_inc(&ctrl->in_init);
	//ptp_init01_prepare(det);
	//det->ops->dump_status(det); // <-@@@
	det->ops->set_phase(det, PTP_PHASE_INIT01);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_init02(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT02))) {
		ptp_notice("det %s has no INIT02\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		ptp_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	ptp_notice("%s(%s) start (ptp_level = 0x%08X).\n", __func__, det->name, ptp_level);
	ptp_notice("DCVOFFSETIN = 0x%08X\n", det->DCVOFFSETIN);
	ptp_notice("AGEVOFFSETIN = 0x%08X\n", det->AGEVOFFSETIN);
	// det->ops->dump_status(det); // <-@@@
	det->ops->set_phase(det, PTP_PHASE_INIT02);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_mon_mode(struct ptp_det *det)
{
	struct TS_PTPOD ts_info;
	thermal_bank_name ts_bank;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!HAS_FEATURE(det, FEA_MON)) {
		ptp_notice("det %s has no MON mode\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		ptp_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	ptp_notice("%s(%s) start (ptp_level = 0x%08X).\n", __func__, det->name, ptp_level);

#ifdef __KERNEL__
	ts_bank = det->ctrl_id; // TODO: FIXME

	get_thermal_slope_intercept(&ts_info, ts_bank);

	det->MTS = ts_info.ts_MTS;
	det->BTS = ts_info.ts_BTS;

#else
	det->MTS =  0x2cf; // (2048 * TS_SLOPE) + 2048;
	det->BTS =  0x80E;// 4 * TS_INTERCEPT;
#endif

	if ((det->PTPINITEN == 0x0) || (det->PTPMONEN == 0x0)) {
		ptp_notice("PTPINITEN = 0x%08X, PTPMONEN = 0x%08X\n", det->PTPINITEN, det->PTPMONEN);
		FUNC_EXIT(FUNC_LV_HELP);
		return 1;
	}

	det->ops->dump_status(det);
	det->ops->set_phase(det, PTP_PHASE_MON);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_get_status(struct ptp_det *det)
{
	int status;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det);
	status = (ptp_read(PTP_PTPEN) != 0) ? 1 : 0;
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);

	return status;
}

static void base_ops_dump_status(struct ptp_det *det)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	ptp_isr_info("[%s]\n",			det->name);

	ptp_isr_info("PTPINITEN = 0x%08X\n",	det->PTPINITEN);
	ptp_isr_info("PTPMONEN = 0x%08X\n",	det->PTPMONEN);
	ptp_isr_info("MDES = 0x%08X\n",		det->MDES);
	ptp_isr_info("BDES = 0x%08X\n",		det->BDES);
	ptp_isr_info("DCMDET = 0x%08X\n",		det->DCMDET);

	ptp_isr_info("DCCONFIG = 0x%08X\n",	det->DCCONFIG);
	ptp_isr_info("DCBDET = 0x%08X\n",		det->DCBDET);

	ptp_isr_info("AGECONFIG = 0x%08X\n",	det->AGECONFIG);
	ptp_isr_info("AGEM = 0x%08X\n",		det->AGEM);

	ptp_isr_info("AGEDELTA = 0x%08X\n",	det->AGEDELTA);
	ptp_isr_info("DVTFIXED = 0x%08X\n",	det->DVTFIXED);
	ptp_isr_info("MTDES = 0x%08X\n",		det->MTDES);
	ptp_isr_info("VCO = 0x%08X\n",		det->VCO);

	ptp_isr_info("DETWINDOW = 0x%08X\n",	det->DETWINDOW);
	ptp_isr_info("VMAX = 0x%08X\n",		det->VMAX);
	ptp_isr_info("VMIN = 0x%08X\n",		det->VMIN);
	ptp_isr_info("DTHI = 0x%08X\n",		det->DTHI);
	ptp_isr_info("DTLO = 0x%08X\n",		det->DTLO);
	ptp_isr_info("VBOOT = 0x%08X\n",		det->VBOOT);
	ptp_isr_info("DETMAX = 0x%08X\n",		det->DETMAX);

	ptp_isr_info("DCVOFFSETIN = 0x%08X\n",	det->DCVOFFSETIN);
	ptp_isr_info("AGEVOFFSETIN = 0x%08X\n",	det->AGEVOFFSETIN);

	ptp_isr_info("MTS = 0x%08X\n",		det->MTS);
	ptp_isr_info("BTS = 0x%08X\n",		det->BTS);

	ptp_isr_info("num_freq_tbl = %d\n", det->num_freq_tbl);

	for (i = 0; i < det->num_freq_tbl; i++)
		ptp_isr_info("freq_tbl[%d] = %d\n", i, det->freq_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		ptp_isr_info("volt_tbl[%d] = %d\n", i, det->volt_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		ptp_isr_info("volt_tbl_init2[%d] = %d\n", i, det->volt_tbl_init2[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		ptp_isr_info("volt_tbl_pmic[%d] = %d\n", i, det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_set_phase(struct ptp_det *det, ptp_phase phase)
{
	unsigned int i, filter, val;
	// unsigned long flags; // <-XXX

	FUNC_ENTER(FUNC_LV_HELP);

	// mt_ptp_lock(&flags); // <-XXX

	det->ops->switch_bank(det);
	/* config PTP register */
	ptp_write(PTP_DESCHAR,
		  ((det->BDES << 8) & 0xff00) | (det->MDES & 0xff));
	ptp_write(PTP_TEMPCHAR,
		  (((det->VCO << 16) & 0xff0000) |
		   ((det->MTDES << 8) & 0xff00) | (det->DVTFIXED & 0xff)));
	ptp_write(PTP_DETCHAR,
		  ((det->DCBDET << 8) & 0xff00) | (det->DCMDET & 0xff));
	ptp_write(PTP_AGECHAR,
		  ((det->AGEDELTA << 8) & 0xff00) | (det->AGEM & 0xff));
	ptp_write(PTP_DCCONFIG, det->DCCONFIG);
	ptp_write(PTP_AGECONFIG, det->AGECONFIG);

	if (PTP_PHASE_MON == phase)
		ptp_write(PTP_TSCALCS,
			  ((det->BTS << 12) & 0xfff000) | (det->MTS & 0xfff));

	if (det->AGEM == 0x0)
		ptp_write(PTP_RUNCONFIG, 0x80000000);
	else {
		val = 0x0;

		for (i = 0; i < 24; i += 2) {
			filter = 0x3 << i;

			if (((det->AGECONFIG) & filter) == 0x0)
				val |= (0x1 << i);
			else
				val |= ((det->AGECONFIG) & filter);
		}

		ptp_write(PTP_RUNCONFIG, val);
	}

	ptp_write(PTP_FREQPCT30,
		  ((det->freq_tbl[3] << 24) & 0xff000000)	|
		  ((det->freq_tbl[2] << 16) & 0xff0000)	|
		  ((det->freq_tbl[1] << 8) & 0xff00)	|
		  (det->freq_tbl[0] & 0xff));
	ptp_write(PTP_FREQPCT74,
		  ((det->freq_tbl[7] << 24) & 0xff000000)	|
		  ((det->freq_tbl[6] << 16) & 0xff0000)	|
		  ((det->freq_tbl[5] << 8) & 0xff00)	|
		  ((det->freq_tbl[4]) & 0xff));
	ptp_write(PTP_LIMITVALS,
		  ((det->VMAX << 24) & 0xff000000)	|
		  ((det->VMIN << 16) & 0xff0000)	|
		  ((det->DTHI << 8) & 0xff00)		|
		  (det->DTLO & 0xff));
	ptp_write(PTP_VBOOT, (((det->VBOOT) & 0xff)));
	ptp_write(PTP_DETWINDOW, (((det->DETWINDOW) & 0xffff)));
	ptp_write(PTP_PTPCONFIG, (((det->DETMAX) & 0xffff)));

	/* clear all pending PTP interrupt & config PTPINTEN */
	ptp_write(PTP_PTPINTSTS, 0xffffffff);

	ptp_notice("%s(%s) start (phase = %d).\n", __func__, det->name, phase);
	switch (phase) {
	case PTP_PHASE_INIT01:
		ptp_write(PTP_PTPINTEN, 0x00005f01);
		/* enable PTP INIT measurement */
		ptp_write(PTP_PTPEN, 0x00000001);
		break;

	case PTP_PHASE_INIT02:
		ptp_write(PTP_PTPINTEN, 0x00005f01);
		ptp_write(PTP_INIT2VALS,
			  ((det->AGEVOFFSETIN << 16) & 0xffff0000) |
			  (det->DCVOFFSETIN & 0xffff));
		/* enable PTP INIT measurement */
		ptp_write(PTP_PTPEN, 0x00000005);
		break;

	case PTP_PHASE_MON:
		ptp_write(PTP_PTPINTEN, 0x00FF0000);
		/* enable PTP monitor mode */
		ptp_write(PTP_PTPEN, 0x00000002);
		break;

	default:
		BUG();
		break;
	}

	// mt_ptp_unlock(&flags); // <-XXX

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_get_temp(struct ptp_det *det)
{
	thermal_bank_name ts_bank;
#if 1 // TODO: FIXME wait for Jerry
	FUNC_ENTER(FUNC_LV_HELP);
	/*
	    THERMAL_BANK0     = 0,//CPU (TS_MCU1,TS_MCU2)
	    THERMAL_BANK1     = 1,//LTE (TS_MCU4)
	    THERMAL_BANK2     = 2,//GPU (TS_MCU3)
	    THERMAL_BANK3     = 3,//SOC (TS_MCU1,TS_MCU2,TS_MCU3,TS_MCU4)

	*/
	ts_bank = (det_to_id(det) == PTP_DET_CPU)? THERMAL_BANK0  :
		      (det_to_id(det) == PTP_DET_LTE)? THERMAL_BANK1  :
		      (det_to_id(det) == PTP_DET_GPU)? THERMAL_BANK2  :
		       THERMAL_BANK3;

	FUNC_EXIT(FUNC_LV_HELP);
#endif
	return tscpu_get_temp_by_bank(ts_bank);
}

static int base_ops_get_volt(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	ptp_warning("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_set_volt(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	ptp_warning("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void base_ops_restore_default_volt(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	ptp_warning("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_get_freq_table(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	det->freq_tbl[0] = 100;
	det->num_freq_tbl = 1;

	FUNC_EXIT(FUNC_LV_HELP);
}

//Will return 10uV
static int get_volt_cpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	return mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LITTLE);// unit mv * 100 = 10uv
}

//volt_tbl_pmic is convert from 10uV
static int set_volt_cpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	//ptp_notice("set_volt_cpu=%x %x %x %x %x\n",
	//det->volt_tbl_pmic[0],det->volt_tbl_pmic[1], det->volt_tbl_pmic[2], det->volt_tbl_pmic[3], det->volt_tbl_pmic[4]);
	return mt_cpufreq_update_volt(MT_CPU_DVFS_LITTLE, det->volt_tbl_pmic, det->num_freq_tbl);
}

static int get_volt_lte(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	return mt_get_cur_volt_lte();// unit mv * 100 = 10uv
}

static int set_volt_lte(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
#ifdef __KERNEL__
	return mt_set_cur_volt_lte(det->volt_tbl_pmic[0]);
#else
	return dvfs_set_vlte_volt(det->volt_tbl_pmic[0]);
#endif
}

static void restore_default_volt_cpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	mt_cpufreq_restore_default_volt(MT_CPU_DVFS_LITTLE);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void get_freq_table_cpu(struct ptp_det *det)
{
	int i;
	enum mt_cpu_dvfs_id cpu;

	FUNC_ENTER(FUNC_LV_HELP);
	cpu = MT_CPU_DVFS_LITTLE;
#if 0
	if (cpu != 0)
		return; // TODO: FIXME, just for E1
#endif

	//det->max_freq_khz = mt_cpufreq_get_freq_by_idx(cpu, 0);

	for (i = 0; i < NR_FREQ; i++) {
		det->freq_tbl[i] = PERCENT(mt_cpufreq_get_freq_by_idx(cpu, i), det->max_freq_khz);

		if (0 == det->freq_tbl[i])
			break;
	}

	det->num_freq_tbl = i;

	FUNC_EXIT(FUNC_LV_HELP);
}
#if 0
static void switch_to_vcore_ao(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	ptp_write_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL, SEL_VCORE_AO);
	ptp_write_field(PTP_PTPCORESEL, APBSEL, det->ctrl_id);
	ptp_ctrls[PTP_CTRL_VCORE].det_id = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void switch_to_vcore_pdn(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	ptp_write_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL, SEL_VCORE_PDN);
	ptp_write_field(PTP_PTPCORESEL, APBSEL, det->ctrl_id);
	ptp_ctrls[PTP_CTRL_SOC].det_id = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
}
#endif

static int get_volt_gpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
#ifdef __KERNEL__
	return mt_gpufreq_get_cur_volt(); // TODO: FIXME, unit = 10 uv wait fot gpu
#else
	//return gpu_dvfs_get_cur_volt(); //Ask gpu owner
#endif
}

static int set_volt_gpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	unsigned int lock_code = get_devinfo_with_index(23);
#ifdef __KERNEL__
	if ((lock_code & 0x1) == 1) 
		return mt_gpufreq_voltage_set_by_ptpod(det->volt_tbl_pmic, det->num_freq_tbl);
	else
		return 0;
#else
	return gpu_dvfs_set_cur_volt(det->volt_tbl_pmic); //Ask gpu owner
#endif
}

static void restore_default_volt_gpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	mt_gpufreq_return_default_DVS_by_ptpod(); //wait for gpu
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void get_freq_table_gpu(struct ptp_det *det)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	
	//det->max_freq_khz = mt_gpufreq_get_frequency_by_level(0);
	for (i = 0; i < NR_FREQ; i++) {
		det->freq_tbl[i] = PERCENT(mt_gpufreq_get_frequency_by_level(i), det->max_freq_khz); // TODO: FIXME

		if (0 == det->freq_tbl[i])
			break;
	}

#else
	int gpu_freq[5] = {550000, 450000, 400000, 300000, 250000};

	for (i = 0; i < 5; i++) {
		det->freq_tbl[i] = PERCENT(gpu_freq[i], det->max_freq_khz); // TODO: FIXME

		if (0 == det->freq_tbl[i])
			break;
	}

#endif
	FUNC_EXIT(FUNC_LV_HELP);

	det->num_freq_tbl = i;
}
#ifndef __KERNEL__
static int set_volt_vcore(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	//return mt_set_cur_volt_vcore_pdn(det->volt_tbl_pmic[0]); // unit = 10 uv
	return dvfs_set_vcore_pdn_volt(det->volt_tbl_pmic[0]);
}
#endif
static int get_volt_vcore(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	return mt_get_cur_volt_vcore_pdn(); // unit = 10 uv
}

/*=============================================================
 * Global function definition
 *=============================================================*/
#if 0
unsigned int mt_ptp_get_level(void)
{
	unsigned int spd_bin_resv = 0, ret = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	spd_bin_resv = (get_devinfo_with_index(15) >> 28) & 0x7;

	switch (spd_bin_resv) {
	case 1:
		ret = 1; /* 2.0G */
		break;

	case 2:
		ret = 2; /* 1.3G */
		break;

	case 4:
		ret = 2; /* 1.3G */
		break;

	default:
		ret = 0; /* 1.7G */
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}
#endif
#if 0	// TODO: FIXME, remove it latter (unused)
static unsigned int ptp_trasnfer_to_volt(unsigned int value)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	/* (700mv + n * 6.25mv) */
	return VAL_TO_MV(value);
}
#endif	// TODO: FIXME, remove it latter (unused)

void mt_ptp_lock(unsigned long *flags)
{
	// FUNC_ENTER(FUNC_LV_HELP);
	/* FIXME: lock with MD32 */
	/* get_md32_semaphore(SEMAPHORE_PTP); */
#ifdef __KERNEL__
	spin_lock_irqsave(&ptp_spinlock, *flags);
#endif
	// FUNC_EXIT(FUNC_LV_HELP);
}
EXPORT_SYMBOL(mt_ptp_lock);

void mt_ptp_unlock(unsigned long *flags)
{
	// FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	spin_unlock_irqrestore(&ptp_spinlock, *flags);
	/* FIXME: lock with MD32 */
	/* release_md32_semaphore(SEMAPHORE_PTP); */
#endif
	// FUNC_EXIT(FUNC_LV_HELP);
}
EXPORT_SYMBOL(mt_ptp_unlock);

#if 0
int mt_ptp_idle_can_enter(void)
{
	struct ptp_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_ctrl(ctrl) {
		if (atomic_read(&ctrl->in_init)) {
			FUNC_EXIT(FUNC_LV_HELP);
			return 0;
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 1;
}
EXPORT_SYMBOL(mt_ptp_idle_can_enter);
#endif

#ifdef __KERNEL__
/*
 * timer for log
 */
static enum hrtimer_restart ptp_log_timer_func(struct hrtimer *timer)
{
	struct ptp_det *det;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_det(det) {
		ptp_notice("PTP_LOG: PTPOD[%s] (%d) - "
			   "(%d, %d, %d, %d, %d, %d, %d, %d) - "
			   "(%d, %d, %d, %d, %d, %d, %d, %d)\n",
			   det->name, det->ops->get_temp(det),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
			   det->freq_tbl[0],
			   det->freq_tbl[1],
			   det->freq_tbl[2],
			   det->freq_tbl[3],
			   det->freq_tbl[4],
			   det->freq_tbl[5],
			   det->freq_tbl[6],
			   det->freq_tbl[7]);
	}

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	FUNC_EXIT(FUNC_LV_HELP);

	return HRTIMER_RESTART;
}
#endif
/*
 * Thread for voltage setting
 */
static int ptp_volt_thread_handler(void *data)
{
	struct ptp_ctrl *ctrl = (struct ptp_ctrl *)data;
	struct ptp_det *det = id_to_ptp_det(ctrl->det_id);

	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	//set_freezable();
	do {
		wait_event_interruptible(ctrl->wq, ctrl->volt_update);
		//wait_event_freezable(ctrl->wq, ctrl->volt_update);
		//try_to_freeze();
		if ((ctrl->volt_update & PTP_VOLT_UPDATE) && det->ops->set_volt) {
#ifdef CONFIG_PTP_AEE_RR_REC
			int temp = -1;

			switch (det->ctrl_id) {
				case PTP_CTRL_CPU:
					aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() | (1 << PTP_CPU_LITTLE_IS_SET_VOLT));
					temp = PTP_CPU_LITTLE_IS_SET_VOLT;
					break;

				case PTP_CTRL_LTE:
					aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() | (1 << PTP_CPU_BIG_IS_SET_VOLT));
					temp = PTP_CPU_BIG_IS_SET_VOLT;
					break;

				case PTP_CTRL_GPU:
					aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() | (1 << PTP_GPU_IS_SET_VOLT));
					temp = PTP_GPU_IS_SET_VOLT;
					break;

				default:
					break;
			}

#endif
			det->ops->set_volt(det);
			
#ifdef CONFIG_PTP_AEE_RR_REC
			if (temp >= PTP_CPU_LITTLE_IS_SET_VOLT)
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() & ~(1 << temp));
#endif

		}
		if ((ctrl->volt_update & PTP_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

		ctrl->volt_update = PTP_VOLT_NONE;

	} while (!kthread_should_stop());

#endif
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void inherit_base_det(struct ptp_det *det)
{
	/*
	 * Inherit ops from ptp_det_base_ops if ops in det is NULL
	 */
	FUNC_ENTER(FUNC_LV_HELP);

#define INIT_OP(ops, func)					\
		do {							\
			if (ops->func == NULL)				\
				ops->func = ptp_det_base_ops.func;	\
		} while (0)

	INIT_OP(det->ops, disable);
	INIT_OP(det->ops, disable_locked);
	INIT_OP(det->ops, switch_bank);
	INIT_OP(det->ops, init01);
	INIT_OP(det->ops, init02);
	INIT_OP(det->ops, mon_mode);
	INIT_OP(det->ops, get_status);
	INIT_OP(det->ops, dump_status);
	INIT_OP(det->ops, set_phase);
	INIT_OP(det->ops, get_temp);
	INIT_OP(det->ops, get_volt);
	INIT_OP(det->ops, set_volt);
	INIT_OP(det->ops, restore_default_volt);
	INIT_OP(det->ops, get_freq_table);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void ptp_init_ctrl(struct ptp_ctrl *ctrl)
{
	FUNC_ENTER(FUNC_LV_HELP);

	//	init_completion(&ctrl->init_done);
	//	atomic_set(&ctrl->in_init, 0);
#ifdef __KERNEL__

	if (1) { // HAS_FEATURE(id_to_ptp_det(ctrl->det_id), FEA_MON)) { // TODO: FIXME, why doesn't work <-XXX
		init_waitqueue_head(&ctrl->wq);
		ctrl->thread = kthread_run(ptp_volt_thread_handler, ctrl, ctrl->name);

		if (IS_ERR(ctrl->thread))
			ptp_error("Create %s thread failed: %ld\n", ctrl->name, PTR_ERR(ctrl->thread));
	}

#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void ptp_init_det(struct ptp_det *det, struct ptp_devinfo *devinfo)
{
	unsigned int lock_code = get_devinfo_with_index(23);
	ptp_det_id det_id = det_to_id(det);

	FUNC_ENTER(FUNC_LV_HELP);

	inherit_base_det(det);

	/* init with devinfo */
	det->PTPINITEN		= devinfo->PTPINITEN;
	det->PTPMONEN		= devinfo->PTPMONEN;

	/* init with constant */
	det->DETWINDOW	= DETWINDOW_VAL;
	det->VMAX	= VMAX_VAL;
	det->VMIN	= VMIN_VAL;
	det->VBOOT		= PTP_VOLT_TO_PMIC_VAL(100000);

	det->DTHI	= DTHI_VAL;
	det->DTLO	= DTLO_VAL;
	det->DETMAX	= DETMAX_VAL;

	det->AGECONFIG	= AGECONFIG_VAL;
	det->AGEM	= AGEM_VAL;
	det->DVTFIXED	= DVTFIXED_VAL;
	det->VCO	= VCO_VAL;
	det->DCCONFIG	= DCCONFIG_VAL;

	if (NULL != det->ops->get_volt) {
		det->VBOOT = PTP_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));
		pr_notice("@%s(), det->VBOOT = %d\n", __func__, det->VBOOT);
	}

	switch (det_id) {
	case PTP_DET_CPU:
		det->MDES	= devinfo->CPU_MDES;
		det->BDES	= devinfo->CPU_BDES;
		det->DCMDET	= devinfo->CPU_DCMDET;
		det->DCBDET	= devinfo->CPU_DCBDET;
		// det->VBOOT	= PTP_VOLT_TO_PMIC_VAL(mt_cpufreq_cur_vproc(MT_CPU_DVFS_LITTLE));
		//lock_code = 0x1; //wrong EFUSE workaround
		if ((lock_code & 0x1) == 1)
			det->VMAX	= PTP_VOLT_TO_PMIC_VAL(115000); // override default setting
		det->VMIN	= PTP_VOLT_TO_PMIC_VAL(90000); // override default settingWW
		break;

	case PTP_DET_LTE:
		det->MDES	= devinfo->LTE_MDES;
		det->BDES	= devinfo->LTE_BDES;
		det->DCMDET	= devinfo->LTE_DCMDET;
		det->DCBDET	= devinfo->LTE_DCBDET;
		det->VCO	= VCO_VAL_LTE;
		det->DVTFIXED	= DVTFIXED_VAL_LTE; 
		// det->VBOOT	= MV_TO_VAL(mt_cpufreq_cur_vproc(MT_CPU_DVFS_BIG));
		det->VMIN	= PTP_VOLT_TO_PMIC_VAL(90000); // override default setting
		det->num_freq_tbl = 1;
		det->freq_tbl[0] = 300 * 100 / 300; // XXX: percentage, 300/300, dedicated for LTE only
		break;

	case PTP_DET_GPU:
		det->MDES	= devinfo->GPU_MDES;
		det->BDES	= devinfo->GPU_BDES;
		det->DCMDET	= devinfo->GPU_DCMDET;
		det->DCBDET	= devinfo->GPU_DCBDET;
		det->VMIN	= VMIN_VAL_GPU;// override default setting
		det->VCO	= VCO_VAL_GPU;
		det->DVTFIXED	= DVTFIXED_VAL_GPU; 
		// det->VBOOT	= PTP_VOLT_TO_PMIC_VAL(mt_gpufreq_get_cur_volt() / 100); // TODO: FIXME, unit = 10us
		break;

	case PTP_DET_SOC:
		det->MDES	= devinfo->SOC_MDES;
		det->BDES	= devinfo->SOC_BDES;
		det->DCMDET	= devinfo->SOC_DCMDET;
		det->DCBDET	= devinfo->SOC_DCBDET;
		det->VMIN	= VMIN_VAL_GPU; // override default setting
		det->VCO	= VCO_VAL_SOC;
		det->DVTFIXED	= DVTFIXED_VAL_SOC; 
		// det->VBOOT = XXX // TODO: FIXME
		/* get DVFS frequency table */
		det->num_freq_tbl = 3;
		det->freq_tbl[0] = 1600 * 100 / 1600; // XXX: percentage, 800/800, dedicated for VCORE only
		det->freq_tbl[1] = 1333 * 100 / 1600; // XXX: percentage, 600/800, dedicated for VCORE only
		det->freq_tbl[2] = 800 * 100 / 1600; // XXX: percentage, 600/800, dedicated for VCORE only
		break;

	default:
		ptp_error("[%s]: Unknown det_id %d\n", __func__, det_id);
		break;
	}

	switch (det->ctrl_id) {
	case PTP_CTRL_CPU:
		det->AGEDELTA	= devinfo->CPU_AGEDELTA;
		det->MTDES	= devinfo->CPU_MTDES;
		break;

	case PTP_CTRL_LTE:
		det->AGEDELTA	= devinfo->LTE_AGEDELTA;
		det->MTDES	= devinfo->LTE_MTDES;
		break;

	case PTP_CTRL_GPU:
		det->AGEDELTA	= devinfo->GPU_AGEDELTA;
		det->MTDES	= devinfo->GPU_MTDES;
		break;

	case PTP_CTRL_SOC:
		det->AGEDELTA	= devinfo->SOC_AGEDELTA;
		det->MTDES	= devinfo->SOC_MTDES;
		break;

	default:
		ptp_error("[%s]: Unknown ctrl_id %d\n", __func__, det->ctrl_id);
		break;
	}

	/* get DVFS frequency table */
	det->ops->get_freq_table(det);

	FUNC_EXIT(FUNC_LV_HELP);
}


static void ptp_set_ptp_volt(struct ptp_det *det)
{
#if SET_PMIC_VOLT
	int i, cur_temp, low_temp_offset;
	struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id);

	cur_temp = det->ops->get_temp(det);
	ptp_debug("ptp_set_ptp_volt cur_temp = %d\n", cur_temp);
	if (cur_temp <= 33000) {
		low_temp_offset = 10;
		ctrl->volt_update |= PTP_VOLT_UPDATE;
	} else {
		low_temp_offset = 0;
		ctrl->volt_update |= PTP_VOLT_UPDATE;
	}

	//all scale of volt_tbl_pmic,volt_tbl,volt_offset are pmic value
	//scale of det->volt_offset must equal 10uV
	for (i = 0; i < det->num_freq_tbl; i++)
		det->volt_tbl_pmic[i] = clamp(det->volt_tbl[i] + det->volt_offset + low_temp_offset, det->VMIN, det->VMAX);

#ifdef __KERNEL__
	wake_up_interruptible(&ctrl->wq);
#else//only for SLT
	/*if ((ctrl->volt_update & PTP_VOLT_UPDATE) && det->ops->set_volt)
			det->ops->set_volt(det);

	if ((ctrl->volt_update & PTP_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

	ctrl->volt_update = PTP_VOLT_NONE;*/
#endif
#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void ptp_restore_ptp_volt(struct ptp_det *det)
{
#if SET_PMIC_VOLT
	struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id);

	ctrl->volt_update |= PTP_VOLT_RESTORE;
#ifdef __KERNEL__
	wake_up_interruptible(&ctrl->wq);
#else
	/*if ((ctrl->volt_update & PTP_VOLT_UPDATE) && det->ops->set_volt)
			det->ops->set_volt(det);

	if ((ctrl->volt_update & PTP_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

	ctrl->volt_update = PTP_VOLT_NONE;*/
#endif
#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}
#if 0
static void mt_ptp_reg_dump_locked(void)
{
	unsigned long addr;
	
	for (addr = (unsigned long)PTP_DESCHAR; addr <= (unsigned long)PTP_SMSTATE1; addr += 4)
		ptp_isr_info("0x%lx = 0x%08X\n", addr, *(volatile unsigned int *)addr);

	addr = (unsigned long)PTP_PTPCORESEL;
	ptp_isr_info("0x%lx = 0x%08X\n", addr, *(volatile unsigned int *)addr);
}
#endif
static void mt_ptp_reg_dump(void)
{
	struct ptp_det *det;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	ptp_isr_info("PTP_REVISIONID	= 0x%08X\n", ptp_read(PTP_REVISIONID));
	ptp_isr_info("PTP_TEMPMONCTL0	= 0x%08X\n", ptp_read(PTP_TEMPMONCTL0));
	ptp_isr_info("PTP_TEMPMONCTL1	= 0x%08X\n", ptp_read(PTP_TEMPMONCTL1));
	ptp_isr_info("PTP_TEMPMONCTL2	= 0x%08X\n", ptp_read(PTP_TEMPMONCTL2));
	ptp_isr_info("PTP_TEMPMONINT	= 0x%08X\n", ptp_read(PTP_TEMPMONINT));
	ptp_isr_info("PTP_TEMPMONINTSTS	= 0x%08X\n", ptp_read(PTP_TEMPMONINTSTS));
	ptp_isr_info("PTP_TEMPMONIDET0	= 0x%08X\n", ptp_read(PTP_TEMPMONIDET0));
	ptp_isr_info("PTP_TEMPMONIDET1	= 0x%08X\n", ptp_read(PTP_TEMPMONIDET1));
	ptp_isr_info("PTP_TEMPMONIDET2	= 0x%08X\n", ptp_read(PTP_TEMPMONIDET2));
	ptp_isr_info("PTP_TEMPH2NTHRE	= 0x%08X\n", ptp_read(PTP_TEMPH2NTHRE));
	ptp_isr_info("PTP_TEMPHTHRE	= 0x%08X\n", ptp_read(PTP_TEMPHTHRE));
	ptp_isr_info("PTP_TEMPCTHRE	= 0x%08X\n", ptp_read(PTP_TEMPCTHRE));
	ptp_isr_info("PTP_TEMPOFFSETH	= 0x%08X\n", ptp_read(PTP_TEMPOFFSETH));
	ptp_isr_info("PTP_TEMPOFFSETL	= 0x%08X\n", ptp_read(PTP_TEMPOFFSETL));
	ptp_isr_info("PTP_TEMPMSRCTL0	= 0x%08X\n", ptp_read(PTP_TEMPMSRCTL0));
	ptp_isr_info("PTP_TEMPMSRCTL1	= 0x%08X\n", ptp_read(PTP_TEMPMSRCTL1));
	ptp_isr_info("PTP_TEMPAHBPOLL	= 0x%08X\n", ptp_read(PTP_TEMPAHBPOLL));
	ptp_isr_info("PTP_TEMPAHBTO	= 0x%08X\n", ptp_read(PTP_TEMPAHBTO));
	ptp_isr_info("PTP_TEMPADCPNP0	= 0x%08X\n", ptp_read(PTP_TEMPADCPNP0));
	ptp_isr_info("PTP_TEMPADCPNP1	= 0x%08X\n", ptp_read(PTP_TEMPADCPNP1));
	ptp_isr_info("PTP_TEMPADCPNP2	= 0x%08X\n", ptp_read(PTP_TEMPADCPNP2));
	ptp_isr_info("PTP_TEMPADCMUX	= 0x%08X\n", ptp_read(PTP_TEMPADCMUX));
	ptp_isr_info("PTP_TEMPADCEXT	= 0x%08X\n", ptp_read(PTP_TEMPADCEXT));
	ptp_isr_info("PTP_TEMPADCEXT1	= 0x%08X\n", ptp_read(PTP_TEMPADCEXT1));
	ptp_isr_info("PTP_TEMPADCEN	= 0x%08X\n", ptp_read(PTP_TEMPADCEN));
	ptp_isr_info("PTP_TEMPPNPMUXADDR	= 0x%08X\n", ptp_read(PTP_TEMPPNPMUXADDR));
	ptp_isr_info("PTP_TEMPADCMUXADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCMUXADDR));
	ptp_isr_info("PTP_TEMPADCEXTADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCEXTADDR));
	ptp_isr_info("PTP_TEMPADCEXT1ADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCEXT1ADDR));
	ptp_isr_info("PTP_TEMPADCENADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCENADDR));
	ptp_isr_info("PTP_TEMPADCVALIDADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCVALIDADDR));
	ptp_isr_info("PTP_TEMPADCVOLTADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCVOLTADDR));
	ptp_isr_info("PTP_TEMPRDCTRL	= 0x%08X\n", ptp_read(PTP_TEMPRDCTRL));
	ptp_isr_info("PTP_TEMPADCVALIDMASK	= 0x%08X\n", ptp_read(PTP_TEMPADCVALIDMASK));
	ptp_isr_info("PTP_TEMPADCVOLTAGESHIFT	= 0x%08X\n", ptp_read(PTP_TEMPADCVOLTAGESHIFT));
	ptp_isr_info("PTP_TEMPADCWRITECTRL	= 0x%08X\n", ptp_read(PTP_TEMPADCWRITECTRL));
	ptp_isr_info("PTP_TEMPMSR0	= 0x%08X\n", ptp_read(PTP_TEMPMSR0));
	ptp_isr_info("PTP_TEMPMSR1	= 0x%08X\n", ptp_read(PTP_TEMPMSR1));
	ptp_isr_info("PTP_TEMPMSR2	= 0x%08X\n", ptp_read(PTP_TEMPMSR2));
	ptp_isr_info("PTP_TEMPIMMD0	= 0x%08X\n", ptp_read(PTP_TEMPIMMD0));
	ptp_isr_info("PTP_TEMPIMMD1	= 0x%08X\n", ptp_read(PTP_TEMPIMMD1));
	ptp_isr_info("PTP_TEMPIMMD2	= 0x%08X\n", ptp_read(PTP_TEMPIMMD2));
	ptp_isr_info("PTP_TEMPMONIDET3	= 0x%08X\n", ptp_read(PTP_TEMPMONIDET3));
	ptp_isr_info("PTP_TEMPADCPNP3	= 0x%08X\n", ptp_read(PTP_TEMPADCPNP3));
	ptp_isr_info("PTP_TEMPMSR3	= 0x%08X\n", ptp_read(PTP_TEMPMSR3));
	ptp_isr_info("PTP_TEMPIMMD3	= 0x%08X\n", ptp_read(PTP_TEMPIMMD3));
	ptp_isr_info("PTP_TEMPPROTCTL	= 0x%08X\n", ptp_read(PTP_TEMPPROTCTL));
	ptp_isr_info("PTP_TEMPPROTTA	= 0x%08X\n", ptp_read(PTP_TEMPPROTTA));
	ptp_isr_info("PTP_TEMPPROTTB	= 0x%08X\n", ptp_read(PTP_TEMPPROTTB));
	ptp_isr_info("PTP_TEMPPROTTC	= 0x%08X\n", ptp_read(PTP_TEMPPROTTC));
	ptp_isr_info("PTP_TEMPSPARE0	= 0x%08X\n", ptp_read(PTP_TEMPSPARE0));
	ptp_isr_info("PTP_TEMPSPARE1	= 0x%08X\n", ptp_read(PTP_TEMPSPARE1));
	ptp_isr_info("PTP_TEMPSPARE2	= 0x%08X\n", ptp_read(PTP_TEMPSPARE2));
	ptp_isr_info("PTP_TEMPSPARE3	= 0x%08X\n", ptp_read(PTP_TEMPSPARE3));

	for_each_det(det) {
		mt_ptp_lock(&flags);
		det->ops->switch_bank(det);

		ptp_isr_info("PTP_DESCHAR[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DESCHAR));
		ptp_isr_info("PTP_TEMPCHAR[%s]	= 0x%08X\n", det->name, ptp_read(PTP_TEMPCHAR));
		ptp_isr_info("PTP_DETCHAR[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DETCHAR));
		ptp_isr_info("PTP_AGECHAR[%s]	= 0x%08X\n", det->name, ptp_read(PTP_AGECHAR));
		ptp_isr_info("PTP_DCCONFIG[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DCCONFIG));
		ptp_isr_info("PTP_AGECONFIG[%s]	= 0x%08X\n", det->name, ptp_read(PTP_AGECONFIG));
		ptp_isr_info("PTP_FREQPCT30[%s]	= 0x%08X\n", det->name, ptp_read(PTP_FREQPCT30));
		ptp_isr_info("PTP_FREQPCT74[%s]	= 0x%08X\n", det->name, ptp_read(PTP_FREQPCT74));
		ptp_isr_info("PTP_LIMITVALS[%s]	= 0x%08X\n", det->name, ptp_read(PTP_LIMITVALS));
		ptp_isr_info("PTP_VBOOT[%s]	= 0x%08X\n", det->name, ptp_read(PTP_VBOOT));
		ptp_isr_info("PTP_DETWINDOW[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DETWINDOW));
		ptp_isr_info("PTP_PTPCONFIG[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPCONFIG));
		ptp_isr_info("PTP_TSCALCS[%s]	= 0x%08X\n", det->name, ptp_read(PTP_TSCALCS));
		ptp_isr_info("PTP_RUNCONFIG[%s]	= 0x%08X\n", det->name, ptp_read(PTP_RUNCONFIG));
		ptp_isr_info("PTP_PTPEN[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPEN));
		ptp_isr_info("PTP_INIT2VALS[%s]	= 0x%08X\n", det->name, ptp_read(PTP_INIT2VALS));
		ptp_isr_info("PTP_DCVALUES[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DCVALUES));
		ptp_isr_info("PTP_AGEVALUES[%s]	= 0x%08X\n", det->name, ptp_read(PTP_AGEVALUES));
		ptp_isr_info("PTP_VOP30[%s]	= 0x%08X\n", det->name, ptp_read(PTP_VOP30));
		ptp_isr_info("PTP_VOP74[%s]	= 0x%08X\n", det->name, ptp_read(PTP_VOP74));
		ptp_isr_info("PTP_TEMP[%s]	= 0x%08X\n", det->name, ptp_read(PTP_TEMP));
		ptp_isr_info("PTP_PTPINTSTS[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPINTSTS));
		ptp_isr_info("PTP_PTPINTSTSRAW[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPINTSTSRAW));
		ptp_isr_info("PTP_PTPINTEN[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPINTEN));
		ptp_isr_info("PTP_SMSTATE0[%s]	= 0x%08X\n", det->name, ptp_read(PTP_SMSTATE0));
		ptp_isr_info("PTP_SMSTATE1[%s]	= 0x%08X\n", det->name, ptp_read(PTP_SMSTATE1));

		mt_ptp_unlock(&flags);
	}

	ptp_isr_info("PTP_PTPCORESEL	= 0x%08X\n", ptp_read(PTP_PTPCORESEL));
	ptp_isr_info("PTP_THERMINTST	= 0x%08X\n", ptp_read(PTP_THERMINTST));
	ptp_isr_info("PTP_PTPODINTST	= 0x%08X\n", ptp_read(PTP_PTPODINTST));
	ptp_isr_info("PTP_THSTAGE0ST	= 0x%08X\n", ptp_read(PTP_THSTAGE0ST));
	ptp_isr_info("PTP_THSTAGE1ST	= 0x%08X\n", ptp_read(PTP_THSTAGE1ST));
	ptp_isr_info("PTP_THSTAGE2ST	= 0x%08X\n", ptp_read(PTP_THSTAGE2ST));
	ptp_isr_info("PTP_THAHBST0	= 0x%08X\n", ptp_read(PTP_THAHBST0));
	ptp_isr_info("PTP_THAHBST1	= 0x%08X\n", ptp_read(PTP_THAHBST1));
	ptp_isr_info("PTP_PTPSPARE0	= 0x%08X\n", ptp_read(PTP_PTPSPARE0));
	ptp_isr_info("PTP_PTPSPARE1	= 0x%08X\n", ptp_read(PTP_PTPSPARE1));
	ptp_isr_info("PTP_PTPSPARE2	= 0x%08X\n", ptp_read(PTP_PTPSPARE2));
	ptp_isr_info("PTP_PTPSPARE3	= 0x%08X\n", ptp_read(PTP_PTPSPARE3));
	ptp_isr_info("PTP_THSLPEVEB	= 0x%08X\n", ptp_read(PTP_THSLPEVEB));

	FUNC_EXIT(FUNC_LV_HELP);
}

static inline void handle_init01_isr(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	ptp_isr_info("@ %s(%s)\n", __func__, det->name);

	det->dcvalues[PTP_PHASE_INIT01]		= ptp_read(PTP_DCVALUES);
	det->ptp_freqpct30[PTP_PHASE_INIT01]	= ptp_read(PTP_FREQPCT30);
	det->ptp_26c[PTP_PHASE_INIT01]		= ptp_read(PTP_PTPINTEN + 0x10);
	det->ptp_vop30[PTP_PHASE_INIT01]	= ptp_read(PTP_VOP30);
	det->ptp_ptpen[PTP_PHASE_INIT01]	= ptp_read(PTP_PTPEN);
#if DUMP_DATA_TO_DE
	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++)
		{
			det->reg_dump_data[i][PTP_PHASE_INIT01] = ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);
			ptp_isr_info("0x%lx === 0x%08x\n",
						   (unsigned long)PTP_BASEADDR + reg_dump_addr_off[i],
						   det->reg_dump_data[i][PTP_PHASE_INIT01]
						  );
		}
	}
#endif
	/*
	 * Read & store 16 bit values DCVALUES.DCVOFFSET and
	 * AGEVALUES.AGEVOFFSET for later use in INIT2 procedure
	 */
	det->DCVOFFSETIN = ~(ptp_read(PTP_DCVALUES) & 0xffff) + 1; /* hw bug, workaround */
	det->AGEVOFFSETIN = ptp_read(PTP_AGEVALUES) & 0xffff;

	/*
	 * Set PTPEN.PTPINITEN/PTPEN.PTPINIT2EN = 0x0 &
	 * Clear PTP INIT interrupt PTPINTSTS = 0x00000001
	 */
	ptp_write(PTP_PTPEN, 0x0);
	ptp_write(PTP_PTPINTSTS, 0x1);
	//ptp_init01_finish(det);
	det->ops->init02(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init02_isr(struct ptp_det *det)
{
	unsigned int temp;
	int i;
	//	struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id);

	FUNC_ENTER(FUNC_LV_LOCAL);

	ptp_isr_info("@ %s(%s)\n", __func__, det->name);
	det->dcvalues[PTP_PHASE_INIT02]		= ptp_read(PTP_DCVALUES);
	det->ptp_freqpct30[PTP_PHASE_INIT02]	= ptp_read(PTP_FREQPCT30);
	det->ptp_26c[PTP_PHASE_INIT02]		= ptp_read(PTP_PTPINTEN + 0x10);
	det->ptp_vop30[PTP_PHASE_INIT02]	= ptp_read(PTP_VOP30);
	det->ptp_ptpen[PTP_PHASE_INIT02]	= ptp_read(PTP_PTPEN);
#if DUMP_DATA_TO_DE
	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++)
		{
			det->reg_dump_data[i][PTP_PHASE_INIT02] = ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);
			ptp_isr_info("0x%lx === 0x%08x\n",
						   (unsigned long)PTP_BASEADDR + reg_dump_addr_off[i],
						   det->reg_dump_data[i][PTP_PHASE_INIT02]
						  );
		}
	}
#endif
	temp = ptp_read(PTP_VOP30);
	det->volt_tbl[0] = temp & 0xff;
	det->volt_tbl[1] = (temp >> 8) & 0xff;
	det->volt_tbl[2] = (temp >> 16) & 0xff;
	det->volt_tbl[3] = (temp >> 24) & 0xff;

	temp = ptp_read(PTP_VOP74);
	det->volt_tbl[4] = temp & 0xff;
	det->volt_tbl[5] = (temp >> 8) & 0xff;
	det->volt_tbl[6] = (temp >> 16) & 0xff;
	det->volt_tbl[7] = (temp >> 24) & 0xff;

	memcpy(det->volt_tbl_init2, det->volt_tbl, sizeof(det->volt_tbl_init2));

	for (i = 0; i < NR_FREQ; i++) {
#ifdef CONFIG_PTP_AEE_RR_REC
		switch (det->ctrl_id) {
			case PTP_CTRL_LTE:
				aee_rr_rec_ptp_cpu_big_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) | (aee_rr_curr_ptp_cpu_big_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
				break;
			default:
				break;
		}
#endif
		ptp_isr_info("ptp_detectors[%s].volt_tbl[%d] = 0x%08X (%d)\n",
			     det->name, i, det->volt_tbl[i], PTP_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));
	}
	ptp_isr_info("ptp_level = 0x%08X\n", ptp_level);

	ptp_set_ptp_volt(det);

	/*
	 * Set PTPEN.PTPINITEN/PTPEN.PTPINIT2EN = 0x0 &
	 * Clear PTP INIT interrupt PTPINTSTS = 0x00000001
	 */
	ptp_write(PTP_PTPEN, 0x0);
	ptp_write(PTP_PTPINTSTS, 0x1);

	//atomic_dec(&ctrl->in_init);
	//complete(&ctrl->init_done);
	det->ops->mon_mode(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init_err_isr(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);
	ptp_isr_info("====================================================\n");
	ptp_isr_info("PTP init err: PTPEN(%p) = 0x%08X, PTPINTSTS(%p) = 0x%08X\n",
		     PTP_PTPEN, ptp_read(PTP_PTPEN),
		     PTP_PTPINTSTS, ptp_read(PTP_PTPINTSTS));
	ptp_isr_info("PTP_SMSTATE0 (%p) = 0x%08X\n",
		     PTP_SMSTATE0, ptp_read(PTP_SMSTATE0));
	ptp_isr_info("PTP_SMSTATE1 (%p) = 0x%08X\n",
		     PTP_SMSTATE1, ptp_read(PTP_SMSTATE1));
	ptp_isr_info("====================================================\n");

	// TODO: FIXME
	{
		//struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id);
		//atomic_dec(&ctrl->in_init);
		//complete(&ctrl->init_done);
	}
	// TODO: FIXME

	det->ops->disable_locked(det, BY_INIT_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_mode_isr(struct ptp_det *det)
{
	unsigned int temp;
	int i;

	FUNC_ENTER(FUNC_LV_LOCAL);

	ptp_isr_info("@ %s(%s)\n", __func__, det->name);
	ptp_isr_info("ca7_temp=%d, c17_temp=%d gpu_temp=%d\n", tscpu_get_temp_by_bank(THERMAL_BANK0), tscpu_get_temp_by_bank(THERMAL_BANK1), tscpu_get_temp_by_bank(THERMAL_BANK2));

#ifdef CONFIG_PTP_AEE_RR_REC

	switch (det->ctrl_id) {
	case PTP_CTRL_CPU:
		if (tscpu_get_temp_by_bank(THERMAL_BANK0) != 0) {
			aee_rr_rec_ptp_temp((tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000) << (8 * PTP_CPU_LITTLE_IS_SET_VOLT) | (aee_rr_curr_ptp_temp() & ~(0xFF << (8 * PTP_CPU_LITTLE_IS_SET_VOLT))));
		}
		break;

	case PTP_CTRL_LTE:
		if (tscpu_get_temp_by_bank(THERMAL_BANK1) != 0) {
			aee_rr_rec_ptp_temp((tscpu_get_temp_by_bank(THERMAL_BANK1) / 1000) << (8 * PTP_CPU_BIG_IS_SET_VOLT) | (aee_rr_curr_ptp_temp() & ~(0xFF << (8 * PTP_CPU_BIG_IS_SET_VOLT))));
		}
		break;

	case PTP_CTRL_GPU:
		if (tscpu_get_temp_by_bank(THERMAL_BANK2) != 0) {
			aee_rr_rec_ptp_temp((tscpu_get_temp_by_bank(THERMAL_BANK2) / 1000) << (8 * PTP_GPU_IS_SET_VOLT) | (aee_rr_curr_ptp_temp() & ~(0xFF << (8 * PTP_GPU_IS_SET_VOLT))));
		}
		break;

	default:
		break;
	}

#endif
	det->dcvalues[PTP_PHASE_MON]		= ptp_read(PTP_DCVALUES);
	det->ptp_freqpct30[PTP_PHASE_MON]	= ptp_read(PTP_FREQPCT30);
	det->ptp_26c[PTP_PHASE_MON]		= ptp_read(PTP_PTPINTEN + 0x10);
	det->ptp_vop30[PTP_PHASE_MON]		= ptp_read(PTP_VOP30);
	det->ptp_ptpen[PTP_PHASE_MON]		= ptp_read(PTP_PTPEN);
#if DUMP_DATA_TO_DE
	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++)
		{
			det->reg_dump_data[i][PTP_PHASE_MON] = ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);
			ptp_isr_info("0x%lx === 0x%08x\n",
						   (unsigned long)PTP_BASEADDR + reg_dump_addr_off[i],
						   det->reg_dump_data[i][PTP_PHASE_MON]
						  );
		}
	}
#endif
	/* check if thermal sensor init completed? */
	temp = (ptp_read(PTP_TEMP) & 0xff);

	if ((temp > 0x4b) && (temp < 0xd3)) {
		ptp_isr_info("thermal sensor init has not been completed. "
			     "(temp = 0x%08X)\n", temp);
		goto out;
	}

	temp = ptp_read(PTP_VOP30);
	det->volt_tbl[0] = temp & 0xff;
	det->volt_tbl[1] = (temp >> 8) & 0xff;
	det->volt_tbl[2] = (temp >> 16) & 0xff;
	det->volt_tbl[3] = (temp >> 24) & 0xff;

	temp = ptp_read(PTP_VOP74);
	det->volt_tbl[4] = temp & 0xff;
	det->volt_tbl[5] = (temp >> 8) & 0xff;
	det->volt_tbl[6] = (temp >> 16) & 0xff;
	det->volt_tbl[7] = (temp >> 24) & 0xff;

	for (i = 0; i < NR_FREQ; i++) {
#ifdef CONFIG_PTP_AEE_RR_REC
		switch (det->ctrl_id) {
		case PTP_CTRL_CPU:
			aee_rr_rec_ptp_cpu_little_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) | (aee_rr_curr_ptp_cpu_little_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;

		case PTP_CTRL_GPU:
			aee_rr_rec_ptp_gpu_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) | (aee_rr_curr_ptp_gpu_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;

		default:
			break;
		}

#endif
		ptp_isr_info("ptp_detectors[%s].volt_tbl[%d] = 0x%08X (%d)\n",
			     det->name, i, det->volt_tbl[i], PTP_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));
	}
	ptp_isr_info("ptp_level = 0x%08X\n", ptp_level);
	//ptp_isr_info("ISR : TEMPSPARE1 = 0x%08X\n", ptp_read(TEMPSPARE1));
	ptp_set_ptp_volt(det);

out:
	/* Clear PTP INIT interrupt PTPINTSTS = 0x00ff0000 */
	ptp_write(PTP_PTPINTSTS, 0x00ff0000);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_err_isr(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	/* PTP Monitor mode error handler */
	ptp_isr_info("====================================================\n");
	ptp_isr_info("PTP mon err: PTPEN(%p) = 0x%08X, PTPINTSTS(%p) = 0x%08X\n",
		     PTP_PTPEN, ptp_read(PTP_PTPEN),
		     PTP_PTPINTSTS, ptp_read(PTP_PTPINTSTS));
	ptp_isr_info("PTP_SMSTATE0 (%p) = 0x%08X\n",
		     PTP_SMSTATE0, ptp_read(PTP_SMSTATE0));
	ptp_isr_info("PTP_SMSTATE1 (%p) = 0x%08X\n",
		     PTP_SMSTATE1, ptp_read(PTP_SMSTATE1));
	ptp_isr_info("PTP_TEMP (%p) = 0x%08X\n",
		     PTP_TEMP, ptp_read(PTP_TEMP));
	ptp_isr_info("PTP_TEMPMSR0 (%p) = 0x%08X\n",
		     PTP_TEMPMSR0, ptp_read(PTP_TEMPMSR0));
	ptp_isr_info("PTP_TEMPMSR1 (%p) = 0x%08X\n",
		     PTP_TEMPMSR1, ptp_read(PTP_TEMPMSR1));
	ptp_isr_info("PTP_TEMPMSR2 (%p) = 0x%08X\n",
		     PTP_TEMPMSR2, ptp_read(PTP_TEMPMSR2));
	ptp_isr_info("PTP_TEMPMONCTL0 (%p) = 0x%08X\n",
		     PTP_TEMPMONCTL0, ptp_read(PTP_TEMPMONCTL0));
	ptp_isr_info("PTP_TEMPMSRCTL1 (%p) = 0x%08X\n",
		     PTP_TEMPMSRCTL1, ptp_read(PTP_TEMPMSRCTL1));
	ptp_isr_info("====================================================\n");

	det->ops->disable_locked(det, BY_MON_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void ptp_isr_handler(struct ptp_det *det)
{
	unsigned int PTPINTSTS, PTPEN;

	FUNC_ENTER(FUNC_LV_LOCAL);

	PTPINTSTS = ptp_read(PTP_PTPINTSTS);
	PTPEN = ptp_read(PTP_PTPEN);

	ptp_isr_info("[%s]\n", det->name);
	ptp_isr_info("PTPINTSTS = 0x%08X\n", PTPINTSTS);
	ptp_isr_info("PTP_PTPEN = 0x%08X\n", PTPEN);
	ptp_isr_info("*(%p) = 0x%08X\n", PTP_DCVALUES, ptp_read(PTP_DCVALUES));
	ptp_isr_info("*(%p) = 0x%08X\n", PTP_AGECOUNT, ptp_read(PTP_AGECOUNT));

	if (PTPINTSTS == 0x1) { /* PTP init1 or init2 */
		if ((PTPEN & 0x7) == 0x1)   /* PTP init1 */
			handle_init01_isr(det);
		else if ((PTPEN & 0x7) == 0x5)   /* PTP init2 */
			handle_init02_isr(det);
		else {
			/*
			 * error : init1 or init2,
			 * but enable setting is wrong.
			 */
			handle_init_err_isr(det);
		}
	} else if ((PTPINTSTS & 0x00ff0000) != 0x0)
		handle_mon_mode_isr(det);
	else { /* PTP error handler */
		/* init 1  || init 2 error handler */
		if (((PTPEN & 0x7) == 0x1) || ((PTPEN & 0x7) == 0x5))
			handle_init_err_isr(det);
		else /* PTP Monitor mode error handler */
			handle_mon_err_isr(det);
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#ifdef __KERNEL__
static irqreturn_t ptp_isr(int irq, void *dev_id)
#else
int ptp_isr()
#endif
{
	unsigned long flags;
	struct ptp_det *det = NULL;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	// mt_ptp_reg_dump(); // TODO: FIXME, for temp reg dump <-XXX

	mt_ptp_lock(&flags);
#if 0

	if (!(BIT(PTP_CTRL_VCORE) & ptp_read(PTP_PTPODINTST))) {
		switch (ptp_read_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL)) {
		case SEL_VCORE_AO:
			det = &ptp_detectors[PTP_DET_VCORE_AO];
			break;

		case SEL_VCORE_PDN:
			det = &ptp_detectors[PTP_DET_VCORE_PDN];
			break;
		}

		if (likely(det)) {
			det->ops->switch_bank(det);
			ptp_isr_handler(det);
		}
	}

#endif

	for (i = 0; i < NR_PTP_CTRL; i++) {
		//		if (i == PTP_CTRL_VCORE)
		//			continue;

		if ((BIT(i) & ptp_read(PTP_PTPODINTST))) // TODO: FIXME, it is better to link i @ struct ptp_det
			continue;

		det = &ptp_detectors[i];

		det->ops->switch_bank(det);
		//mt_ptp_reg_dump_locked(); // TODO: FIXME, for temp reg dump <-XXX
		ptp_isr_handler(det);
	}

	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_MODULE);
#ifdef __KERNEL__
	return IRQ_HANDLED;
#else
	return 0;
#endif
}

#if 0
static atomic_t ptp_init01_cnt;
static void ptp_init01_prepare(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	atomic_inc(&ptp_init01_cnt);

	if (atomic_read(&ptp_init01_cnt) == 1) {
		enum mt_cpu_dvfs_id cpu;

		switch (det_to_id(det)) {
		case PTP_DET_LITTLE:
			cpu = MT_CPU_DVFS_LITTLE;
			break;

		case PTP_DET_BIG:
			cpu = MT_CPU_DVFS_BIG;
			break;

		default:
			return;
		}

#if 0	// TODO: move to ptp_init01()

		if (0 == cpu) { // TODO: FIXME, for E1
			/* disable frequency hopping (main PLL) */
			mt_fh_popod_save();
			/* disable DVFS and set vproc = 1.15v (1 GHz) */
			mt_cpufreq_disable_by_ptpod(cpu);
		}

#endif	// TODO: move to ptp_init01()
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static void ptp_init01_finish(struct ptp_det *det)
{
	atomic_dec(&ptp_init01_cnt);

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (atomic_read(&ptp_init01_cnt) < 0)
		BUG();

	if (atomic_read(&ptp_init01_cnt) == 0) {
#if 0
		enum mt_cpu_dvfs_id cpu;

		switch (det_to_id(det)) {
		case PTP_DET_LITTLE:
			cpu = MT_CPU_DVFS_LITTLE;
			break;

		case PTP_DET_BIG:
			cpu = MT_CPU_DVFS_BIG;
			break;

		default:
			return;
		}
#endif
#if 0	// TODO: move to ptp_init01()

		if (0 == cpu) { // TODO: FIXME, for E1
			/* enable DVFS */
			mt_cpufreq_enable_by_ptpod(cpu);
			/* enable frequency hopping (main PLL) */
			mt_fh_popod_restore();
		}

#endif	// TODO: move to ptp_init01()
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#endif

void ptp_init01(void)
{
	struct ptp_det *det;
	struct ptp_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		{
			unsigned long flag; // <-XXX
			unsigned int vboot;

			vboot = PTP_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));
			pr_notice("@%s(),vboot = %d,  det->VBOOT = %d\n", __func__, vboot, det->VBOOT);
#ifdef __KERNEL__

			if (vboot != det->VBOOT) {
				ptp_error("@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n", __func__, __LINE__, det->name, vboot, det->VBOOT);
				aee_kernel_warning("mt_ptp", "@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n", __func__, __LINE__, det->name, vboot, det->VBOOT);
			}

			BUG_ON(PTP_VOLT_TO_PMIC_VAL(det->ops->get_volt(det)) != det->VBOOT);
#endif
			mt_ptp_lock(&flag); // <-XXX
			det->ops->init01(det);
			mt_ptp_unlock(&flag); // <-XXX
		}

		/*
		 * VCORE_AO and VCORE_PDN use the same controller.
		 * Wait until VCORE_AO init01 and init02 done
		 */
		//if (atomic_read(&ctrl->in_init)) {
		/* TODO: Use workqueue to avoid blocking */
		//	wait_for_completion(&ctrl->init_done);
		//}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void ptp_init02(void)
{
	struct ptp_det *det;
	struct ptp_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		if (HAS_FEATURE(det, FEA_MON)) {
			unsigned long flag;

			mt_ptp_lock(&flag);
			det->ops->init02(det);
			mt_ptp_unlock(&flag);
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

#if EN_PTP_OD
#if 0
static char *readline(struct file *fp)
{
#define BUFSIZE 1024
	static char buf[BUFSIZE]; // TODO: FIXME, dynamic alloc
	static int buf_end = 0;
	static int line_start = 0;
	static int line_end = 0;
	char *ret;

	FUNC_ENTER(FUNC_LV_HELP);
empty:

	if (line_start >= buf_end) {
		line_start = 0;
		buf_end = fp->f_op->read(fp, &buf[line_end], sizeof(buf) - line_end, &fp->f_pos);

		if (0 == buf_end) {
			line_end = 0;
			return NULL;
		}

		buf_end += line_end;
	}

	while (buf[line_end] != '\n') {
		line_end++;

		if (line_end >= buf_end) {
			memcpy(&buf[0], &buf[line_start], buf_end - line_start);
			line_end = buf_end - line_start;
			line_start = buf_end;
			goto empty;
		}
	}

	buf[line_end] = '\0';
	ret = &buf[line_start];
	line_start = line_end + 1;

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}
#endif
/* leakage */
unsigned int leakage_core;
unsigned int leakage_gpu;
unsigned int leakage_sram2;
unsigned int leakage_sram1;


void get_devinfo(struct ptp_devinfo *p)
{
	int *val = (int *)p;

	FUNC_ENTER(FUNC_LV_HELP);

	/* TODO: Read devinfo from file system? */
#if 0 /* FIXME: ned get_devinfo_with_index() ready */
	{
#define PTP_DEVINFO_FILEPATH "/system/20140115_ptpod_csv.csv"

		struct file *fp;

		fp = filp_open(PTP_DEVINFO_FILEPATH, O_RDONLY, 0644);

		if (IS_ERR(fp)) {
			int i;

			val[4] = get_devinfo_with_index(15); /* M_HW_RES4 */
			val[5] = get_devinfo_with_index(16); /* M_HW_RES5 */

			/* search table (temp) */
			for (i = 0; i < ARRAY_SIZE(devinfo); i++) {
				if (val[4] == devinfo[i].M_HW_RES4 && val[5] == devinfo[i].M_HW_RES5) {
					val[0] = devinfo[i].M_HW_RES0;		/* M_HW_RES0 */
					val[1] = devinfo[i].M_HW_RES1;		/* M_HW_RES1 */
					val[2] = get_devinfo_with_index(9);	/* M_HW_RES2 */
					val[3] = devinfo[i].M_HW_RES3;		/* M_HW_RES3 */
					val[6] = devinfo[i].M_HW_RES6;		/* M_HW_RES6 */
					val[7] = devinfo[i].M_HW_RES7;		/* M_HW_RES7 */
					val[8] = devinfo[i].M_HW_RES8;		/* M_HW_RES8 */
					val[9] = devinfo[i].M_HW_RES9;		/* M_HW_RES9 */
					leakage_core  = devinfo[i].core;
					leakage_gpu   = devinfo[i].gpu;
					leakage_sram2 = devinfo[i].sram2;
					leakage_sram1 = devinfo[i].sram1;
#if 1 //disable PTPOD here TODO: FIXME
					p->PTPINITEN = 0; // TODO: FIXME
					p->PTPMONEN  = 0; // TODO: FIXME
#else
					p->PTPINITEN = 1; // TODO: FIXME
					p->PTPMONEN  = 1; // TODO: FIXME
#endif
					break;
				}
			}

			/* get efuse */
			if (i >= ARRAY_SIZE(devinfo)) {
				val[0] = get_devinfo_with_index(7); // ptp_read(0xF0206100);	/* M_HW_RES0 */
				val[1] = get_devinfo_with_index(8); // ptp_read(0xF0206104);	/* M_HW_RES1 */
				val[2] = get_devinfo_with_index(9); // ptp_read(0xF0206108);	/* M_HW_RES2 */
				val[3] = get_devinfo_with_index(14); // ptp_read(0xF0206170);	/* M_HW_RES3 */
				val[4] = get_devinfo_with_index(15); // ptp_read(0xF0206174);	/* M_HW_RES4 */
				val[5] = get_devinfo_with_index(16); // ptp_read(0xF0206178);	/* M_HW_RES5 */
				val[6] = get_devinfo_with_index(17); // ptp_read(0xF020617C);	/* M_HW_RES6 */
				val[7] = get_devinfo_with_index(18); // ptp_read(0xF0206180);	/* M_HW_RES7 */
				val[8] = get_devinfo_with_index(19); // ptp_read(0xF0206184);	/* M_HW_RES8 */
				val[9] = get_devinfo_with_index(21); // ptp_read(0xF0206188);	/* M_HW_RES9 */
			}

			for (i = 0; i < sizeof(struct ptp_devinfo) / sizeof(unsigned long); i++)
				ptp_notice("M_HW_RES%d\t= 0x%08X\n", i, val[i]);
		} else {
			char *line;
			int ser_num;

			/* skip two lines */
			readline(fp);
			readline(fp);

			while (NULL != (line = readline(fp))) {
				sscanf(line, "%d,%d,%d,0X%08X,0X%08X,0X%08X,0X%08X,0X%08X",
				       &ser_num, &val[4], &val[5], &val[0], &val[1], &val[7], &val[8], &val[9]
				      );

				if (val[4] == get_devinfo_with_index(15) && val[5] == get_devinfo_with_index(16))
					break;
			}
		}
	}
#else

#ifdef __KERNEL__
	val[0] = get_devinfo_with_index(7); // ptp_read(0xF0206100); /* M_HW_RES0 */
	val[1] = get_devinfo_with_index(8); // ptp_read(0xF0206104); /* M_HW_RES1 */
	val[2] = get_devinfo_with_index(9); // ptp_read(0xF0206108); /* M_HW_RES2 */
	val[3] = get_devinfo_with_index(14); // ptp_read(0xF0206170); /* M_HW_RES3 */
	val[4] = get_devinfo_with_index(15); // ptp_read(0xF0206174); /* M_HW_RES4 */
	val[5] = get_devinfo_with_index(16); // ptp_read(0xF0206178); /* M_HW_RES5 */
	val[6] = get_devinfo_with_index(17); // ptp_read(0xF020617C); /* M_HW_RES6 */
	val[7] = get_devinfo_with_index(18); // ptp_read(0xF0206180); /* M_HW_RES7 */
	val[8] = get_devinfo_with_index(19); // ptp_read(0xF0206184); /* M_HW_RES8 */
	val[9] = get_devinfo_with_index(21); // ptp_read(0xF0206188); /* M_HW_RES9 */
#else
	val[0] = ptp_read(0x10206100); /* PTP_OD0 */
	val[1] = ptp_read(0x10206104); /* PTP_OD1 */
	val[2] = ptp_read(0x10206108); /* PTP_OD2 */
	val[3] = ptp_read(0x10206170); /* PTP_OD3 */
	val[4] = ptp_read(0x10206174); /* PTP_OD4 */
	val[5] = ptp_read(0x10206178); /* PTP_OD5 */
	val[6] = ptp_read(0x1020617C); /* PTP_OD6 */
	val[7] = ptp_read(0x10206180); /* PTP_OD7 */
	val[8] = ptp_read(0x10206184); /* PTP_OD7 */
	val[9] = ptp_read(0x10206188); /* PTP_OD7 */
#endif
	/*
	val[0] = 0xFFFFFFFF;
	val[1] = 0xFFFFFFFF;
	val[2] = 0x15A62B14;
	val[3] = 0x007E0090;
	val[4] = 0x14B5202C;
	val[5] = 0x00547074;
	val[6] = 0x14B23F19;
	val[7] = 0x000000DE;
	*/





	ptp_crit("val[0]=0x%x\n", val[0]);
	ptp_crit("val[1]=0x%x\n", val[1]);
	ptp_crit("val[2]=0x%x\n", val[2]);
	ptp_crit("val[3]=0x%x\n", val[3]);
	ptp_crit("val[4]=0x%x\n", val[4]);
	ptp_crit("val[5]=0x%x\n", val[5]);
	ptp_crit("val[6]=0x%x\n", val[6]);
	ptp_crit("val[7]=0x%x\n", val[7]);
	ptp_crit("val[7]=0x%x\n", val[8]);
	ptp_crit("val[7]=0x%x\n", val[9]);
	ptp_crit("p->PTPINITEN=0x%x\n", p->PTPINITEN);
	ptp_crit("p->PTPMONEN=0x%x\n", p->PTPMONEN);
	//p->PTPINITEN = 0; // TODO: FIXME
	//p->PTPMONEN  = 0; // TODO: FIXME
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static int ptp_probe(struct platform_device *pdev)
{
	int ret;
	struct ptp_det *det;
	struct ptp_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_MODULE);

	/* enable thermal CG */
	enable_clock(MT_CG_PERI_THERM, "PTPOD");
#ifdef __KERNEL__
	/* set PTP IRQ */
	ret = request_irq(ptpod_irq_number, ptp_isr, IRQF_TRIGGER_LOW, "ptp", NULL);

	if (ret) {
		ptp_notice("PTP IRQ register failed (%d)\n", ret);
		WARN_ON(1);
	}

#endif
	ptp_notice("Set PTP IRQ OK.\n");

	//ptp_level = mt_ptp_get_level();
	//atomic_set(&ptp_init01_cnt, 0);
	
#ifdef CONFIG_PTP_AEE_RR_REC
	_mt_ptp_aee_init();
#endif

	for_each_ctrl(ctrl) {
		ptp_init_ctrl(ctrl);
	}

	/* disable frequency hopping (main PLL) */
	mt_fh_popod_save();
	/* disable DVFS and set vproc = 1.15v (1 GHz) */
	mt_cpufreq_disable_by_ptpod(MT_CPU_DVFS_LITTLE);
	mt_gpufreq_disable_by_ptpod();
#ifdef __KERNEL__
	enable_clock(MT_CG_MFG_AXI, "PTP");
	enable_clock(MT_CG_MFG_G3D, "PTP");
#endif

#ifdef __KERNEL__
	{
		extern unsigned int ckgen_meter(int val);
		pr_notice("@%s(), hf_faxi_ck = %d, hd_faxi_ck = %d\n", __func__, ckgen_meter(1), ckgen_meter(2));
	}
#endif
	ptp_data[0] = 0xffffffff;

	for_each_det(det) {
		ptp_init_det(det, &ptp_devinfo);
	}
#ifdef __KERNEL__
	ptp_init01();
#endif
	ptp_data[0] = 0;
#ifdef __KERNEL__
	{
		extern unsigned int ckgen_meter(int val);
		pr_notice("@%s(), hf_faxi_ck = %d, hd_faxi_ck = %d\n", __func__, ckgen_meter(1), ckgen_meter(2));
	}
#endif

#ifdef __KERNEL__
	disable_clock(MT_CG_MFG_AXI, "PTP");
	disable_clock(MT_CG_MFG_G3D, "PTP");
#endif
	/* enable DVFS */
	mt_gpufreq_enable_by_ptpod();
	mt_cpufreq_enable_by_ptpod(MT_CPU_DVFS_LITTLE);
	/* enable frequency hopping (main PLL) */
	mt_fh_popod_restore();

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int ptp_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	kthread_stop(ptp_volt_thread);
	*/
	//struct ptp_det *det = id_to_ptp_det(PTP_DET_LTE);
	FUNC_ENTER(FUNC_LV_MODULE);
	/*memcpy(det->volt_tbl, det->volt_tbl_init2, sizeof(det->volt_tbl_init2));
	ptp_set_ptp_volt(det);
	FUNC_EXIT(FUNC_LV_MODULE);*/

	return 0;
}

static int ptp_resume(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);
	ptp_init02();
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_ptpod_of_match[] = {
	{ .compatible = "mediatek,PTP_FSM", },
	{},
};
#endif

static struct platform_driver ptp_driver = {
	.remove     = NULL,
	.shutdown   = NULL,
	.probe      = ptp_probe,
	.suspend    = ptp_suspend,
	.resume     = ptp_resume,
	.driver     = {
		.name   = "mt-ptp",
#ifdef CONFIG_OF
		.of_match_table = mt_ptpod_of_match,
#endif
	},
};

#ifndef __KERNEL__
/*
 * For CTP
 */
int mt_ptp_probe(void)
{
	dbg_print("CTP - In mt_ptp_probe\n");
	ptp_init();
	ptp_probe(NULL);
	return 0;
}

void ptp_init01_ctp(int id)
{
	struct ptp_det *det;
	struct ptp_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		{
			unsigned long flag; // <-XXX

			if ((ctrl->det_id == id) || (id == PTP_CTRL_SOC + 1)) {
				pr_notice("CTP - (%d:%d)\n", PTP_VOLT_TO_PMIC_VAL(det->ops->get_volt(det)), det->VBOOT);

				mt_ptp_lock(&flag); // <-XXX
				det->ops->init01(det);
				mt_ptp_unlock(&flag); // <-XXX
			}
		}
	}
	FUNC_EXIT(FUNC_LV_LOCAL);
}

void ptp_init01_ptp(int id)
{
#if 0
	/* disable frequency hopping (main PLL) */
	mt_fh_popod_save();

	/* disable DVFS and set vproc = 1.15v (1 GHz) */
	mt_cpufreq_disable_by_ptpod(MT_CPU_DVFS_LITTLE);
	mt_gpufreq_disable_by_ptpod();
#endif
	ptp_init01_ctp(id);
#if 0
	/* enable DVFS */
	mt_gpufreq_enable_by_ptpod();
	mt_cpufreq_enable_by_ptpod(MT_CPU_DVFS_LITTLE); // TODO: FIXME, for E1

	/* enable frequency hopping (main PLL) */
	mt_fh_popod_restore();
#endif
}

#endif

int mt_ptp_opp_num(ptp_det_id id)
{
	struct ptp_det *det = id_to_ptp_det(id);

	FUNC_ENTER(FUNC_LV_API);
	FUNC_EXIT(FUNC_LV_API);

	return det->num_freq_tbl;
}
EXPORT_SYMBOL(mt_ptp_opp_num);

void mt_ptp_opp_freq(ptp_det_id id, unsigned int *freq)
{
	struct ptp_det *det = id_to_ptp_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

	for (i = 0; i < det->num_freq_tbl; i++)
		freq[i] = det->freq_tbl[i];

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_ptp_opp_freq);

void mt_ptp_opp_status(ptp_det_id id, unsigned int *temp, unsigned int *volt)
{
	struct ptp_det *det = id_to_ptp_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

	/* TODO: FIXME */
#ifdef __KERNEL__
	*temp = tscpu_get_temp_by_bank((id == PTP_DET_CPU) ? THERMAL_BANK0 : 
										(id == PTP_DET_LTE) ? THERMAL_BANK1 : 
										(id == PTP_DET_GPU) ? THERMAL_BANK2 : 
										THERMAL_BANK3);
#else
	*temp = 0;
#endif

	for (i = 0; i < det->num_freq_tbl; i++)
		volt[i] = PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_ptp_opp_status);

/***************************
* return current PTP stauts
****************************/
int mt_ptp_status(ptp_det_id id)
{
	struct ptp_det *det = id_to_ptp_det(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(!det);
	BUG_ON(!det->ops);
	BUG_ON(!det->ops->get_status);

	FUNC_EXIT(FUNC_LV_API);

	return det->ops->get_status(det);
}
#ifdef CONFIG_PROC_FS
/**
 * ===============================================
 * PROCFS interface for debugging
 * ===============================================
 */

/*
 * show current PTP stauts
 */
static int ptp_debug_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det = (struct ptp_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	/* FIXME: PTPEN sometimes is disabled temp */
	seq_printf(m, "PTPOD[%s] %s (ptp_level = 0x%08X)\n",
		   det->name,
		   det->ops->get_status(det) ?
		   "enabled" : "disable",
		   ptp_level);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set PTP status by procfs interface
 */
static ssize_t ptp_debug_proc_write(struct file *file,
				    const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	int enabled = 0;
	char *buf = (char *) __get_free_page(GFP_USER);
	struct ptp_det *det = (struct ptp_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (sscanf(buf, "%d", &enabled) == 1) {
		ret = 0;

		if (1 == enabled)
			; // det->ops->enable(det, BY_PROCFS); // TODO: FIXME, kernel panic when enabled
		else
			det->ops->disable(det, BY_PROCFS);
	} else
		ret = -EINVAL;

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * show current PTP data
 */
static int ptp_dump_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det;
	int *val = (int *)&ptp_devinfo;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	// ptp_detectors[PTP_DET_LITTLE].ops->dump_status(&ptp_detectors[PTP_DET_LITTLE]); // <-XXX
	// ptp_detectors[PTP_DET_BIG].ops->dump_status(&ptp_detectors[PTP_DET_BIG]); // <-XXX

	mt_ptp_reg_dump();

	for (i = 0; i < 2 * sizeof(struct ptp_devinfo) / sizeof(unsigned long); i++)
		seq_printf(m, "M_HW_RES%d\t= 0x%08X\n", i, val[i]);

	//seq_printf(m, "det->PTPMONEN= 0x%08X,det->PTPINITEN= 0x%08X\n", det->PTPMONEN, det->PTPINITEN);
	/*
		seq_printf(m, "leakage_core\t= %d\n"
			   "leakage_gpu\t= %d\n"
			   "leakage_little\t= %d\n"
			   "leakage_big\t= %d\n",
			   leakage_core,
			   leakage_gpu,
			   leakage_sram2,
			   leakage_sram1
			   );
	*/
	for_each_det(det) {

		seq_printf(m, "PTP_DCVALUES[%s]\t= 0x%08X\n", det->name, det->VBOOT);

		for (i = PTP_PHASE_INIT01; i < NR_PTP_PHASE; i++) {

			seq_printf(m, "dcvalues=0x%08X, ptp_freqpct30=0x%08X, ptp_26c=0x%08X, ptp_vop30=0x%08X,ptp_ptpen= 0x%08X\n",
				   det->dcvalues[i],
				   det->ptp_freqpct30[i],
				   det->ptp_26c[i],
				   det->ptp_vop30[i],
				   det->ptp_ptpen[i]
				  );
#if DUMP_DATA_TO_DE
			{
				int j;

				for (j = 0; j < ARRAY_SIZE(reg_dump_addr_off); j++)
					seq_printf(m, "0x%lx === 0x%08x\n",
						   (unsigned long)PTP_BASEADDR + reg_dump_addr_off[j],
						   det->reg_dump_data[j][i]
						  );
			}
#endif
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * show current voltage
 */
static int ptp_cur_volt_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det = (struct ptp_det *)m->private;
	u32 rdata = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	rdata = det->ops->get_volt(det);

	if (rdata != 0)
		seq_printf(m, "%d\n", rdata);
	else
		seq_printf(m, "PTPOD[%s] read current voltage fail\n", det->name);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
#define CPU_TURBO_VOLT_DIFF_1 2500
#define CPU_TURBO_VOLT_DIFF_2 1250
#define VPROC_MAX 115000
#define NUM_OF_CPU_FREQ_TABLE 8
#define NUM_OF_LTE_FREQ_TABLE 1
#define NUM_OF_GPU_FREQ_TABLE 5

static int volt_to_buck_volt(u32 cur_volt, int need)
{
	if (need) {
		if (cur_volt >= VPROC_MAX)
			return VPROC_MAX;
		else {
			if (cur_volt % 1000 == 0)
				return cur_volt;
		else
			return (cur_volt + 1000) / 1000 * 1000;
	}
	}
	else
		return cur_volt;
}

static int ptpod_not_work(u32 cur_volt, unsigned int *volt_tbl_pmic, int num)
{
	int i;
	int need = 0;

	if (num == NUM_OF_CPU_FREQ_TABLE) {
		need = 1;
		ptp_isr_info("CPU: In ptpod_not_work, cur_volt = %d -> %d\n", cur_volt,  PTP_VOLT_TO_PMIC_VAL(cur_volt));
	}
	else if (num == NUM_OF_LTE_FREQ_TABLE)
		ptp_isr_info("LTE: In ptpod_not_work, cur_volt = %d -> %d\n", cur_volt,  PTP_VOLT_TO_PMIC_VAL(cur_volt));
	else
		ptp_isr_info("GPU: In ptpod_not_work, cur_volt = %d -> %d\n", cur_volt,  PTP_VOLT_TO_PMIC_VAL(cur_volt));

	for (i = 0; i < num; i++)
	{
		ptp_isr_info("volt_tbl_pmic[%d] = %d => %d\n", i, volt_tbl_pmic[i], volt_to_buck_volt(PTP_PMIC_VAL_TO_VOLT(volt_tbl_pmic[i]), need));
		if (cur_volt == volt_to_buck_volt(PTP_PMIC_VAL_TO_VOLT(volt_tbl_pmic[i]), need))
			return 0;
	}
		
	if ((cur_volt == volt_to_buck_volt((PTP_PMIC_VAL_TO_VOLT(volt_tbl_pmic[0]) + CPU_TURBO_VOLT_DIFF_1), need)) || (cur_volt == volt_to_buck_volt((PTP_PMIC_VAL_TO_VOLT(volt_tbl_pmic[0]) + CPU_TURBO_VOLT_DIFF_2), need)))
	{
		ptp_isr_info("%d, %d, %d\n", cur_volt, volt_to_buck_volt((PTP_PMIC_VAL_TO_VOLT(volt_tbl_pmic[0]) + CPU_TURBO_VOLT_DIFF_1), need), volt_to_buck_volt((PTP_PMIC_VAL_TO_VOLT(volt_tbl_pmic[0]) + CPU_TURBO_VOLT_DIFF_2), need));
		return 0;
	}

	return 1;
}

static int ptp_stress_result_proc_show(struct seq_file *m, void *v)
{
	unsigned int result = 0;
	struct ptp_det *det;
	u32 rdata = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_det(det) {
		rdata = det->ops->get_volt(det);
		switch (det->ctrl_id) {
			case PTP_CTRL_CPU:
				result |= (ptpod_not_work(rdata, det->volt_tbl_pmic, NUM_OF_CPU_FREQ_TABLE) << PTP_CTRL_CPU);
				if ((result & (1 << PTP_CTRL_CPU)) != 0)
					ptp_isr_info("CPU PTP fail\n");
				break;
				
			case PTP_CTRL_LTE:
				result |= (ptpod_not_work(rdata, det->volt_tbl_pmic, NUM_OF_LTE_FREQ_TABLE) << PTP_CTRL_LTE);
				if ((result & (1 << PTP_CTRL_LTE)) != 0)
					ptp_isr_info("LTE PTP fail\n");
				break;
				
			case PTP_CTRL_GPU:
				result |= (ptpod_not_work(rdata, det->volt_tbl_pmic, NUM_OF_GPU_FREQ_TABLE) << PTP_CTRL_GPU);
				if ((result & (1 << PTP_CTRL_GPU)) != 0)
					ptp_isr_info("GPU PTP fail\n");
				break;

			default:
				break;
		}
	}
	seq_printf(m, "0x%X\n", result);
	return 0;
}
/*
 * show current PTP status
 */
static int ptp_status_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det = (struct ptp_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "PTP_LOG: PTPOD[%s] (%d) - "
		   "(%d, %d, %d, %d, %d, %d, %d, %d) - "
		   "(%d, %d, %d, %d, %d, %d, %d, %d)\n",
		   det->name, det->ops->get_temp(det),
		   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
		   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
		   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
		   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
		   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
		   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
		   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
		   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
		   det->freq_tbl[0],
		   det->freq_tbl[1],
		   det->freq_tbl[2],
		   det->freq_tbl[3],
		   det->freq_tbl[4],
		   det->freq_tbl[5],
		   det->freq_tbl[6],
		   det->freq_tbl[7]);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set PTP log enable by procfs interface
 */
static int ptp_log_en = 0;

static int ptp_log_en_proc_show(struct seq_file *m, void *v)
{
	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%d\n", ptp_log_en);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t ptp_log_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	ret = -EINVAL;

	if (sscanf(buf, "%d", &ptp_log_en) != 1) {
		ptp_notice("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (ptp_log_en) {
	case 0:
		ptp_notice("ptp log disabled.\n");
		hrtimer_cancel(&ptp_log_timer);
		break;

	case 1:
		ptp_notice("ptp log enabled.\n");
		hrtimer_start(&ptp_log_timer, ns_to_ktime(LOG_INTERVAL), HRTIMER_MODE_REL);
		break;

	default:
		ptp_error("bad argument!! Should be \"0\" or \"1\"\n");
		ret = -EINVAL;
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}


/*
 * show PTP offset
 */
static int ptp_offset_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det = (struct ptp_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "%d\n", det->volt_offset);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set PTP offset by procfs
 */
static ssize_t ptp_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	int offset = 0;
	struct ptp_det *det = (struct ptp_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (sscanf(buf, "%d", &offset) == 1) {
		ret = 0;
		det->volt_offset = offset;
		ptp_set_ptp_volt(det);
	} else {
		ret = -EINVAL;
		ptp_notice("bad argument_1!! argument should be \"0\"\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(ptp_debug);
PROC_FOPS_RO(ptp_dump);
PROC_FOPS_RO(ptp_stress_result);
PROC_FOPS_RW(ptp_log_en);
PROC_FOPS_RO(ptp_status);
PROC_FOPS_RO(ptp_cur_volt);
PROC_FOPS_RW(ptp_offset);

static int create_procfs(void)
{
	struct proc_dir_entry *ptp_dir = NULL;
	struct proc_dir_entry *det_dir = NULL;
	int i;
	struct ptp_det *det;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry det_entries[] = {
		PROC_ENTRY(ptp_debug),
		PROC_ENTRY(ptp_status),
		PROC_ENTRY(ptp_cur_volt),
		PROC_ENTRY(ptp_offset),
	};

	struct pentry ptp_entries[] = {
		PROC_ENTRY(ptp_dump),
		PROC_ENTRY(ptp_log_en),
		PROC_ENTRY(ptp_stress_result),
	};

	FUNC_ENTER(FUNC_LV_HELP);

	ptp_dir = proc_mkdir("ptp", NULL);

	if (!ptp_dir) {
		ptp_error("[%s]: mkdir /proc/ptp failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(ptp_entries); i++) {
		if (!proc_create(ptp_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, ptp_dir, ptp_entries[i].fops)) {
			ptp_error("[%s]: create /proc/ptp/%s failed\n", __func__, ptp_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}

	for_each_det(det) {
		det_dir = proc_mkdir(det->name, ptp_dir);

		if (!det_dir) {
			ptp_error("[%s]: mkdir /proc/ptp/%s failed\n", __func__, det->name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -2;
		}

		for (i = 0; i < ARRAY_SIZE(det_entries); i++) {
			if (!proc_create_data(det_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, det_dir, det_entries[i].fops, det)) {
				ptp_error("[%s]: create /proc/ptp/%s/%s failed\n", __func__, det->name, det_entries[i].name);
				FUNC_EXIT(FUNC_LV_HELP);
				return -3;
			}
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}
#endif

int get_ptpod_status(void)
{
	get_devinfo(&ptp_devinfo);

	return ptp_devinfo.PTPINITEN;

}
EXPORT_SYMBOL(get_ptpod_status);

#define VCORE_VOLT_0 1125000
#define VCORE_VOLT_1 1000000
unsigned int vcore1;
unsigned int vcore2;

static int __init dt_get_ptp_devinfo(unsigned long node, const char *uname, int depth, void *data)
{
    struct devinfo_ptp_tag *tags;
    unsigned long size = 0 ;

    if (depth != 1 ||(strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
        return 0;
        
    tags = (struct devinfo_ptp_tag *) of_get_flat_dt_prop(node, "atag,ptp", &size);
    
    if(tags){
        vcore1 = tags->volt1;
        vcore2 = tags->volt2;
		pr_notice("[PTP][VCORE] - Kernel Got from DT (0x%0X, 0x%0X)\n", vcore1, vcore2);
    }
    return 1;	
}

unsigned int get_vcore_ptp_volt(int uv)
{
	unsigned int ret;
	switch (uv) {
		case VCORE_VOLT_0:
			ret = vcore1;
			break;
#if 0
		case VCORE_VOLT_1:
			ret = vcore2;
			break;
#endif
		default:
			ret = PTP_VOLT_TO_PMIC_VAL(uv/10);
			break;
	}

	if (ret == 0)
		ret = PTP_VOLT_TO_PMIC_VAL(uv/10);

	return ret;
}

static int __init vcore_ptp_init(void)
{
	of_scan_flat_dt(dt_get_ptp_devinfo, NULL);

	return 0;
}



#ifdef __KERNEL__
/*
 * Module driver
 */
static int __init ptp_init(void)
#else
int __init ptp_init(void)
#endif
{
	int err = 0;
#if 1
	struct device_node *node = NULL;
#endif
	FUNC_ENTER(FUNC_LV_MODULE);

#if 1
	node = of_find_compatible_node(NULL, NULL, "mediatek,PTP_FSM");

	if (node) {
		/* Setup IO addresses */
		ptpod_base = of_iomap(node, 0);
		pr_notice("[PTPOD] ptpod_base = 0x%p\n", ptpod_base);
	}

	/*get ptpod irq num*/
	ptpod_irq_number = irq_of_parse_and_map(node, 0);
	if (!ptpod_irq_number) {
		ptp_notice("[PTPOD] get irqnr failed=0x%d\n", ptpod_irq_number);
		return 0;
	}

#endif
	get_devinfo(&ptp_devinfo);

	if (0 == ptp_devinfo.PTPINITEN) {
		ptp_notice("PTPINITEN = 0x%08X\n", ptp_devinfo.PTPINITEN);
		FUNC_EXIT(FUNC_LV_MODULE);
		return 0;
	}

#ifdef __KERNEL__
	/*
	 * init timer for log / volt
	 */
	hrtimer_init(&ptp_log_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL); // <-XXX
	ptp_log_timer.function = ptp_log_timer_func; // <-XXX

	create_procfs();
#endif

	/*
	 * reg platform device driver
	 */
	err = platform_driver_register(&ptp_driver);

	if (err) {
		ptp_notice("PTP driver callback register failed..\n");
		FUNC_EXIT(FUNC_LV_MODULE);
		return err;
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static void __exit ptp_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);
	ptp_notice("PTP de-initialization\n");
	FUNC_EXIT(FUNC_LV_MODULE);
}

/* TODO: FIXME, disable for bring up */
arch_initcall(vcore_ptp_init);
late_initcall(ptp_init);
#endif

MODULE_DESCRIPTION("MediaTek PTPOD Driver v0.3");
MODULE_LICENSE("GPL");

#undef __MT_PTP_C__

