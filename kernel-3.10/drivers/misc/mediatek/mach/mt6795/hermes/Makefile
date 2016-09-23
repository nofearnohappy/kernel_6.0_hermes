include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
ARCH_MTK_PROJECT := $(shell echo $(CONFIG_ARCH_MTK_PROJECT))
obj-y   += core/
obj-y   += dct/dct/
endif

obj-y   +=  camera/camera/
obj-$(CONFIG_MTK_VIBRATOR)   +=  vibrator/
obj-$(CONFIG_MTK_LEDS)   +=  leds/
obj-y   +=  keypad/
obj-y   +=  sound/
obj-y   +=  accdet/

ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)
ifeq ($(CONFIG_MTK_LTR559),y)
obj-y	+=  alsps/ltr559/
endif

ifeq ($(CONFIG_MTK_STK3X1X),y)
obj-y	+=  alsps/stk3x1x/
endif

ifeq ($(CONFIG_MTK_BMI160_ACC),y)
obj-y   +=  accelerometer/bmi160_acc/
endif

ifeq ($(CONFIG_MTK_BMI160_GYRO),y)
obj-y   +=  gyroscope/bmi160_gyro/
endif

ifeq ($(CONFIG_MTK_YAS537),y)
obj-y	+=  magnetometer/yamaha537/
endif
endif
