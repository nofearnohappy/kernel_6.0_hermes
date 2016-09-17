#ifdef BUILD_UBOOT
#define ENABLE_DSI_INTERRUPT 0

#include <asm/arch/disp_drv_platform.h>
#else
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/mutex.h>

#include "cmdq_record.h"
#include <disp_drv_log.h>
#endif
#include <debug.h>

#include "mach/mt_typedefs.h"
#include <mach/sync_write.h>
#include <mach/mt_clkmgr.h>

#include <mach/irqs.h>

#include <linux/xlog.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "mtkfb.h"
#include "ddp_drv.h"
#include "ddp_hal.h"
#include "ddp_manager.h"
#include "ddp_dpi_reg.h"
#include "ddp_dpi.h"
#include "ddp_reg.h"
#include "ddp_log.h"

#define LOG_TAG "DPI"
#define ENABLE_DPI_INTERRUPT        0
/* #define DIABLE_CLOCK_API */

#define DPI_SMT
static int cache_bg_parameter;


#undef LCD_BASE
#define LCD_BASE (0xF4024000)
#define DPI_REG_OFFSET(r)       offsetof(DPI_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))


#ifdef INREG32
#undef INREG32
#define INREG32(x)          (__raw_readl((unsigned long *)(x)))
#endif

/*
#if 0
static int dpi_reg_op_debug = 0;

#define DPI_OUTREG32(cmdq, addr, val) \
	{\
		if (dpi_reg_op_debug) \
			printk("[dsi/reg]0x%08x=0x%08x, cmdq:0x%08x\n", addr, val, cmdq);\
		if (cmdq) \
			cmdqRecWrite(cmdq, (unsigned int)(addr)&0x1fffffff, val, ~0); \
		else \
			mt65xx_reg_sync_writel(val, addr);}
#else
#define DPI_OUTREG32(cmdq, addr, val) \
	{\
	mt_reg_sync_writel(val, addr);}
		//mt65xx_reg_sync_writel(val, addr);}

#endif
*/
#define DPI_OUTREG32(cmdq, addr, val)           DISP_REG_SET(cmdq, addr, val)
#define DPI_OUTREGBIT(cmdq, TYPE, REG, bit, value)  \
	do {\
		TYPE r;\
		TYPE v;\
		if (cmdq) { \
			*(unsigned int *)(&r) = ((unsigned int)0x00000000); r.bit = ~(r.bit);\
			*(unsigned int *)(&v) = ((unsigned int)0x00000000); v.bit = value; \
			DISP_REG_MASK(cmdq, &REG, AS_UINT32(&v), AS_UINT32(&r)); \
		} else {\
			mt_reg_sync_writel(INREG32(&REG), &r); r.bit = (value); \
			DISP_REG_SET(cmdq, &REG, INREG32(&r));\
		}				\
	} while (0)
#define DPI_MASKREG32(cmdq, REG, MASK, VALUE)	DISP_REG_MASK((cmdq), (REG), (VALUE), (MASK));


#define DISP_LOG_PRINT(level, sub_module, fmt, arg...)  xlog_printk(level, "DISP/"sub_module, fmt, ##arg);


static BOOL s_isDpiPowerOn = FALSE;
static BOOL s_isDpiStart = FALSE;
static BOOL s_isDpiConfig = FALSE;

static int dpi_vsync_irq_count;
static int dpi_undflow_irq_count;

static DPI_REGS regBackup;
PDPI_REGS DPI_REG = 0;

static const LCM_UTIL_FUNCS lcm_utils_dpi;

static void (*dpiIntCallback) (DISP_INTERRUPT_EVENTS);

