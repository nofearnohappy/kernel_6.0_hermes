#$(call all-subdir-src-or-makefile)
include $(srctree)/drivers/misc/mediatek/Makefile.custom

# For MT6795 FPGA bring-up
ifeq ($(CONFIG_ARCH_MT6795),y)
include $(srctree)/drivers/misc/mediatek/Makefile.mt6795
else

ifneq (,$(filter $(CONFIG_ARCH_MT6735) $(CONFIG_ARCH_MT6735M) $(CONFIG_ARCH_MT6753), y))
include $(srctree)/drivers/misc/mediatek/Makefile.mt6735
else

ifneq (,$(filter $(CONFIG_ARCH_MT6580), y))
include $(srctree)/drivers/misc/mediatek/Makefile.mt6580
else


ifeq ($(CONFIG_MTK_L2C_SHARE),y)
obj-y   += l2c_share/
endif

ifeq ($(CONFIG_MTK_SYS_CIRQ),y)
obj-y += cirq/
else
obj-$(CONFIG_MTK_CIRQ) += cirq/
endif

ifneq ($(CONFIG_MTK_GIC),y)
obj-y   += irq/
endif

obj-$(CONFIG_MTK_CPU_STRESS)	+= cpu_stress/
obj-$(CONFIG_MTK_LASTPC)	    += lastpc/
obj-$(CONFIG_MTK_DBG_DUMP) += dbg_dump/

obj-$(CONFIG_MTK_EXTMEM) += exm/

obj-$(CONFIG_MTK_SYSTRACKER)   += systracker/
obj-y   += cqdma/

obj-y   += watchpoint/
obj-$(CONFIG_MTK_ETM)   += etm/
obj-y	+= mach/
obj-$(CONFIG_MTK_MEM)	+= mem/
obj-y	+= kernel/
obj-y	+= aee/
obj-$(CONFIG_MTK_EMI_MPU)	+= emi_mpu/
obj-$(CONFIG_MTK_EMI_BWL)	+= emi_bwl/

obj-$(CONFIG_MTK_FMRADIO)	+= fmradio/
obj-$(CONFIG_MTK_GPS)	    += gps/

obj-y	+= m4u/
#obj-y	+= mhl/
obj-$(CONFIG_MTPROF)	+= mtprof/
obj-$(CONFIG_MEDIATEK_SOLUTION)	+= sched/
obj-$(CONFIG_MTK_CMDQ) += cmdq/
obj-y	+= mmp/
obj-$(CONFIG_MTK_FB) += video/

obj-$(CONFIG_MTK_AUXADC)	+= auxadc/

#obj-y	+= bthid/

ifdef CONFIG_CUSTOM_KERNEL_EXTMD
ifneq ($(strip $(CONFIG_CUSTOM_KERNEL_EXTMD)),"")
obj-y	+= emd_ctl/
endif
endif

ifneq ($(wildcard $(srctree)/$(src)/met/),)
 
endif

obj-$(CONFIG_MTK_XHCI)   +=  xhci/

#obj-y	+=  hdmitx/
obj-y	+= ram_console/
obj-$(CONFIG_THERMAL)	+= thermal/
obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += videocodec/
obj-$(CONFIG_MTK_MJC_DRIVER) += mjc/
obj-$(CONFIG_MTK_LDVT_VDEC) += vdec/
obj-y	+= xlog/
ifneq ($(filter y, $(CONFIG_ARM_PSCI) $(CONFIG_ARM64)),)
	obj-y   +=  atf_log/
endif
obj-$(CONFIG_MTK_WD_KICKER)		+= wdk/
obj-$(CONFIG_MTK_MT_LOGGER)	+= mt_logger/
obj-$(CONFIG_MD32_SUPPORT)   += md32/
obj-y += dramc/
obj-y	+= lcm/

ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)

