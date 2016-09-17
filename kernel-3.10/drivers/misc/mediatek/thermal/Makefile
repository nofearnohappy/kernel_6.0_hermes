THERMAL_CHIP_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/thermal/$(MTK_PLATFORM)

ccflags-y += -I$(srctree)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc

obj-$(CONFIG_THERMAL) += mtk_thermal_monitor.o
obj-$(CONFIG_THERMAL) += mtk_cooler_shutdown.o
ifeq ($(CONFIG_MTK_LEDS), y)
    # setMaxbrightness() defined in mediatek/kernel/drivers/leds/leds_drv.c only built when CONFIG_MTK_LEDS is set
    obj-$(CONFIG_THERMAL) += mtk_cooler_backlight.o
endif

obj-$(CONFIG_THERMAL) += mtk_cooler_kshutdown.o
obj-$(CONFIG_THERMAL) += mtk_cooler_cam.o
obj-$(CONFIG_THERMAL) += mtk_cooler_vrt.o


ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6582" "mt6592"))
ifeq ($(CONFIG_THERMAL), y)
else
    obj-y += mtk_thermal_dummy.o
endif

# Only included in non-EVB projects
ifeq ($(CONFIG_POWER_EXT), y)
else
    ifeq ($(CONFIG_MTK_SMART_BATTERY), y)
            obj-$(CONFIG_THERMAL) += mtk_cooler_bcct.o
            obj-$(CONFIG_THERMAL) += mtk_ts_battery.o
    endif
endif

ifeq ($(CONFIG_MTK_THERMAL_PA_VIA_ATCMD),y)
    #obj-$(CONFIG_THERMAL) += mtk_cooler_mdulthro.o
    #obj-$(CONFIG_THERMAL) += mtk_cooler_mdtxpwr.o
    #obj-$(CONFIG_THERMAL) += mtk_cooler_mddtxpwr.o
    #obj-$(CONFIG_THERMAL) += mtk_cooler_amdtxctrl.o
    #obj-$(CONFIG_THERMAL) += mtk_cooler_amddulthro.o
    #obj-$(CONFIG_THERMAL) += mtk_cooler_mddulthro.o
endif

obj-$(CONFIG_THERMAL) += mtk_thermal_platform.o
obj-$(CONFIG_THERMAL) += mtk_ts_abb.o
obj-$(CONFIG_THERMAL) += mtk_ts_pa_thput.o
obj-$(CONFIG_THERMAL) += mtk_ts_pmic.o
#obj-$(CONFIG_THERMAL) += mtk_ts_xtal.o
#obj-$(CONFIG_THERMAL) += mtk_ts_skin.o
obj-$(CONFIG_THERMAL) += mtk_ts_pa.o

ifeq ($(CONFIG_MTK_TMP103_SUPPORT),y)
#obj-$(CONFIG_THERMAL) += mtk_ts_pcb.o
endif

ifneq ($(CONFIG_MTK_COMBO_WIFI),)
    obj-$(CONFIG_THERMAL) += mtk_ts_wmt.o
endif
endif


obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

