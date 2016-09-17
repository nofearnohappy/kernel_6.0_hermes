include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/inc\
					-I$(srctree)/drivers/misc/mediatek/ext_disp/$(CONFIG_MTK_PLATFORM)\
					-I$(srctree)/drivers/misc/mediatek/dispsys/$(CONFIG_MTK_PLATFORM)

ifeq ($(CONFIG_CUSTOM_KERNEL_HDMI), "MT8193")
ccflags-y += 	-DHDMI_MT8193_SUPPORT
endif

obj-y	+= hdmi_drv.o mt8193_ctrl.o mt8193avd.o mt8193cec.o mt8193ddc.o mt8193edid.o mt8193hdcp.o mt8193hdmicmd.o mt8193hdmictrl.o