obj-$(CONFIG_CUSTOM_KERNEL_SENSORHUB)	+= sensorHub/
obj-$(CONFIG_MTK_HWMON)	    += hwmon/
obj-$(CONFIG_CUSTOM_KERNEL_ALSPS) += alsps/
obj-$(CONFIG_CUSTOM_KERNEL_ACCELEROMETER) += accelerometer/
obj-$(CONFIG_CUSTOM_KERNEL_GYROSCOPE) += gyroscope/
obj-$(CONFIG_CUSTOM_KERNEL_MAGNETOMETER) += magnetometer/
obj-$(CONFIG_CUSTOM_KERNEL_BAROMETER) += barometer/
obj-$(CONFIG_CUSTOM_KERNEL_STEP_COUNTER)	+= step_counter/
obj-$(CONFIG_CUSTOM_KERNEL_SIGNIFICANT_MOTION_SENSOR)	+= step_counter/
obj-$(CONFIG_CUSTOM_KERNEL_ACTIVITY_SENSOR)	+= activity_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_PICK_UP_SENSOR)	+= pick_up_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_SHAKE_SENSOR)	+= shake_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_FACE_DOWN_SENSOR)	+= face_down_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_IN_POCKET_SENSOR)	+= in_pocket_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_PEDOMETER)	+= pedometer/
obj-$(CONFIG_CUSTOM_KERNEL_HEART)	+= heart_rate_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_TILT_DETECTOR_SENSOR)	+= tilt_detector_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_WAKE_GESTURE_SENSOR)	+= wake_gesture_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_GLANCE_GESTURE_SENSOR)	+= glance_gesture_sensor/
obj-$(CONFIG_CUSTOM_KERNEL_BRINGTOSEE_SENSOR)	+= bringtosee/
obj-$(CONFIG_CUSTOM_KERNEL_GRAVITY_SENSOR)	+= gravity/
obj-$(CONFIG_CUSTOM_KERNEL_RV_SENSOR)	+= rotationvector/
obj-$(CONFIG_CUSTOM_KERNEL_LINEARACCEL_SENSOR)	+= linearacceleration/
obj-$(CONFIG_CUSTOM_KERNEL_GRV_SENSOR)	+= gamerotationvector/
obj-$(CONFIG_CUSTOM_KERNEL_GMRV_SENSOR)	+= geomagneticrotationvector/

endif

#obj-y	+= matv/
#obj-y	+= touchpanel/
# 

obj-$(CONFIG_MTK_VOW_SUPPORT)	+= vow/

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include
obj-$(CONFIG_MTK_COMBO)			+= connectivity/
obj-$(CONFIG_MTK_CONN_MD)	+= conn_md/

obj-y	+=  ccci_util/
obj-$(CONFIG_MTK_NET_CCMNI) +=  ccmni/
obj-$(CONFIG_MTK_CCCI_DRIVER)  +=  ccci/
obj-$(CONFIG_MTK_CCCI_EXT)     +=  dual_ccci/
obj-$(CONFIG_MTK_ECCCI_DRIVER) +=  eccci/
obj-$(CONFIG_MTK_EEMCS_DRIVER) +=  eemcs/
obj-$(CONFIG_MTK_IRTX_SUPPORT) += irtx/
obj-$(CONFIG_MTK_NFC) += nfc/
obj-$(CONFIG_ZRAM)	+= mlog/


obj-$(CONFIG_MTK_LENS)   += lens/
obj-$(CONFIG_MTK_IMGSENSOR)	+= imgsensor/src/


ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/cam_cal),)
obj-$(CONFIG_MTK_CAM_CAL)	+= cam_cal/src/
endif

#exclude mt6735
ifeq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6735"))
obj-y	+=  flashlight/src/
endif

obj-$(CONFIG_MTK_SYNC)	+= sync/

#rtc must compile before power, because they are both in fs_initcall, rtc need initial first for PMIC alarm call back
obj-y	+= rtc/
obj-$(CONFIG_MTK_BOOT)	+= boot/
obj-$(CONFIG_MTK_BOOT_REASON)	+= boot_reason/
obj-$(CONFIG_MTK_CHIP)	+= chip/
obj-$(CONFIG_MTK_GPIO)	+= gpio/
obj-y	+= eint/
obj-$(CONFIG_MTK_SERIAL)	+= uart/
obj-y	+= pmic_wrap/

ifeq ($(CONFIG_MTK_HDMI_SUPPORT),y)
obj-y	+= hdmi/
obj-y	+= ext_disp/
endif

obj-$(CONFIG_MTK_HIBERNATION) += hibcore/
ifeq ($(CONFIG_CUSTOM_KERNEL_FLASHLIGHT),"dummy_flashlight")
#obj-y	+= flashlight/dummy_flashlight/
endif

obj-y	+= accdet/
obj-$(CONFIG_MTK_KEYPAD)	+= keypad/
obj-$(CONFIG_MTK_SYSENV)	+= sysenv/

ifdef CONFIG_CUSTOM_KERNEL_SSW
ifneq ($(strip $(CONFIG_CUSTOM_KERNEL_SSW)),"")
obj-y	+= ssw/
endif
endif

