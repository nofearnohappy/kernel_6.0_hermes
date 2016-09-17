################################################################################
# COMMON PART
# 1. xxx.h/c: Source files that is common for all chips
# 2. IPvx/: Common part for a Chipset Groups
#
# When one new chip comming, you can select an existing common part, ex. IPv1 or
# you can create a new group, ex. IPv2
################################################################################

include $(srctree)/drivers/misc/mediatek/Makefile.custom


######################################
# MT6572/82/92 are lagecy projects, will be removed, so no need common part
# any more, or put it here


#####################################
# MT67xx chipset group
#obj-$(CONFIG_ARCH_MT6595) += IPv1/
#obj-$(CONFIG_ARCH_MT6752) += IPv1/
#obj-$(CONFIG_ARCH_MT6795) += IPv1/
#obj-$(CONFIG_ARCH_MT6735) += IPv1/
#obj-$(CONFIG_ARCH_MT6580) += IPv1/
#obj-$(CONFIG_ARCH_MT6755) += IPv1/

ifeq (y, $(filter y, $(CONFIG_ARCH_MT6752)))
obj-y += aal20/ \
	corr10/
endif

# Todo: newer chip
ifeq (y, $(filter y, $(CONFIG_ARCH_MT6755) $(CONFIG_ARCH_MT6752) $(CONFIG_ARCH_MT6735) $(CONFIG_ARCH_MT6735M) $(CONFIG_ARCH_MT6753) $(CONFIG_ARCH_MT6580) $(CONFIG_ARCH_MT6795) $(CONFIG_ARCH_MT6595)))
obj-y += \
	color20/
endif

#####################################
# Add new chipset groups here


#####################################
# Common Source files for all chips
obj-$(CONFIG_MTK_FB) += 			\
				data_rgb888_64x64.o		\
				data_rgb888_64x64_golden.o \
				mtkfb_fence.o \
				ddp_color_format.o

ifeq (y, $(CONFIG_ARCH_MT6752))
obj-$(CONFIG_MTK_FB) += mtkfb.o
endif

ccflags-y += -I$(srctree)/drivers/staging/android/ion

ccflags-y += \
				-I$(srctree)/drivers/misc/mediatek/video/include     \
				-I$(srctree)/drivers/misc/meditatek/cmdq/v2/ \
				-I$(srctree)/drivers/misc/mediatek/video/common/IPv1     \
				-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys \
				-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox \

ifneq ($(CONFIG_MTK_FB), y)

obj-y += mtkfb_dummy.o

endif

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6735/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6735/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6735/
endif
ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6735m/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6735m/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6735m/
endif
ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6753/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6753/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6753/
endif
