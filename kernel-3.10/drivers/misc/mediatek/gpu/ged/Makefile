include $(srctree)/drivers/misc/mediatek/Makefile.custom

GED_TOP = $(srctree)/drivers/misc/mediatek/gpu/ged

VER:=eng

ifneq ($(strip $(TARGET_BUILD_VARIANT)),)
ifneq ($(strip $(TARGET_BUILD_VARIANT)),eng)
VER:=user
ccflags-y += -DGED_LOG_SIZE_LIMITED
endif
endif

ccflags-y += \
	-I$(GED_TOP)/include/ 

PLATFORM_NAME = $(subst ",,$(CONFIG_MTK_PLATFORM))
$(info *PLATFORM_NAME is: $(PLATFORM_NAME))

ifeq ($(PLATFORM_NAME),mt6795)
# for mt6795
ccflags-y += -DMT_GET_FREQ_ALIAS
CONFIG_MTK_GPU_COMMON_DVFS_SUPPORT=y
endif


$(info $$CONFIG_MTK_GPU_COMMON_DVFS_SUPPORT is [${CONFIG_MTK_GPU_COMMON_DVFS_SUPPORT}])

ifeq ($(CONFIG_MTK_GPU_COMMON_DVFS_SUPPORT),y)
ccflags-y += -DGED_DVFS_ENABLE \
             -DENABLE_COMMON_DVFS \
	     -DENABLE_TIMER_BACKUP \
             -DGED_DVFS_DEBUG_BUF
endif




ccflags-y += \
	-include $(GED_TOP)/include/config_kernel_$(VER).h

obj-y += ged.o

ged-y += \
	src/ged_base.o \
	src/ged_main.o \
	src/ged_debugFS.o \
	src/ged_log.o \
	src/ged_hal.o \
	src/ged_bridge.o \
	src/ged_profile_dvfs.o \
	src/ged_monitor_3D_fence.o \
	src/ged_notify_sw_vsync.o \
	src/ged_hashtable.o \
	src/ged_thread.o \
	src/ged_dvfs.o