#obj-$(CONFIG_MTK_CMMB) += cmmbdrv/
obj-y	+= devapc/
obj-y	+= devinfo/
obj-$(CONFIG_MTK_GPU_SUPPORT)	+= gpu/

obj-y	+= almk/
obj-$(CONFIG_MTK_JPEG)	+= jpeg/

obj-$(CONFIG_MTK_LEDS)	+= leds/
obj-$(CONFIG_MTK_SECURITY_SW_SUPPORT) +=masp/
obj-$(CONFIG_MTK_MMC)	+= mmc-host/
obj-$(CONFIG_MTK_MMC)	+= pmt/
obj-$(CONFIG_MTK_MTD_NAND)	+= nand/
obj-$(CONFIG_MTK_GPT_SCHEME_SUPPORT)	+= partition/
obj-$(CONFIG_MTK_SOUND) += smartpa/

obj-$(CONFIG_MTK_DUM_CHAR)	+= dum-char/

#charge pump
ifeq ($(CONFIG_CUSTOM_KERNEL_CHARGEPUMP),y)
obj-y	+= chargepump/
endif

obj-y	+= power/
obj-$(CONFIG_MTK_I2C)	+= i2c/
obj-$(CONFIG_MTK_PWM)	+= pwm/
obj-$(CONFIG_MTK_SMI_EXT)	+= smi/
obj-$(CONFIG_MTK_BTCVSD) += btcvsd/
obj-$(CONFIG_MTK_SOUND)	+= sound/
obj-$(CONFIG_MTK_SPI)	+= spi/
obj-$(CONFIG_MTK_SPM)	+= spm/

# USB need to after gpio
obj-$(CONFIG_USB_MTK_HDRC)	+= usb20/
obj-y	+= usb11/
obj-$(CONFIG_USB_MU3D_DRV)	+= mu3d/
obj-$(CONFIG_USB_MU3D_DRV)	+= mu3phy/

obj-$(CONFIG_MTK_TEST_XHCI)   +=  xhci_test/

obj-y	+= vibrator/
obj-$(CONFIG_MTK_WD_KICKER)		+= wdt/
obj-y	+= freqhopping/
obj-$(CONFIG_SECURITY_SELINUX)	+= selinux_warning/
# MTK PASR SW flow
obj-$(CONFIG_MTKPASR)	+= mtkpasr/

obj-$(CONFIG_MTK_TC1_FEATURE)	+= tc1_interface/

obj-$(CONFIG_MTK_SWITCH_TX_POWER) += sw_tx_power/

#charge pump
#obj-y	+= chargepump/

#obj-$(CONFIG_MTK_SOUND)	+= smartpa/

obj-$(CONFIG_MTK_BTIF)	+= btif/
obj-$(CONFIG_MTK_POWER_GS)	+= power_gs/
obj-$(CONFIG_MTK_VIDEO) += video/
obj-$(CONFIG_MTK_AEE_MRDUMP)	+= mrdump/
obj-$(CONFIG_MTK_WMT_CCCI)	+= wmt_ccci/
obj-$(CONFIG_MTK_VCOREFS)	+= vcorefs/

obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += gud/
obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += secmem/
obj-$(CONFIG_MTK_C2K_SUPPORT) += c2k_ccci/
obj-$(CONFIG_MTK_SCHED_CMP_POWER_AWARE_CONTROLLER)	+=  pa/
obj-$(CONFIG_HMP_POWER_AWARE_CONTROLLER)	+=  pa_hmp/

obj-$(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) += trustzone/

#power-loss-test
obj-$(CONFIG_PWR_LOSS_MTK_TEST) += power-loss-test/

endif           # end of CONFIG_ARCH_MT6735
endif		# end of CONFIG_ARCH_MT6795
endif       # end of CONFIG_ARCH_MT6580


ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6752"))
obj-y	+= ddr/$(MTK_PLATFORM)/
endif

obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += gcpu/

# gator driver is platform-independent
ifeq ($(CONFIG_GATOR_DRIVER),y)
 ifneq ($(wildcard $(srctree)/$(src)/gator-driver/),)
  obj-$(CONFIG_GATOR_DRIVER) += gator-driver/
 endif # gator-driver folder exist
endif  # COFNIG_GATOR_DRIVER is set

obj-$(CONFIG_MTK_MULTIBRIDGE_SUPPORT) += multibridge/

#widevine drm
#Classic DRM
$(info "CONFIG_TRUSTONIC_TEE_SUPPORT = $(CONFIG_TRUSTONIC_TEE_SUPPORT)")
obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += secwidevine/