const UINT32 BACKUP_DPI_REG_OFFSETS[] = {
	DPI_REG_OFFSET(INT_ENABLE),
	DPI_REG_OFFSET(CNTL),
	DPI_REG_OFFSET(SIZE),

	DPI_REG_OFFSET(TGEN_HWIDTH),
	DPI_REG_OFFSET(TGEN_HPORCH),
	DPI_REG_OFFSET(TGEN_VWIDTH_LODD),
	DPI_REG_OFFSET(TGEN_VPORCH_LODD),

	DPI_REG_OFFSET(BG_HCNTL),
	DPI_REG_OFFSET(BG_VCNTL),
	DPI_REG_OFFSET(BG_COLOR),

	DPI_REG_OFFSET(TGEN_VWIDTH_LEVEN),
	DPI_REG_OFFSET(TGEN_VPORCH_LEVEN),
	DPI_REG_OFFSET(TGEN_VWIDTH_RODD),

	DPI_REG_OFFSET(TGEN_VPORCH_RODD),
	DPI_REG_OFFSET(TGEN_VWIDTH_REVEN),

	DPI_REG_OFFSET(TGEN_VPORCH_REVEN),
	DPI_REG_OFFSET(ESAV_VTIM_LOAD),
	DPI_REG_OFFSET(ESAV_VTIM_ROAD),
	DPI_REG_OFFSET(ESAV_FTIM),
};

/*the static functions declare*/
static void lcm_udelay(UINT32 us)
{
	udelay(us);
}

static void lcm_mdelay(UINT32 ms)
{
	msleep(ms);
}

static void lcm_set_reset_pin(UINT32 value)
{
#ifndef DPI_SMT
	DPI_OUTREG32(0, MMSYS_CONFIG_BASE + 0x150, value);
#endif
}

static void lcm_send_cmd(UINT32 cmd)
{
#ifndef DPI_SMT
	DPI_OUTREG32(0, LCD_BASE + 0x0F80, cmd);
#endif
}

static void lcm_send_data(UINT32 data)
{
#ifndef DPI_SMT
	DPI_OUTREG32(0, LCD_BASE + 0x0F90, data);
#endif
}

static void _BackupDPIRegisters(void)
{
	UINT32 i;
	DPI_REGS *reg = &regBackup;

	for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++i)
		DPI_OUTREG32(0, REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i]),
			     AS_UINT32(REG_ADDR(DPI_REG, BACKUP_DPI_REG_OFFSETS[i])));
}

static void _RestoreDPIRegisters(void)
{
	UINT32 i;
	DPI_REGS *reg = &regBackup;

	for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++i)
		DPI_OUTREG32(0, REG_ADDR(DPI_REG, BACKUP_DPI_REG_OFFSETS[i]),
			     AS_UINT32(REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i])));
}

