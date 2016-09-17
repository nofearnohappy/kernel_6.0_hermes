include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_FB) += mtkfb_console.o             \
         disp_assert_layer.o                        \
         mtkfb.o                                    \
		 mtkfb_vsync.o			    \
         debug.o                                    \
         primary_display.o                        \
         disp_lcm.o                                 \
         disp_utils.o                               \
	mtk_disp_mgr.o\
	fbconfig_kdebug_x.o\
	mtk_mira.o \
	mtk_ovl.o \
	disp_helper.o

ccflags-y += -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/misc/mediatek/ext_disp \
             -I$(srctree)/drivers/misc/mediatek/ext_disp/$(MTK_PLATFORM)

# TODO: temp solution, add this -I flag so that we can use "dispsys/ddp_od.h" file as mt6795/dispsys/ddp_od.h
# to avoid build error caused by using include/ddp_od.h (which does not declare necessary functions)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/

ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/
endif

ifeq ($(CONFIG_MTK_INTERNAL_MHL_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mhltx/
endif

ifeq ($(CONFIG_MTK_DITHERING_SUPPORT), y)
ccflags-y += -DDITHERING_SUPPORT
endif

ifeq ($(CONFIG_MTK_GPU_SUPPORT), y)
ccflags-y += -DHWGPU_SUPPORT
endif

ifeq ($(CONFIG_MTK_WQHD_VCORE10_SUPPORT), y)
ccflags-y += -DWQHD_VCORE10_SUPPORT
endif

ifeq ($(MTK_MIRAVISION_SUPPORT), yes)
    ccflags-y += -DCONFIG_FOR_SOURCE_PQ
endif

