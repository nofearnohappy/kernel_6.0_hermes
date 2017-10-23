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

obj-$(CONFIG_TOUCHSCREEN_MZ_FT5346) += FT5346/

ifeq ($(CONFIG_TOUCHSCREEN_MTK_ATMEL_MXT336),y)
obj-y	+=  ATMEL_MXT336/
endif

ifeq ($(CONFIG_MTK_FT5206),y)
obj-y	+=  ft5206/
endif