/*the fuctions declare*/
/*DPI clock setting - use TVDPLL provide DPI clock*/
DPI_STATUS ddp_dpi_ConfigPclk(cmdqRecHandle cmdq, unsigned int mipi_pll_clk_ref, DPI_POLARITY polarity)
{
	UINT32 clksrc = 0;
	UINT32 posdiv = 0xc0000101;
	UINT32 prediv = 0x80112276;
	int type = 2;

	DPI_REG_OUTPUT_SETTING ctrl = DPI_REG->OUTPUT_SETTING;

	if (mipi_pll_clk_ref == 27027) {
		clksrc = 5;
		posdiv = 0xc0000101;
		prediv = 0x800C7958;
	} else if (mipi_pll_clk_ref == 74250)
		clksrc = 2;
	else if (mipi_pll_clk_ref == 148500)
		clksrc = 1;

	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_ConfigPclk clk %d , clksrc: %d\n", mipi_pll_clk_ref, clksrc);

#if 1
	clkmux_sel(MT_MUX_DPI0, clksrc, "DPI");
	/* /OUTREG32(CKSYS_BASE+0xa0, 0x01010102);   ///CKSYS_BASE 1:1080p@60, 2: 720p@60/1080@30 */
	OUTREG32(TVDPLL_CON0, posdiv);
	OUTREG32(TVDPLL_CON1, prediv);

#else
	DPI_OUTREG32(cmdq, CKSYS_BASE + 0xa0, 0x01010102);	/* /CKSYS_BASE */

	UINT32 temp = INREG32(0xf0209F00);
	DPI_OUTREG32(cmdq, 0xf0209F00, temp & 0xffffff7f);
	DPI_OUTREG32(cmdq, 0xf020927c, 2);
	DPI_OUTREG32(cmdq, 0xf020927c, 3);
	DPI_OUTREG32(cmdq, 0xf020927c, 1);

	DPI_OUTREG32(cmdq, 0xf0209270, 0xc0000100);
	DPI_OUTREG32(cmdq, 0xf0209270, 0xc0000101);
	DPI_OUTREG32(cmdq, 0xf0209274, INREG32(0xf0209274));	/* /0x114ec4  0x8a762 */
	DPI_OUTREG32(cmdq, 0xf0209274, 0x80112276);	/* /0x114ec4  0x8008a762 */
#endif

	/* /MASKREG32(0xf0005d50, 0x700, type <<8); */
	/* /MASKREG32(0xf0005d60, 0x700, type <<8); */
	/* /MASKREG32(0xf0005d70, 0x700, type <<8); */

	/*DPI output clock polarity */
	ctrl.CLK_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	DPI_OUTREGBIT(cmdq, DPI_REG_OUTPUT_SETTING, DPI_REG->OUTPUT_SETTING, CLK_POL, ctrl.CLK_POL);

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigDE(cmdqRecHandle cmdq, DPI_POLARITY polarity)
{
	DPI_REG_OUTPUT_SETTING pol = DPI_REG->OUTPUT_SETTING;

	pol.DE_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	DPI_OUTREGBIT(cmdq, DPI_REG_OUTPUT_SETTING, DPI_REG->OUTPUT_SETTING, DE_POL, pol.DE_POL);

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigVsync(cmdqRecHandle cmdq, DPI_POLARITY polarity, UINT32 pulseWidth,
			       UINT32 backPorch, UINT32 frontPorch)
{
	DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd = DPI_REG->TGEN_VWIDTH_LODD;
	DPI_REG_TGEN_VPORCH_LODD vporch_lodd = DPI_REG->TGEN_VPORCH_LODD;
	DPI_REG_OUTPUT_SETTING pol = DPI_REG->OUTPUT_SETTING;
	DPI_REG_CNTL VS = DPI_REG->CNTL;

	pol.VSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	vwidth_lodd.VPW_LODD = pulseWidth;
	vporch_lodd.VBP_LODD = backPorch;
	vporch_lodd.VFP_LODD = frontPorch;

	VS.VS_LODD_EN = 1;
	VS.VS_LEVEN_EN = 0;
	VS.VS_RODD_EN = 0;
	VS.VS_REVEN_EN = 0;

	DPI_OUTREGBIT(cmdq, DPI_REG_OUTPUT_SETTING, DPI_REG->OUTPUT_SETTING, VSYNC_POL,
		      pol.VSYNC_POL);

	DPI_OUTREGBIT(cmdq, DPI_REG_TGEN_VWIDTH_LODD, DPI_REG->TGEN_VWIDTH_LODD, VPW_LODD,
		      vwidth_lodd.VPW_LODD);


	DPI_OUTREGBIT(cmdq, DPI_REG_TGEN_VPORCH_LODD, DPI_REG->TGEN_VPORCH_LODD, VBP_LODD,
		      vporch_lodd.VBP_LODD);
	DPI_OUTREGBIT(cmdq, DPI_REG_TGEN_VPORCH_LODD, DPI_REG->TGEN_VPORCH_LODD, VFP_LODD,
		      vporch_lodd.VFP_LODD);

	DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG->CNTL, VS_LODD_EN, VS.VS_LODD_EN);
	DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG->CNTL, VS_LEVEN_EN, VS.VS_LEVEN_EN);
	DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG->CNTL, VS_RODD_EN, VS.VS_RODD_EN);
	DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG->CNTL, VS_REVEN_EN, VS.VS_REVEN_EN);


	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigHsync(cmdqRecHandle cmdq, DPI_POLARITY polarity, UINT32 pulseWidth,
			       UINT32 backPorch, UINT32 frontPorch)
{
	DPI_REG_TGEN_HPORCH hporch = DPI_REG->TGEN_HPORCH;
	DPI_REG_OUTPUT_SETTING pol = DPI_REG->OUTPUT_SETTING;

	hporch.HBP = backPorch;
	hporch.HFP = frontPorch;
	pol.HSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	DPI_REG->TGEN_HWIDTH = pulseWidth;

	DPI_OUTREG32(cmdq, &DPI_REG->TGEN_HWIDTH, pulseWidth);

	DPI_OUTREGBIT(cmdq, DPI_REG_TGEN_HPORCH, DPI_REG->TGEN_HPORCH, HBP, hporch.HBP);
	DPI_OUTREGBIT(cmdq, DPI_REG_TGEN_HPORCH, DPI_REG->TGEN_HPORCH, HFP, hporch.HFP);

	DPI_OUTREGBIT(cmdq, DPI_REG_OUTPUT_SETTING, DPI_REG->OUTPUT_SETTING, HSYNC_POL,
		      pol.HSYNC_POL);

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigDualEdge(cmdqRecHandle cmdq, bool enable, UINT32 mode)
{
	DPI_REG_OUTPUT_SETTING ctrl = DPI_REG->OUTPUT_SETTING;
	DPI_REG_DDR_SETTING ddr_setting = DPI_REG->DDR_SETTING;

	ctrl.DUAL_EDGE_SEL = enable;
	DPI_OUTREGBIT(cmdq, DPI_REG_OUTPUT_SETTING, DPI_REG->OUTPUT_SETTING, DUAL_EDGE_SEL,
		      ctrl.DUAL_EDGE_SEL);

	ddr_setting.DDR_4PHASE = 1;
	ddr_setting.DDR_EN = 1;
	DPI_OUTREGBIT(cmdq, DPI_REG_DDR_SETTING, DPI_REG->DDR_SETTING, DDR_4PHASE,
		      ddr_setting.DDR_4PHASE);
	DPI_OUTREGBIT(cmdq, DPI_REG_DDR_SETTING, DPI_REG->DDR_SETTING, DDR_EN, ddr_setting.DDR_EN);

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigBG(cmdqRecHandle cmdq, bool enable, int BG_W, int BG_H)
{
	if (enable == false) {
		DPI_REG_CNTL pol = DPI_REG->CNTL;
		pol.BG_EN = 0;
		DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG->CNTL, BG_EN, pol.BG_EN);

	} else {		/* if(BG_W || BG_H) */

		DPI_REG_CNTL pol = DPI_REG->CNTL;
		pol.BG_EN = 1;
		DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG->CNTL, BG_EN, pol.BG_EN);

		DPI_REG_BG_HCNTL pol2 = DPI_REG->BG_HCNTL;
		pol2.BG_RIGHT = BG_W / 2;
		pol2.BG_LEFT = BG_W - pol2.BG_RIGHT;
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_HCNTL, DPI_REG->BG_HCNTL, BG_RIGHT, pol2.BG_RIGHT);
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_HCNTL, DPI_REG->BG_HCNTL, BG_LEFT, pol2.BG_LEFT);

		DPI_REG_BG_VCNTL pol3 = DPI_REG->BG_VCNTL;
		pol3.BG_BOT = BG_H / 2;
		pol3.BG_TOP = BG_H - pol3.BG_BOT;
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_VCNTL, DPI_REG->BG_VCNTL, BG_BOT, pol3.BG_BOT);
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_VCNTL, DPI_REG->BG_VCNTL, BG_TOP, pol3.BG_TOP);

		DPI_REG_BG_COLOR pol4 = DPI_REG->BG_COLOR;
		pol4.BG_B = 0;
		pol4.BG_G = 0;
		pol4.BG_R = 0;
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_COLOR, DPI_REG->BG_COLOR, BG_B, pol4.BG_B);
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_COLOR, DPI_REG->BG_COLOR, BG_G, pol4.BG_G);
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_COLOR, DPI_REG->BG_COLOR, BG_R, pol4.BG_R);
	}

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigSize(cmdqRecHandle cmdq, UINT32 width, UINT32 height)
{
	DPI_REG_SIZE size = DPI_REG->SIZE;
	size.WIDTH = width;
	size.HEIGHT = height;
	DPI_OUTREGBIT(cmdq, DPI_REG_SIZE, DPI_REG->SIZE, WIDTH, size.WIDTH);
	DPI_OUTREGBIT(cmdq, DPI_REG_SIZE, DPI_REG->SIZE, HEIGHT, size.HEIGHT);

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_EnableColorBar(void)
{
	/*enable internal pattern - color bar */
	DPI_OUTREG32(0, DISPSYS_DPI_BASE + 0xF00, 0x41);

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_EnableINT(cmdqRecHandle cmdq, UINT32 irq_bit, UINT32 value)
{
	/*enable internal pattern - color bar*/
	DPI_REG_INTERRUPT enInt = DPI_REG->INT_ENABLE;
	if (irq_bit&0x1) {
		enInt.VSYNC = value;
		DPI_OUTREGBIT(cmdq, DPI_REG_INTERRUPT, DPI_REG->INT_ENABLE, VSYNC, enInt.VSYNC);
	}

	if (irq_bit & 0x2) {
		enInt.VDE = value;
		DPI_OUTREGBIT(cmdq, DPI_REG_INTERRUPT, DPI_REG->INT_ENABLE, VDE, enInt.VDE);
	}

	return DPI_STATUS_OK;
}

int ddp_dpi_power_on(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	int ret = 0;
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_power_on, s_isDpiPowerOn %d\n",
		       s_isDpiPowerOn);
	if (!s_isDpiPowerOn) {
#ifndef DIABLE_CLOCK_API
		ret += enable_clock(MT_CG_DISP1_DPI_PIXEL, "DPI");
		ret += enable_clock(MT_CG_DISP1_DPI_ENGINE, "DPI");
#endif
		if (ret > 0) {
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI",
				       "power manager API return FALSE\n");
		}
		/* _RestoreDPIRegisters(); */
		s_isDpiPowerOn = TRUE;
	}

	return 0;
}

