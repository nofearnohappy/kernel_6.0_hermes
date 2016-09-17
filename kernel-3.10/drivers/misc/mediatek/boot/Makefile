include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_BOOT) += BOOT_COMMON.o
BOOT_COMMON-$(CONFIG_MTK_BOOT) := mt_boot_common.o

obj-$(CONFIG_MTK_BOOT) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
