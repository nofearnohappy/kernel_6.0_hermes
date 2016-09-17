include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_MMC) += $(subst ",,$(CONFIG_MTK_PLATFORM))/

ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/common/include/trustzone/rpmb/drrpmb/public
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/common/include/trustzone/rpmb/tlrpmb/public
endif

ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
endif

ccflags-y += -I$(srctree)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmc-host/$(MTK_PLATFORM)
obj-$(CONFIG_MTK_MMC) += emmc_rpmb.o