int ddp_dpi_power_off(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	int ret = 0;
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_power_off, s_isDpiPowerOn %d\n",
		       s_isDpiPowerOn);
	if (s_isDpiPowerOn) {
#ifndef DIABLE_CLOCK_API
		/* _BackupDPIRegisters(); */
		ret += disable_clock(MT_CG_DISP1_DPI_PIXEL, "DPI");
		ret += disable_clock(MT_CG_DISP1_DPI_ENGINE, "DPI");
#endif
		if (ret > 0) {
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI",
				       "power manager API return FALSE\n");
		}
		s_isDpiPowerOn = FALSE;
	}

	return 0;

}

int ddp_dpi_config(DISP_MODULE_ENUM module, disp_ddp_path_config *config, void *cmdq_handle)
{
	if (s_isDpiConfig == FALSE) {
		LCM_DPI_PARAMS *dpi_config = &(config->dispif_config.dpi);
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_config DPI status:%x, cmdq:%x\n",
			       INREG32(&DPI_REG->STATUS), cmdq_handle);

		ddp_dpi_ConfigPclk(cmdq_handle, dpi_config->dpi_clock, dpi_config->clk_pol);
		ddp_dpi_ConfigSize(cmdq_handle, dpi_config->width, dpi_config->height);
		ddp_dpi_ConfigBG(cmdq_handle, true, dpi_config->bg_width, dpi_config->bg_height);
		cache_bg_parameter = dpi_config->bg_width << 16 | dpi_config->bg_height;
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI",
			       "dpi_config->bg_width: %d, dpi_config->bg_height: %d\n",
			       dpi_config->bg_width, dpi_config->bg_height);

		ddp_dpi_ConfigDE(cmdq_handle, dpi_config->de_pol);
		ddp_dpi_ConfigVsync(cmdq_handle, dpi_config->vsync_pol,
				    dpi_config->vsync_pulse_width, dpi_config->vsync_back_porch,
				    dpi_config->vsync_front_porch);
		ddp_dpi_ConfigHsync(cmdq_handle, dpi_config->hsync_pol,
				    dpi_config->hsync_pulse_width, dpi_config->hsync_back_porch,
				    dpi_config->hsync_front_porch);

		ddp_dpi_ConfigDualEdge(cmdq_handle, dpi_config->i2x_en, dpi_config->i2x_edge);

		s_isDpiConfig = TRUE;
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_config done\n");
	}

	if (s_isDpiConfig == TRUE) {
		LCM_DPI_PARAMS *dpi_config = &(config->dispif_config.dpi);
		int now_bg_parameters = dpi_config->bg_width << 16 | dpi_config->bg_height;
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI",
			       "now_bg_parameters: 0x%x, cache_bg_parameter: 0x%x\n",
			       now_bg_parameters, cache_bg_parameter);

		if (now_bg_parameters != cache_bg_parameter) {
			DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "Need to rechange DPI BG\n");

			/* DPI_OUTREG32(cmdq_handle, DISPSYS_DPI_BASE, 0x00000000); */
			/* DISP_REG_CMDQ_POLLING(cmdq_handle, DISPSYS_DPI_BASE+0x40, 0, 0x00000001); */
			ddp_dpi_ConfigSize(cmdq_handle, dpi_config->width, dpi_config->height);
			ddp_dpi_ConfigBG(cmdq_handle, true, dpi_config->bg_width,
					 dpi_config->bg_height);

			/* ddp_dpi_reset(module, cmdq_handle); */
			/* DPI_OUTREG32(cmdq_handle, DISPSYS_DPI_BASE, 0x00000001); */

			cache_bg_parameter = now_bg_parameters;
		}
