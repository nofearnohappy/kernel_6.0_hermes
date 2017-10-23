/*
 * Meizu touchpanel gesture handling module
 * Date: Thu, 23 Oct 2014 22:03:36 CST
 * Author: Pingbo WEN <wpb@meizu.com>
 * Usage:
 * This module is writed for flyme gesture control. In order to capture
 * touchpanel gesture by flyme, you should report gesture by mz_gesture_report
 * api.
 *
 * In addition, If you want to change TP mode according to flyme gesture state,
 * a callback is needed to update TP mode(gesture mode or sleep mode) in every
 * state changing. Using mz_gesture_handle_register to register tp callback.
 */

#ifndef MEIZU_TOUCHPANEL_GESTURE_CORE_H__
#define MEIZU_TOUCHPANEL_GESTURE_CORE_H__

#include <linux/input.h>

#define MZ_GESTURE_DISABLE	(0x00)
#define MZ_GESTURE_ENABLE	(0x01)
#define MZ_GESTURE_PART		(0x02)

#define GESTURE_ERROR	0x00

/*double tap */
#define DOUBLE_TAP	0xA0

/*swipe  */
#define SWIPE_X_LEFT	0xB0
#define SWIPE_X_RIGHT	0xB1
#define SWIPE_Y_UP	0xB2
#define SWIPE_Y_DOWN	0xB3

/*unicode */
#define UNICODE_E	0xC0
#define UNICODE_C	0xC1
#define UNICODE_W	0xC2
#define UNICODE_M	0xC3
#define UNICODE_O	0xC4
#define UNICODE_S	0xC5
#define UNICODE_V_UP	0xC6
#define UNICODE_V_DOWN	0xC7
#define UNICODE_V_L	0xC8
#define UNICODE_V_R	0xC9
#define UNICODE_Z	0xCA

/*
 * @gesture accepted, return 1
 * @gesture rejected, return 0
 */
int mz_gesture_report(struct input_dev *dev, u8 gesture);
void mz_gesture_handle_register(void (*handle) (u8));

#endif
