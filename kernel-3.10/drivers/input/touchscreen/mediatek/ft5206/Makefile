include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/ft5206/
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/ft5206

obj-y	+=  focaltech_driver.o
obj-y	+=  focaltech_ex_fun.o
obj-y	+=  focaltech_ctl.o
obj-y	+=  fts_alsps.o
# sweep2wake
# obj-y	+=  sweep2wake.o
