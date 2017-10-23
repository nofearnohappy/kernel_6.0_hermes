#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/input.h>

#include <linux/input/mz_gesture_ts.h>
#include <linux/meizu-sys.h>

#define pr_fmt(fmt)	"[MZ_GESTURE]" fmt

#define SWIPE_INDEX	(0)
#define TAP_INDEX	(1)
#define UNICODE_INDEX	(2)
#define ALL_INDEX	(3)

/*disable gesture value */
#define ALL_CTR		0x01
#define TAP_CTR		0x02
#define UNICODE_CTR	0x03
#define SWIPE_CTR	0x04

struct gesture_map {
	u8 keycode;
	u8 pos;
};

struct mz_gesture {
	u8 gesture_value;
	/* byte0: unicode, byte1: swipe, byte2: reserved, byte3: tap */
	u32 gesture_mask;
	u8 gesture_state;
	struct mutex gesture_lock;
	// tp handler, to update tp gesture status
	void (*handle) (u8);
};

static struct gesture_map mz_gs_map[] = {
	{
		.keycode = DOUBLE_TAP,
		.pos = 9
	},
	{
		.keycode = SWIPE_X_LEFT,
		.pos = 2
	},
	{
		.keycode = SWIPE_X_RIGHT,
		.pos = 1
	},
	{
		.keycode = SWIPE_Y_UP,
		.pos = 4
	},
	{
		.keycode = SWIPE_Y_DOWN,
		.pos = 3
	},
	{
		.keycode = UNICODE_E,
		.pos = 19
	},
	{
		.keycode = UNICODE_C,
		.pos = 18
	},
	{
		.keycode = UNICODE_W,
		.pos = 20
	},
	{
		.keycode = UNICODE_M,
		.pos = 21
	},
	{
		.keycode = UNICODE_O,
		.pos = 24
	},
	{
		.keycode = UNICODE_S,
		.pos = 22
	},
	{
		.keycode = UNICODE_V_UP,
		.pos = 17
	},
	{
		.keycode = UNICODE_V_DOWN,
		.pos = 17
	},
	{
		.keycode = UNICODE_V_L,
		.pos = 17
	},
	{
		.keycode = UNICODE_V_R,
		.pos = 17,
	},
	{
		.keycode = UNICODE_Z,
		.pos = 23
	},
	{
		.keycode = 0,
		.pos = 0
	},
};

static struct mz_gesture mgs;

static int mz_get_gesture_pos(u8 gesture)
{
	//struct gesure_map const *tmp = mz_gs_map;
	int ret = 0;
	int i = 0;

	while(mz_gs_map[i].keycode != 0) {
		if (mz_gs_map[i].keycode == gesture) {
			ret = mz_gs_map[i].pos;
			break;
		}
		i++;
	}

	if (mz_gs_map[i].keycode == 0)
		pr_err("unknown gesture %02x, ignore\n", gesture);

	return ret;
}

int mz_gesture_report(struct input_dev *dev, u8 gesture)
{
	int pos = 0;
	//default rejected
	int ret = 0;

	if(!mutex_trylock(&(mgs.gesture_lock)))
		return ret; //obtain lock failed

	if(mgs.gesture_state == MZ_GESTURE_DISABLE) {
		pr_warn("all gestures have disabled, ignore this gesture %d\n",
				gesture);
		goto exit;
	}

	pos = mz_get_gesture_pos(gesture);

	pr_err("dump gesture %02x, mask %x, pos %d\n",
			gesture, mgs.gesture_mask, pos);

	if((pos > 0) && test_bit(pos - 1 , &(mgs.gesture_mask))) {
		mgs.gesture_value = gesture;
		input_report_key(dev, KEY_GESTURE, 1);
		input_sync(dev);
		input_report_key(dev, KEY_GESTURE, 0);
		input_sync(dev);
		ret = 1;
	} else {
		pr_debug("ignore gesture %02x, according to mask %08x\n",
				gesture, mgs.gesture_mask);
		mgs.gesture_value = GESTURE_ERROR;
	}

exit:
	mutex_unlock(&(mgs.gesture_lock));
	return ret;
}
EXPORT_SYMBOL(mz_gesture_report);

void mz_gesture_handle_register(void (*handle) (u8))
{
	mutex_lock(&(mgs.gesture_lock));
	mgs.handle = handle;
	mutex_unlock(&(mgs.gesture_lock));
}
EXPORT_SYMBOL(mz_gesture_handle_register);

static ssize_t mz_sysfs_gesture_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int count = 0;

	mutex_lock(&(mgs.gesture_lock));
	count = sprintf(buf, "%u\n", mgs.gesture_value);
	mgs.gesture_value = GESTURE_ERROR;
	mutex_unlock(&(mgs.gesture_lock));

	return count;
}

static ssize_t mz_sysfs_gesture_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return -EPERM;
}

