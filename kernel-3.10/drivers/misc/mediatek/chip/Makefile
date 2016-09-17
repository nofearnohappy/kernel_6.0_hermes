include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_CHIP) += CHIP_COMMON.o
CHIP_COMMON-$(CONFIG_MTK_CHIP) := mt_chip_common.o

obj-$(CONFIG_MTK_CHIP) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