/* ddp_dpi_dump(DISP_MODULE_DPI, 1); */
	}

	return 0;
}

int ddp_dpi_reset(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_reset\n");

	DPI_REG_RST reset = DPI_REG->DPI_RST;
	reset.RST = 1;
	DPI_OUTREGBIT(cmdq_handle, DPI_REG_RST, DPI_REG->DPI_RST, RST, reset.RST);


	reset.RST = 0;
	DPI_OUTREGBIT(cmdq_handle, DPI_REG_RST, DPI_REG->DPI_RST, RST, reset.RST);

	return 0;
}

int ddp_dpi_start(DISP_MODULE_ENUM module, void *cmdq)
{
	return 0;
}

int ddp_dpi_trigger(DISP_MODULE_ENUM module, void *cmdq)
{
	if (s_isDpiStart == FALSE) {
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_start\n");
		ddp_dpi_EnableINT(cmdq, 1, 1);	/*enable vsync interrupt */
		ddp_dpi_reset(module, cmdq);
		/*enable DPI */
		DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE, 0x00000001);

		s_isDpiStart = TRUE;
	}
	return 0;
}

int ddp_dpi_stop(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_stop\n");

	/*disable DPI and background, and reset DPI */
	ddp_dpi_EnableINT(cmdq_handle, 1, 0);
	DPI_OUTREG32(cmdq_handle, DISPSYS_DPI_BASE, 0x00000000);
	ddp_dpi_ConfigBG(cmdq_handle, false, 0, 0);
	ddp_dpi_reset(module, cmdq_handle);

	s_isDpiStart = FALSE;
	s_isDpiConfig = FALSE;
	dpi_vsync_irq_count = 0;
	dpi_undflow_irq_count = 0;

	return 0;
}

