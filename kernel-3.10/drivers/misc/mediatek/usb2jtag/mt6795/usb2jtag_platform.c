#include <linux/mu3phy/mtk-phy.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/mu3phy/mtk-phy-asic.h>
#include <linux/mu3d/hal/mu3d_hal_osal.h>
#include <mach/upmu_hw.h>
#include <mach/sync_write.h>
#include <mach/mt_pm_ldo.h>
#include <mach/mt_clkmgr.h>
#include <asm/io.h>
#include "../usb2jtag_v1.h"

#define RG_SSUSB_VUSB10_ON (1<<5)
#define RG_SSUSB_VUSB10_ON_OFST (5)

static void usb_clock_on(void __iomem *addr)
{
	/*---POWER-----*/
	/*AVDD18_USB_P0 is always turned on. The driver does _NOT_ need to control it.*/
	hwPowerOn(MT6332_POWER_LDO_VUSB33, VOL_3300, "VDD33_USB_P0");
	/* Set RG_VUSB10_ON as 1 after VDD10 Ready */
	hwPowerOn(MT6331_POWER_LDO_VUSB10, VOL_1000, "VDD10_USB_P0");
	/*---CLOCK-----*/
	/* ADA_SSUSB_XTAL_CK:26MHz */
	set_ada_ssusb_xtal_ck(1);
	/* AD_LTEPLL_SSUSB26M_CK:26MHz always on */
	/* It seems that when turning on ADA_SSUSB_XTAL_CK, AD_LTEPLL_SSUSB26M_CK will also turn on.*/
	/* enable_ssusb26m_ck(true); */
	/* f_fusb30_ck:125MHz */
	enable_clock(MT_CG_PERI_USB0, "USB30");
	/* AD_SSUSB_48M_CK:48MHz */
	/* It seems that when turning on f_fusb30_ck, AD_SSUSB_48M_CK will also turn on.*/
	/*Wait 50 usec. (PHY 3.3v & 1.8v power stable time)*/
	udelay(50);
	/* Set RG_SSUSB_VUSB10_ON as 1 after VUSB10 ready */
	U3PhyWriteField32(addr + 0xb00, RG_SSUSB_VUSB10_ON_OFST, RG_SSUSB_VUSB10_ON, 1);
}

static int mt_usb2jtag_hw_init(void)
{
	void __iomem *INFRA_AO_BASE;
	void __iomem *USB_SIF_BASE;
	void __iomem *USB_SIF2_BASE;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6795-usb2jtag");
	if (!node) {
		pr_err("[USB2JTAG] map node @ mt6795-usb2jtag failed\n");
		return -1;
	}
	INFRA_AO_BASE = of_iomap(node, 0);
	if (!INFRA_AO_BASE) {
		pr_err("[USB2JTAG] iomap INFRA_AO_BASE failed\n");
		return -1;
	}
	USB_SIF_BASE = of_iomap(node, 1);
	if (!USB_SIF_BASE) {
		pr_err("[USB2JTAG] iomap USB_SIF_BASE failed\n");
		return -1;
	}
	USB_SIF2_BASE = of_iomap(node, 2);
	if (!USB_SIF2_BASE) {
		pr_err("[USB2JTAG] iomap USB_SIF2_BASE failed\n");
		return -1;
	}
	/* set ap_usb2jtag_en: 0x1000_1f00 bit[14] = 1 */
	writel(readl(INFRA_AO_BASE + 0xf00) | (0x1 << 14), INFRA_AO_BASE + 0xf00);

	usb_clock_on(USB_SIF2_BASE);

	writel(0x488, USB_SIF2_BASE + 0x0818);
	writel(readl(USB_SIF_BASE + 0x0820) | (0x1 << 8), USB_SIF_BASE + 0x0820);
	writel(readl(USB_SIF_BASE + 0x0800) | (0x1 << 0), USB_SIF_BASE + 0x0800);
	writel(readl(USB_SIF_BASE + 0x0808) & ~(0x1 << 17), USB_SIF_BASE + 0x0808);

	pr_err("[USB2JTAG] 0x10001f00 = 0x%x\n", readl(INFRA_AO_BASE + 0xf00));
	pr_err("[USB2JTAG] 0x11290818 = 0x%x\n", readl(USB_SIF2_BASE + 0x0818));
	pr_err("[USB2JTAG] 0x11210820 = 0x%x\n", readl(USB_SIF_BASE + 0x0820));
	pr_err("[USB2JTAG] 0x11210800 = 0x%x\n", readl(USB_SIF_BASE + 0x0800));
	pr_err("[USB2JTAG] 0x11210808 = 0x%x\n", readl(USB_SIF_BASE + 0x0808));

	pr_err("[USB2JTAG] setting done\n");
	return 0;
}

static int __init mt_usb2jtag_platform_init(void)
{
	struct mt_usb2jtag_driver *mt_usb2jtag_drv;

	mt_usb2jtag_drv = get_mt_usb2jtag_drv();
	mt_usb2jtag_drv->usb2jtag_init = mt_usb2jtag_hw_init;
	mt_usb2jtag_drv->usb2jtag_suspend = NULL;
	mt_usb2jtag_drv->usb2jtag_resume = NULL;

	return 0;
}

arch_initcall(mt_usb2jtag_platform_init);
