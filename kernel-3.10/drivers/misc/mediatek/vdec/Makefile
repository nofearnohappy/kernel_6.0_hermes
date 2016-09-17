# Makefile for the linux kernel.
include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -Wno-error=address -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wframe-larger-than=4096

obj-y := vdec.o
obj-y   += hal/
obj-y   += verify/
#obj-$(CONFIG_MTK_LDVT_VDEC)   += hal/
#obj-$(CONFIG_MTK_LDVT_VDEC)   += verify/
