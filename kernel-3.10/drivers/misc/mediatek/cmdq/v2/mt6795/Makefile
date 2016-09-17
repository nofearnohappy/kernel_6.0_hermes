# drivers/cmdq

ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2 \
             -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include

# Enable MET
ccflags-y += -DMET_USER_EVENT_SUPPORT

obj-y += cmdq_mdp.o