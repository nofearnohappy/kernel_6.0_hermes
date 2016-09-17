/*
 * drivers/input/touchscreen/sweep2wake.c
 *
 *
 * Copyright (c) 2012, Dennis Rassmann <showp1984@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * History:
 *	Added sysfs adjustments for different sweep styles
 * 		by paul reioux (aka Faux123) <reioux@gmail.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/input/sweep2wake.h>

/* Resources */
int sweep2wake = SWEEP2WAKE;
bool scr_suspended = false;
static struct input_dev * sweep2wake_pwrdev;
static DEFINE_MUTEX(pwrkeyworklock);

static int s2w_height_adjust = S2W_HEIGHT_ADJUST;

int tripon = 0;
int tripoff = 0;
unsigned long triptime = 0;

/* Read cmdline for s2w */
static int __init read_s2w_cmdline(char *s2w)
{
	if (strcmp(s2w, "2") == 0) {
		sweep2wake = 2;
	} else if (strcmp(s2w, "1") == 0) {
		sweep2wake = 1;
	} else if (strcmp(s2w, "0") == 0) {
		sweep2wake = 0;
	}else {
		sweep2wake = 0;
	}
	return 1;
}
__setup("s2w=", read_s2w_cmdline);

/* PowerKey setter */
void sweep2wake_setdev(struct input_dev * input_device) {
	sweep2wake_pwrdev = input_device;
	return;
}
EXPORT_SYMBOL(sweep2wake_setdev);

static void reset_sweep2wake(void)
{
    tripoff = 0;
    tripon = 0;
    triptime = 0;
    return;
}

/* PowerKey work func */
static void sweep2wake_presspwr(struct work_struct * sweep2wake_presspwr_work) {
	
	if (!mutex_trylock(&pwrkeyworklock))
           	     return;
        	reset_sweep2wake();
		input_event(sweep2wake_pwrdev, EV_KEY, KEY_POWER, 1);
		input_event(sweep2wake_pwrdev, EV_SYN, 0, 0);
		msleep(60);
		input_event(sweep2wake_pwrdev, EV_KEY, KEY_POWER, 0);
		input_event(sweep2wake_pwrdev, EV_SYN, 0, 0);
		msleep(60);
        	mutex_unlock(&pwrkeyworklock);
		return;
	
}
static DECLARE_WORK(sweep2wake_presspwr_work, sweep2wake_presspwr);

/* PowerKey trigger */
void sweep2wake_pwrtrigger(void) {
	schedule_work(&sweep2wake_presspwr_work);
        return;
}

/* Sweep2wake main function */
void detect_sweep2wake(int sweep_coord, int sweep_height, unsigned long time, int i)
{
    //left->right
    if ((scr_suspended == true) && (sweep2wake > 0) && sweep_height > s2w_height_adjust) {
        if (sweep_coord < 100) {
        tripon = 1;
        triptime = time;
        } else if (tripon == 1 && sweep_coord > 100 && time - triptime < 25) {
            tripon = 2;
        } else if (tripon == 2 && sweep_coord > 200 && time - triptime < 50) {
            tripon = 3;
        } else if (tripon == 3 && sweep_coord > 300 && time - triptime < 75) {
            sweep2wake_pwrtrigger();
        }
    //right->left
    } else if ((scr_suspended == false) && (sweep2wake > 1) && sweep_height > s2w_height_adjust) {
        if (sweep_coord > 400) {
            tripoff = 1;
            triptime = time;
        } else if (tripoff == 1 && sweep_coord < 400 && time - triptime < 25) {
            tripoff = 2;
        } else if (tripoff == 2 && sweep_coord < 300 && time - triptime < 50) {
            tripoff = 3;
        } else if (tripoff == 3 && sweep_coord < 200 && (time - triptime < 75)) {
            sweep2wake_pwrtrigger();
        }
    }
}

/********************* SYSFS INTERFACE ***********************/
static ssize_t s2w_height_adjust_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%i\n", s2w_height_adjust);
}

static ssize_t s2w_height_adjust_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int data;
	if(sscanf(buf, "%i\n", &data) == 1)
		s2w_height_adjust = data;
	else
		pr_info("%s: unknown input!\n", __FUNCTION__);
	return count;
}

static ssize_t sweep2wake_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%i\n", sweep2wake);
}

static ssize_t sweep2wake_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int data;
	if(sscanf(buf, "%i\n", &data) == 1)
		sweep2wake = data;
	else
		pr_info("%s: unknown input!\n", __FUNCTION__);
	return count;
}

static struct kobj_attribute s2w_height_adjust_attribute =
	__ATTR(s2w_height_adjust,
		0666,
		s2w_height_adjust_show,
		s2w_height_adjust_store);

static struct kobj_attribute sweep2wake_attribute =
	__ATTR(sweep2wake,
		0666,
		sweep2wake_show,
		sweep2wake_store);

static struct attribute *s2w_parameters_attrs[] =
	{
		&s2w_height_adjust_attribute.attr,
		&sweep2wake_attribute.attr,
		NULL,
	};

static struct attribute_group s2w_parameters_attr_group =
	{
		.attrs = s2w_parameters_attrs,
	};

static struct kobject *s2w_parameters_kobj;

/*
 * INIT / EXIT stuff below here
 */

static int __init sweep2wake_init(void)
{
	int sysfs_result;

	s2w_parameters_kobj = kobject_create_and_add("android_touch", NULL);
	if (!s2w_parameters_kobj) {
		return -ENOMEM;
        }

	sysfs_result = sysfs_create_group(s2w_parameters_kobj, &s2w_parameters_attr_group);

        if (sysfs_result) {
		kobject_put(s2w_parameters_kobj);
	}
	return sysfs_result;
}

static void __exit sweep2wake_exit(void)
{
	return;
}

module_init(sweep2wake_init);
module_exit(sweep2wake_exit);

MODULE_DESCRIPTION("Sweep2wake");
MODULE_LICENSE("GPLv2");

