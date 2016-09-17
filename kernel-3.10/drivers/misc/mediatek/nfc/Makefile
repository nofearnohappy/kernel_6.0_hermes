
#obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
# legacy chip use v1 driver
ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6582" "mt6592" "mt8127"))
    obj-y += v1/
else
    obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
