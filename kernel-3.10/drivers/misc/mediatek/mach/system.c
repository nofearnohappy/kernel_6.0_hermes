#include <linux/kernel.h>
#include <linux/string.h>

#include <mach/mtk_rtc.h>
#include <mach/wd_api.h>




void arch_reset(char mode, const char *cmd)
{
	char reboot = 0;
	int res = 0;
	struct wd_api *wd_api = NULL;
#ifdef CONFIG_FPGA_EARLY_PORTING
	return;
#else

	res = get_wd_api(&wd_api);
	pr_warn("arch_reset: cmd = %s\n", cmd ? : "NULL");

	if (cmd && !strcmp(cmd, "charger")) {
		/* do nothing */
	} else if (cmd && !strcmp(cmd, "recovery")) {
 #ifndef CONFIG_MTK_FPGA
		rtc_mark_recovery();
 #endif
	} else if (cmd && !strcmp(cmd, "bootloader")) {
 #ifndef CONFIG_MTK_FPGA
		rtc_mark_fast();
 #endif
	}
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	else if (cmd && !strcmp(cmd, "kpoc"))
		rtc_mark_kpoc();
#endif
	else
		reboot = 1;

	if (res)
		pr_warn("arch_reset, get wd api error %d\n", res);
	else
		wd_api->wd_sw_reset(reboot);
 #endif
}
