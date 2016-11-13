/*
 * Copyright � 2014, Varun Chitre "varun.chitre15" <varun.chitre15@gmail.com>
 *
 * Vibration Intensity Controller for MTK Vibrator
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * Please preserve this licence and driver name if you implement this 
 * anywhere else.
 *
 */
 
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>

#include <cust_vibrator.h>
#include <vibrator_hal.h>

#define MAX_VIBR 7
#define MIN_VIBR 0

extern void mt6331_upmu_set_rg_vibr_vosel(kal_uint32 val);

static ssize_t vibr_vtg_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vibrator_hw* hw = mt_get_cust_vibrator_hw();

	return sprintf(buf, "%d\n", hw->vib_vol);
}

static ssize_t vibr_vtg_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;
    struct vibrator_hw* hw = mt_get_cust_vibrator_hw();
	sscanf(buf, "%u", &val);
	if(val >= MIN_VIBR && val <= MAX_VIBR) {
	    mt6331_upmu_set_rg_vibr_vosel(val);
	    hw->vib_vol=val;
	    }
    
	return count;
}

static struct kobj_attribute thunderquake_level_attribute =
	__ATTR(pwmvalue,
		0666,
		vibr_vtg_show, vibr_vtg_store);

static struct attribute *thunderquake_engine_attrs[] =
	{
		&thunderquake_level_attribute.attr,
		NULL,
	};

static struct attribute_group vibr_level_control_attr_group =
	{
		.attrs = thunderquake_engine_attrs,
	};
 
static struct kobject *vibr_level_control_kobj;

static int vibr_level_control_init(void)
{
	int sysfs_result;
	printk(KERN_DEBUG "[%s]\n",__func__);

	vibr_level_control_kobj =
		kobject_create_and_add("vibrator", NULL);

	if (!vibr_level_control_kobj) {
		pr_err("%s Interface create failed!\n",
			__FUNCTION__);
		return -ENOMEM;
        }

	sysfs_result = sysfs_create_group(vibr_level_control_kobj,
			&vibr_level_control_attr_group);

	if (sysfs_result) {
		pr_info("%s sysfs create failed!\n", __FUNCTION__);
		kobject_put(vibr_level_control_kobj);
	}
	return sysfs_result;
}

static void vibr_level_control_exit(void)
{
	if (vibr_level_control_kobj != NULL)
		kobject_put(vibr_level_control_kobj);
}

module_init(vibr_level_control_init);
module_exit(vibr_level_control_exit);
MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Varun Chitre <varun.chitre15@gmail.com>");
MODULE_DESCRIPTION("ThundQuake Engine - Driver to control Mediatek Vibrator Intensity");
