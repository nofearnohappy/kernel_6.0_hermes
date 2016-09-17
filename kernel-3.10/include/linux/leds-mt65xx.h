/******************************************************************************
 * mt65xx_leds.h
 *
 * Copyright 2010 MediaTek Co.,Ltd.
 *
 ******************************************************************************/
#ifndef _MT65XX_LEDS_H
#define _MT65XX_LEDS_H

#include <linux/leds.h>
#include <cust_leds.h>

#ifdef CONFIG_MTK_LEDS
extern int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness value);
extern int backlight_brightness_set(int level);
#else
static inline int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness value){return 0;}
static inline int backlight_brightness_set(int level){return 0;}
#endif

#endif
