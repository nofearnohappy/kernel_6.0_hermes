# Custom kernel for Xiaomi Redmi Note 2 (Hermes)
# Kernel version 3.10.72
# Vendor Vanzo (ALPS-MP-M0.MP11-V1_VZ6795_LWT_M)
Works in rom(ALPS 6.0)
http://4pda.ru/forum/index.php?showtopic=695717&st=720#entry52877661

Works in rom(CM13)
http://4pda.ru/forum/index.php?showtopic=695717&st=1020#entry53819235

=========================================================================
* Works:
	* LCM(NT35596_TIANMA , NT35596_AUO , NT35532_BOE)
	* Touch (ATMEL , FT5206)
	* CW2015
	* Sdcard
	* Wi-fi
	* BT
	* GPS
	* Button-backlight
	* Brightness
	* Leds indication
	* Rill(sim1 and sim2)
	* Alsps (LT559 and STK)
	* Accel(BMI160_ACC LSM6DS3_ACCEL)
	* Giro
	* OTG
	* Sound(Speaker,Headphones)
	* Vibrator
	* Battery 3000mah(stock table)
	* Flashlight
	* Camera(s5k3m2 OV5670)
	* Lens(DF9761BAF)
	* Fixed graphics bug
      	* Fixed bug headphones Disconnects
      	* Twitching with the change in the brightness thresholds
      	* Сharging(0-100% 2.5 hours)
      	* Proximity Sensor Calibration
      	* GPU from source sony

=========================================================================
* Disabled drivers:

        * MAGNETOMETER             (AKM09911_NEW)

=========================================================================
* Don't work:
	* IR Blaster
        * MAGNETOMETER             (YAS537)

=================================================
# BUILD
export TOP=$(pwd)
export CROSS_COMPILE=/home/nofearnohappy/aarch64-linux-android-4.9-linaro-master/bin/aarch64-linux-android-
mkdir -p $TOP/KERNEL_OBJ
make -C kernel-3.10 O=$TOP/KERNEL_OBJ ARCH=arm64 MTK_TARGET_PROJECT=hermes TARGET_BUILD_VARIANT=user CROSS_COMPILE=$TOOLCHAIN ROOTDIR=$TOP hermes_defconfig
make -C kernel-3.10 O=$TOP/KERNEL_OBJ ROOTDIR=$TOP

# I2C

* I2C0
	* TPS65132              (003e)
	* KD_CAMERA_HW          (007f)
	* DF9761BAF             (0018) - LENS
	* CAM_CAL_DRV           (0036)

* I2C1
	* DA9210                (0068)
	* TPS6128x              (0075)

* I2C2
	* ATMEL                 (004a)
	* KD_CAMERA_HW_BUS_2    (007f)
	* FT			(0038)

* I2C3
	* AKM0991               (000c)
	* YAS537                (002e)
	* LSM6DS3_ACCEL         (006a)
	* LTR_559ALS		(0023)
	* LSM6DS3_GYRO		(0034)
	* STK3X1X               (0048)
	* BMI160_GYRO		(0066)
	* BMI160_ACC		(0068)

* I2C4
	* CW2015 		(0062)

# AUTORS
* nofearnohappy
* LazyC0DEr
* Anomalchik