int ddp_dpi_is_busy(DISP_MODULE_ENUM module)
{
	unsigned int status = INREG32(DISPSYS_DPI_BASE + 0x40);

	return status & (0x1 << 16) ? 1 : 0;
}

int ddp_dpi_is_idle(DISP_MODULE_ENUM module)
{
	return !ddp_dpi_is_busy(module);
}

#if ENABLE_DPI_INTERRUPT
static irqreturn_t _DPI_InterruptHandler(int irq, void *dev_id)
{
	static int counter;
	DPI_REG_INTERRUPT status = DPI_REG->INT_STATUS;

	if (status.VSYNC) {
		dpi_vsync_irq_count++;
		if (dpi_vsync_irq_count > 30) {
			DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "dpi vsync\n");
			dpi_vsync_irq_count = 0;
		}

		if (counter) {
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "[Error] DPI FIFO is empty,
				       received % d times interrupt !!!\n", counter);
			counter = 0;
		}
	}

	DPI_OUTREG32(0, &DPI_REG->INT_STATUS, 0);

	return IRQ_HANDLED;
}
#endif

int ddp_dpi_init(DISP_MODULE_ENUM module, void *cmdq)
{
	DPI_REG = (PDPI_REGS) (DDP_REG_BASE_DPI);
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_init -  % x\n", cmdq);

	UINT32 temp = 0x200;

#if 0				/* /def MTKFB_FPGA_ONLY */

	DPI_OUTREG32(cmdq, MMSYS_CONFIG_BASE + 0x108, 0xffffffff);
	DPI_OUTREG32(cmdq, MMSYS_CONFIG_BASE + 0x118, 0xffffffff);
	/* DPI_OUTREG32(cmdq,  MMSYS_CONFIG_BASE+0xC08, 0xffffffff); */

	DPI_OUTREG32(cmdq, LCD_BASE + 0x001C, 0x00ffffff);
	DPI_OUTREG32(cmdq, LCD_BASE + 0x0028, 0x010000C0);
	DPI_OUTREG32(cmdq, LCD_BASE + 0x002C, 0x1);
	DPI_OUTREG32(cmdq, LCD_BASE + 0x002C, 0x0);

	DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE + 0x14, 0x00000000);
	DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE + 0x1C, 0x00000005);

	DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE + 0x20, 0x0000001A);
	DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE + 0x24, 0x001A001A);
	DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE + 0x28, 0x0000000A);
	DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE + 0x2C, 0x000A000A);
	DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE + 0x08, 0x00000007);

	DPI_OUTREG32(cmdq, DISPSYS_DPI_BASE + 0x00, 0x00000000);
