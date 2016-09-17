#######################################
# Specify source files
#######################################

# BUILD-IN
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/gcpu/$(MTK_PLATFORM)),)
obj-y += $(MTK_PLATFORM)/
else ### For IP-BASE
obj-y += common/ ### For MT6xxx
endif