static ssize_t mz_sysfs_gesture_control_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int count = 0;
	mutex_lock(&(mgs.gesture_lock));
	//memcpy(buf, &(mgs.gesture_mask), count);
	count += sprintf(buf, "%x\n", mgs.gesture_mask);
	mutex_unlock(&(mgs.gesture_lock));

	return count;
}

static ssize_t mz_sysfs_gesture_control_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u32 data = 0;
	int tmp = 0, pos = 0;
	u8 byte0 = 0, byte2 = 0;
	int ret = count;

	mutex_lock(&(mgs.gesture_lock));
	memcpy(&data, buf, 4);
	//data = simple_strtoul(buf, NULL, 16);

	byte0 = data & 0xff;
	byte2 = (data >> 16) & 0xff;
	pr_debug("dump control byte2 %02x, byte0 %02x\n", byte2, byte0);

	if (byte0)
		mgs.gesture_state = MZ_GESTURE_ENABLE;
	if (byte2 == ALL_CTR) {
		pos = ALL_INDEX * 8 + 6;
		mgs.gesture_mask &= ~(0x3 << pos);
		mgs.gesture_mask |= (0x01 & byte0) << pos;
		mgs.gesture_state = byte0;
	} else {
		if (byte2 == SWIPE_CTR) {
		pos = SWIPE_INDEX * 8;
		mgs.gesture_mask &= ~(0x0f << pos);
		mgs.gesture_mask |= (0x0f & byte0) << pos;
		} else if (byte2 == UNICODE_CTR) {
			pos = UNICODE_INDEX * 8;
			mgs.gesture_mask &= ~(0xff << pos);
			mgs.gesture_mask |= (0xff & byte0) << pos;
		} else if (byte2 == TAP_CTR) {
			pos = TAP_INDEX * 8;
			mgs.gesture_mask &= ~(0x01 << pos);
			mgs.gesture_mask |= (0x01 & byte0) << pos;
		} else {
			pr_err("parse gesture type error\n");
			ret = -EIO;
			goto exit;
		}

		if((mgs.gesture_mask & 0xffffff) == 0xff010f)
			tmp = 0x01; // all gesture opened
		else if ((mgs.gesture_mask & 0xffffff) == 0x0)
			tmp = 0x0; // all gesture closed
		else
			tmp = 0x2; // part gesture opened

		pos = ALL_INDEX * 8 + 6;
		mgs.gesture_mask &= ~(0x3 << pos);
		mgs.gesture_mask |= tmp << pos;
		// update tp gesture state
		mgs.gesture_state = tmp;
	}

	if(mgs.handle != NULL)
		mgs.handle(mgs.gesture_state);

exit:
	mutex_unlock(&(mgs.gesture_lock));
	return ret;
}

static DEVICE_ATTR(gesture_control, S_IRUGO | S_IWUSR,
		mz_sysfs_gesture_control_show, mz_sysfs_gesture_control_store);
static DEVICE_ATTR(gesture_data, S_IRUGO | S_IWUSR,
		mz_sysfs_gesture_data_show, mz_sysfs_gesture_data_store);

static struct attribute *mz_gesture_attributes[] = {
	&dev_attr_gesture_control.attr,
	&dev_attr_gesture_data.attr,
	NULL,
};

static struct attribute_group mz_gesture_attribute_group = {
	.attrs = mz_gesture_attributes,
};

static int mz_gesture_probe(struct platform_device *pdev)
{
	int ret = 0;
	mutex_init(&(mgs.gesture_lock));

	mutex_lock(&(mgs.gesture_lock));

	mgs.gesture_value = GESTURE_ERROR;

	// default, only enable tap gesture
	mgs.gesture_mask = 0x80000100;

	mgs.gesture_state = 0x2;

	mgs.handle = NULL;

	ret = sysfs_create_group(&pdev->dev.kobj, &mz_gesture_attribute_group);
	if (ret) {
		pr_err("create sysfs failed, abort\n");
		goto failed;
	}

	meizu_sysfslink_register_name(&pdev->dev, "mx-gs");
	pr_warn("mz_gesture probe success\n");

failed:
	mutex_unlock(&(mgs.gesture_lock));
	return ret;
}

static int mz_gesture_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &mz_gesture_attribute_group);

	return 0;
}

struct platform_device mz_gesture_device = {
	.name = "mx-gs",
	.id = -1,
};

struct platform_driver mz_gesture_driver = {
	.driver = {
	       .name = "mx-gs",
	       .owner = THIS_MODULE,
	},
	.probe = mz_gesture_probe,
	.remove = mz_gesture_remove,
};

static int __init mz_gesture_init(void)
{
	if(platform_device_register(&mz_gesture_device) != 0) {
		pr_err("add platform device error\n");
		return -1;
	}

	if(platform_driver_register(&mz_gesture_driver) != 0) {
		pr_err("add platform driver error\n");
		return -1;
	}

	return 0;
}

static void __exit mz_gesture_exit(void)
{
	platform_driver_unregister(&mz_gesture_driver);
}

module_init(mz_gesture_init);
module_exit(mz_gesture_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pingbo WEN <wpb@meizu.com>");