#else
	/* /_BackupDPIRegisters(); */
	ddp_dpi_power_on(DISP_MODULE_DPI, cmdq);
#endif

#if ENABLE_DPI_INTERRUPT
	if (request_irq(DPI0_IRQ_BIT_ID,
			_DPI_InterruptHandler, IRQF_TRIGGER_LOW, "mtkdpi", NULL) < 0) {
		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI", "[ERROR] fail to request DPI irq\n");
		return DPI_STATUS_ERROR;
	}

	DPI_REG_INTERRUPT enInt = DPI_REG->INT_ENABLE;
	enInt.VSYNC = 1;
	DPI_OUTREGBIT(cmdq, DPI_REG_INTERRUPT, DPI_REG->INT_ENABLE, VSYNC, enInt.VSYNC);
#endif
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_init done % x\n", cmdq);

	return 0;
}

int ddp_dpi_deinit(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_deinit -  % x\n", cmdq_handle);
	ddp_dpi_stop(DISP_MODULE_DPI, cmdq_handle);
	ddp_dpi_power_off(DISP_MODULE_DPI, cmdq_handle);

	return 0;
}

int ddp_dpi_set_lcm_utils(DISP_MODULE_ENUM module, LCM_DRIVER *lcm_drv)
{
	DISPFUNC();
	LCM_UTIL_FUNCS *utils = NULL;

	if (lcm_drv == NULL) {
		DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "lcm_drv is null !\n");
		return -1;
	}

	utils = &lcm_utils_dpi;

	utils->set_reset_pin = lcm_set_reset_pin;
	utils->udelay = lcm_udelay;
	utils->mdelay = lcm_mdelay;
	utils->send_cmd = lcm_send_cmd,
	    utils->send_data = lcm_send_data, lcm_drv->set_util_funcs(utils);

	return 0;
}

int ddp_dpi_build_cmdq(DISP_MODULE_ENUM module, void *cmdq_trigger_handle, CMDQ_STATE state)
{
	return 0;
}

