include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y += ddp_color.o

ifeq ($(strip $(MTK_PQ_COLOR_MODE)),MDP)
    ccflags-y += -DDISP_COLOR_OFF
else ifeq ($(strip $(MTK_PQ_COLOR_MODE)),OFF)
    ccflags-y += -DDISP_COLOR_OFF
endif

ifeq (y, $(filter y, $(CONFIG_ARCH_MT6755)))
ccflags-y += -DCCORR_SUPPORT
endif

ifeq ($(strip $(MTK_PQ_COLOR_MODE)),MDP)
    ccflags-y += -DMDP_COLOR_ON
else ifeq ($(strip $(MTK_PQ_COLOR_MODE)),DISP_MDP)
    ccflags-y += -DDISP_MDP_COLOR_ON
else ifeq ($(strip $(MTK_PQ_COLOR_MODE)),DISP)
    ccflags-y += -DDISP_COLOR_ON
endif

