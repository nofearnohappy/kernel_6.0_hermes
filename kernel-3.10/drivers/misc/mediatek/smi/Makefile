include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/

ifeq ($(CONFIG_ARCH_MT6582),y)
ccflags-y += -DSMI_82
obj-y += smi_common_v1.o
obj-y += smi_info_util.o
else
ifeq ($(CONFIG_ARCH_MT6592),y)
ccflags-y += -DSMI_92
obj-y += smi_common_v1.o
obj-y += smi_info_util.o
else
ifeq ($(CONFIG_ARCH_MT6795),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include
ccflags-y += -DSMI_RO
obj-y += mmdvfs_mgr.o
obj-y += smi_common_v2.o
obj-y += smi_info_util.o
else
ifeq ($(CONFIG_ARCH_MT6752),y)
ccflags-y += -DSMI_K2
obj-y += smi_common_v2.o
obj-y += smi_info_util.o
else
ifeq ($(CONFIG_ARCH_MT2601),y)
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
else
ifeq ($(CONFIG_ARCH_MT8127),y)
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
else
ifeq ($(CONFIG_ARCH_MT8163),y)
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
else
obj-y += smi_common_v3.o
obj-y += mmdvfs_mgr_v3.o
endif
endif
endif
endif
endif
endif
endif