int ddp_dpi_dump(DISP_MODULE_ENUM module, int level)
{
	UINT32 i;
	DDPDUMP("---------- Start dump DPI registers ----------\n");

	for (i = 0; i <= 0x40; i += 4)
		DDPDUMP("DPI+%04x : 0x%08x\n", i, INREG32(DISPSYS_DPI_BASE + i));

	for (i = 0x68; i <= 0x7C; i += 4)
		DDPDUMP("DPI+%04x : 0x%08x\n", i, INREG32(DISPSYS_DPI_BASE + i));

	DDPDUMP("DPI+Color Bar : 0x%04x : 0x%08x\n", 0xF00, INREG32(DISPSYS_DPI_BASE + 0xF00));
#if 0

	DDPDUMP("DPI Addr 0xF00000A0 : 0x%08x\n", INREG32(0xF00000A0));
	DDPDUMP("DPI Addr 0xF4000108 : 0x%08x\n", INREG32(0xF4000108));
	DDPDUMP("DPI Addr 0xF4000110 : 0x%08x\n", INREG32(0xF4000110));
	DDPDUMP("DPI Addr 0xF4000118 : 0x%08x\n", INREG32(0xF4000118));
	DDPDUMP("DPI Addr 0xf0209270 : 0x%08x\n", INREG32(0xf0209270));
	DDPDUMP("DPI Addr 0xf0209274 : 0x%08x\n", INREG32(0xf0209274));
	DDPDUMP("DPI Addr 0xf0209278 : 0x%08x\n", INREG32(0xf0209278));
	DDPDUMP("DPI Addr 0xf020927c : 0x%08x\n", INREG32(0xf020927c));
	DDPDUMP("DPI Addr 0xf0209F00 : 0x%08x\n", INREG32(0xf0209F00));
	DDPDUMP("DPI Addr 0xf0206040 : 0x%08x\n", INREG32(0xf0206040));

	DDPDUMP("DPI Addr IO Driving : 0x%08x\n", INREG32(DISPSYS_IO_DRIVING));
	DDPDUMP("DPI TVDPLL CON0 : 0x%08x\n", INREG32(DDP_REG_TVDPLL_CON0));
	DDPDUMP("DPI TVDPLL CON1 : 0x%08x\n", INREG32(DDP_REG_TVDPLL_CON1));
	DDPDUMP("DPI TVDPLL CON6 : 0x%08x\n", INREG32(DDP_REG_TVDPLL_CON6));
	DDPDUMP("DPI MMSYS_CG_CON1:0x%08x\n", INREG32(DISP_REG_CONFIG_MMSYS_CG_CON1));
#endif
	/*DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI+", "DPI Addr 0xf0005d50 : 0x%08x\n",  INREG32(0xf0005d50)); */
	/*DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI+", "DPI Addr 0xf0005d60 : 0x%08x\n",  INREG32(0xf0005d60)); */
	/*DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI+", "DPI Addr 0xf0005d70 : 0x%08x\n",  INREG32(0xf0005d70)); */
	return 0;
}

int ddp_dpi_ioctl(DISP_MODULE_ENUM module, void *cmdq_handle, unsigned int ioctl_cmd,
		  unsigned long *params)
{
	DISPFUNC();

	int ret = 0;
	DDP_IOCTL_NAME ioctl = (DDP_IOCTL_NAME) ioctl_cmd;
	DISP_LOG_PRINT(ANDROID_LOG_DEBUG, "DPI", "DPI ioctl :  % d\n", ioctl);

	switch (ioctl) {
	case DDP_DPI_FACTORY_TEST:
		{
			disp_ddp_path_config *config_info = (disp_ddp_path_config *) params;

			ddp_dpi_init(module, NULL);
			ddp_dpi_power_on(module, NULL);
			ddp_dpi_stop(module, NULL);
			ddp_dpi_config(module, config_info, NULL);
			ddp_dpi_EnableColorBar();

			ddp_dpi_trigger(module, NULL);
			ddp_dpi_start(module, NULL);
			ddp_dpi_dump(module, 1);
			break;
		}
	default:
		break;
	}

	return ret;
}

DDP_MODULE_DRIVER ddp_driver_dpi = {
	.module = DISP_MODULE_DPI,
	.init = ddp_dpi_init,
	.deinit = ddp_dpi_deinit,
	.config = ddp_dpi_config,
	.build_cmdq = ddp_dpi_build_cmdq,
	.trigger = ddp_dpi_trigger,
	.start = ddp_dpi_start,
	.stop = ddp_dpi_stop,
	.reset = ddp_dpi_reset,
	.power_on = ddp_dpi_power_on,
	.power_off = ddp_dpi_power_off,
	.is_idle = ddp_dpi_is_idle,
	.is_busy = ddp_dpi_is_busy,
	.dump_info = ddp_dpi_dump,
	.set_lcm_utils = ddp_dpi_set_lcm_utils,
	.ioctl = ddp_dpi_ioctl
};
