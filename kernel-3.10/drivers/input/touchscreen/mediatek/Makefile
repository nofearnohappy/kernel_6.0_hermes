include $(srctree)/drivers/misc/mediatek/Makefile.custom


# In case the platform does NOT support this type of sensors
obj-y   +=  mtk_tpd.o
obj-y   +=  tpd_button.o
obj-y   +=  tpd_calibrate.o
obj-y   +=  tpd_debug.o
obj-y   +=  tpd_default.o
obj-y   +=  tpd_init.o
obj-y   +=  tpd_misc.o
obj-y   +=  tpd_setting.o
#Vanzo:wangfei on: Wed, 12 Nov 2014 21:06:42 +0800
# for compatible the old style ,so use CUSTOM_KERNEL_IMGSENSOR
project_name:=$(shell echo $(VANZO_INNER_PROJECT_NAME))
ifneq ($(project_name),)

custom_kernel_touchpanel:= $(addsuffix /, $(shell echo $(CUSTOM_KERNEL_TOUCHPANEL)))

$(info "custom_kernel_touchpanel:$(custom_kernel_touchpanel)")
obj-y += $(custom_kernel_touchpanel)
else
# End of Vanzo:wangfei

ifeq ($(CONFIG_TOUCHSCREEN_MTK_ATMEL_MXT336),y)
obj-y	+=  ATMEL_MXT336/
endif

ifeq ($(CONFIG_MTK_FT5436I),y)
obj-y	+=  ft5436i/
endif

ifeq ($(CONFIG_MTK_FT5206),y)
obj-y	+=  ft5206/
endif

# Vanzo:wangfei on: Sat, 28 Feb 2015 16:22:43 +0800
# for use new style to config touchscreen
endif
# End of Vanzo:wangfei
