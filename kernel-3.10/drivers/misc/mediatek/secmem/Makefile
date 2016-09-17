
ifneq ($(MTK_ROOT_BUILD),)
include $(MTK_ROOT_BUILD)/Makefile
else
include $(srctree)/drivers/misc/mediatek/Makefile.custom
endif

ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/include/trustzone/utils/tlutils
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/trustzone/utils/tlutils
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
ccflags-y += -I$(src)/$(MTK_PLATFORM)
endif

obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += secmem.o
