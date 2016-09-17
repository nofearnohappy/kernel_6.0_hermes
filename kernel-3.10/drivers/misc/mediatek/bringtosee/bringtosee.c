#include "bringtosee.h"

static struct bts_context *bts_context_obj = NULL;

static struct bts_init_info* bringtosee_init= {0}; //modified
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_EARLYSUSPEND)
static void bts_early_suspend(struct early_suspend *h);
static void bts_late_resume(struct early_suspend *h);
#endif

static int resume_enable_status = 0;

static struct bts_context *bts_context_alloc_object(void)
{
	struct bts_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL); 
    	BTS_LOG("bts_context_alloc_object++++\n");
	if(!obj)
	{
		BTS_ERR("Alloc bts object error!\n");
		return NULL;
	}	
	atomic_set(&obj->wake, 0);
	mutex_init(&obj->bts_op_mutex);

	BTS_LOG("bts_context_alloc_object----\n");
	return obj;
}

int bts_notify()
{
	int err=0;
	int value=0;
	struct bts_context *cxt = NULL;
  	cxt = bts_context_obj;
	BTS_LOG("bts_notify++++\n");
	
	value = 1;
	input_report_rel(cxt->idev, EVENT_TYPE_BTS_VALUE, value);
	input_sync(cxt->idev); 
	
	return err;
}

static int bts_real_enable(int enable)
{
	int err =0;
	struct bts_context *cxt = NULL;
	cxt = bts_context_obj;

	if(BTS_RESUME == enable)
	{
		enable = resume_enable_status;
	}

	if(1==enable)
	{
		resume_enable_status = 1;
		if(atomic_read(&(bts_context_obj->early_suspend))) //not allow to enable under suspend
		{
			return 0;
		}
		if(false==cxt->is_active_data)
		{
			err = cxt->bts_ctl.open_report_data(1);
			if(err)
			{ 
				err = cxt->bts_ctl.open_report_data(1);
				if(err)
				{
					err = cxt->bts_ctl.open_report_data(1);
					if(err)
					{
						BTS_ERR("enable_bringtosee enable(%d) err 3 timers = %d\n", enable, err);
						return err;
					}
				}
			}
			cxt->is_active_data = true;
			BTS_LOG("enable_bringtosee real enable  \n" );
		}
	}
	else if((0==enable) || (BTS_SUSPEND == enable))
	{
		if(0 == enable)
			resume_enable_status = 0;
		if(true==cxt->is_active_data)
		{
			err = cxt->bts_ctl.open_report_data(0);
			if(err)
			{ 
				BTS_ERR("enable_bringtoseeenable(%d) err = %d\n", enable, err);
			}
			cxt->is_active_data =false;
			BTS_LOG("enable_bringtosee real disable  \n" );
		} 
	}
	return err;
}

int bts_enable_nodata(int enable)
{
	struct bts_context *cxt = NULL;
	cxt = bts_context_obj;
	if(NULL  == cxt->bts_ctl.open_report_data)
	{
		BTS_ERR("bts_enable_nodata:bts ctl path is NULL\n");
		return -1;
	}

	if(1 == enable)
	{
		cxt->is_active_nodata = true;
	}
	if(0 == enable)
	{
		cxt->is_active_nodata = false;
	}
	bts_real_enable(enable);
	return 0;
}

static ssize_t bts_show_enable_nodata(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	struct bts_context *cxt = NULL;
	cxt = bts_context_obj;
	
	BTS_LOG("bts active: %d\n", cxt->is_active_nodata);
	return snprintf(buf, PAGE_SIZE, "%d\n", cxt->is_active_nodata); 
}

static ssize_t bts_store_enable_nodata(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	struct bts_context *cxt = NULL;
	BTS_LOG("bts_store_enable nodata buf=%s\n",buf);
	mutex_lock(&bts_context_obj->bts_op_mutex);
	cxt = bts_context_obj;
	if(NULL == cxt->bts_ctl.open_report_data)
	{
		BTS_LOG("bts_ctl enable nodata NULL\n");
		mutex_unlock(&bts_context_obj->bts_op_mutex);
	 	return count;
	}
	if (!strncmp(buf, "1", 1))
	{
		bts_enable_nodata(1);
	}
	else if (!strncmp(buf, "0", 1))
	{
		bts_enable_nodata(0);
    	}
	else
	{
		BTS_ERR(" bts_store enable nodata cmd error !!\n");
	}
	mutex_unlock(&bts_context_obj->bts_op_mutex);
	return count;
}

static ssize_t bts_store_active(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	struct bts_context *cxt = NULL;
	int res =0;
	int en=0;
	BTS_LOG("bts_store_active buf=%s\n",buf);
	mutex_lock(&bts_context_obj->bts_op_mutex);
	
	cxt = bts_context_obj;
	if((res = sscanf(buf, "%d", &en))!=1)
	{
		BTS_LOG(" bts_store_active param error: res = %d\n", res);
	}
	BTS_LOG(" bts_store_active en=%d\n",en);
	if(1 == en)
	{
		bts_real_enable(1);
	}
	else if(0 == en)
	{
		bts_real_enable(0);
	}
	else
	{
		BTS_ERR(" bts_store_active error !!\n");
	}
	mutex_unlock(&bts_context_obj->bts_op_mutex);
	BTS_LOG(" bts_store_active done\n");
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t bts_show_active(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	struct bts_context *cxt = NULL;
	cxt = bts_context_obj;

	BTS_LOG("bts active: %d\n", cxt->is_active_data);
	return snprintf(buf, PAGE_SIZE, "%d\n", cxt->is_active_data); 
}

static ssize_t bts_store_delay(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count) 
{
	int len = 0;
	BTS_LOG(" not support now\n");
	return len;
}


static ssize_t bts_show_delay(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	int len = 0;
	BTS_LOG(" not support now\n");
	return len;
}


static ssize_t bts_store_batch(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	int len = 0;
	BTS_LOG(" not support now\n");
	return len;
}

static ssize_t bts_show_batch(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	int len = 0;
	BTS_LOG(" not support now\n");
	return len;
}

static ssize_t bts_store_flush(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	int len = 0;
	BTS_LOG(" not support now\n");
	return len;
}

static ssize_t bts_show_flush(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
    int len = 0;
	BTS_LOG(" not support now\n");
	return len;
}

static ssize_t bts_show_devnum(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	const char *devname = NULL;
	devname = dev_name(&bts_context_obj->idev->dev);
	return snprintf(buf, PAGE_SIZE, "%s\n", devname+5);  //TODO: why +5?
}
static int bringtosee_remove(struct platform_device *pdev)
{
	BTS_LOG("bringtosee_remove\n");
	return 0;
}

static int bringtosee_probe(struct platform_device *pdev) 
{
	BTS_LOG("bringtosee_probe\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id bringtosee_of_match[] = {
	{ .compatible = "mediatek,bringtosee", },
	{},
};
#endif

static struct platform_driver bringtosee_driver = {
	.probe      = bringtosee_probe,
	.remove     = bringtosee_remove,    
	.driver     = 
	{
		.name  = "bringtosee",
		#ifdef CONFIG_OF
		.of_match_table = bringtosee_of_match,
		#endif
	}
};

static int bts_real_driver_init(void) 
{
	int err=0;
	BTS_LOG(" bts_real_driver_init +\n");
	if(0 != bringtosee_init)
	{
		BTS_LOG(" bts try to init driver %s\n", bringtosee_init->name);
		err = bringtosee_init->init();
		if(0 == err)
		{
			BTS_LOG(" bts real driver %s probe ok\n", bringtosee_init->name);
		}
	}
	return err;
}

static int bts_misc_init(struct bts_context *cxt)
{
	int err=0;
	cxt->mdev.minor = MISC_DYNAMIC_MINOR;
	cxt->mdev.name  = BTS_MISC_DEV_NAME;
	if((err = misc_register(&cxt->mdev)))
	{
		BTS_ERR("unable to register bts misc device!!\n");
	}
	return err;
}

static void bts_input_destroy(struct bts_context *cxt)
{
	struct input_dev *dev = cxt->idev;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int bts_input_init(struct bts_context *cxt)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (NULL == dev)
		return -ENOMEM;

	dev->name = BTS_INPUTDEV_NAME;
	input_set_capability(dev, EV_REL, EVENT_TYPE_BTS_VALUE);
	
	input_set_drvdata(dev, cxt);
	set_bit(EV_REL, dev->evbit);
	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	cxt->idev= dev;

	return 0;
}

DEVICE_ATTR(btsenablenodata,     	S_IWUSR | S_IRUGO, bts_show_enable_nodata, bts_store_enable_nodata);
DEVICE_ATTR(btsactive,     		S_IWUSR | S_IRUGO, bts_show_active, bts_store_active);
DEVICE_ATTR(btsdelay,      		S_IWUSR | S_IRUGO, bts_show_delay,  bts_store_delay);
DEVICE_ATTR(btsbatch,      		S_IWUSR | S_IRUGO, bts_show_batch,  bts_store_batch);
DEVICE_ATTR(btsflush,      			S_IWUSR | S_IRUGO, bts_show_flush,  bts_store_flush);
DEVICE_ATTR(btsdevnum,      			S_IWUSR | S_IRUGO, bts_show_devnum,  NULL);


static struct attribute *bts_attributes[] = {
	&dev_attr_btsenablenodata.attr,
	&dev_attr_btsactive.attr,
	&dev_attr_btsdelay.attr,
	&dev_attr_btsbatch.attr,
	&dev_attr_btsflush.attr,
	&dev_attr_btsdevnum.attr,
	NULL
};

static struct attribute_group bts_attribute_group = {
	.attrs = bts_attributes
};

int bts_register_data_path(struct bts_data_path *data)
{
	struct bts_context *cxt = NULL;
	cxt = bts_context_obj;
	cxt->bts_data.get_data = data->get_data;
	if(NULL == cxt->bts_data.get_data)
	{
		BTS_LOG("bts register data path fail \n");
	 	return -1;
	}
	return 0;
}

int bts_register_control_path(struct bts_control_path *ctl)
{
	struct bts_context *cxt = NULL;
	int err =0;
	cxt = bts_context_obj;
//	cxt->bts_ctl.enable = ctl->enable;
//	cxt->bts_ctl.enable_nodata = ctl->enable_nodata;
	cxt->bts_ctl.open_report_data = ctl->open_report_data;
	
	if(NULL==cxt->bts_ctl.open_report_data)
	{
		BTS_LOG("bts register control path fail \n");
	 	return -1;
	}

	//add misc dev for sensor hal control cmd
	err = bts_misc_init(bts_context_obj);
	if(err)
	{
		BTS_ERR("unable to register bts misc device!!\n");
		return -2;
	}
	err = sysfs_create_group(&bts_context_obj->mdev.this_device->kobj,
			&bts_attribute_group);
	if (err < 0)
	{
		BTS_ERR("unable to create bts attribute file\n");
		return -3;
	}
	kobject_uevent(&bts_context_obj->mdev.this_device->kobj, KOBJ_ADD);
	return 0;	
}

static int bts_probe(struct platform_device *pdev) 
{
	int err;
	BTS_LOG("+++++++++++++bts_probe!!\n");

	bts_context_obj = bts_context_alloc_object();
	if (!bts_context_obj)
	{
		err = -ENOMEM;
		BTS_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}
	//init real bts driver
    	err = bts_real_driver_init();
	if(err)
	{
		BTS_ERR("bts real driver init fail\n");
		goto real_driver_init_fail;
	}

	//init input dev
	err = bts_input_init(bts_context_obj);
	if(err)
	{
		BTS_ERR("unable to register bts input device!\n");
		goto exit_alloc_input_dev_failed;
	}

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_EARLYSUSPEND)
   atomic_set(&(bts_context_obj->early_suspend), 0);
	bts_context_obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1,
	bts_context_obj->early_drv.suspend  = bts_early_suspend,
	bts_context_obj->early_drv.resume   = bts_late_resume,    
	register_early_suspend(&bts_context_obj->early_drv);
#endif //#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_EARLYSUSPEND)

	BTS_LOG("----bts_probe OK !!\n");
	return 0;


	if (err)
	{
	   BTS_ERR("sysfs node creation error \n");
	   bts_input_destroy(bts_context_obj);
	}
	real_driver_init_fail:
	exit_alloc_input_dev_failed:    
	kfree(bts_context_obj);
	exit_alloc_data_failed:
	BTS_LOG("----bts_probe fail !!!\n");
	return err;
}

static int bts_remove(struct platform_device *pdev)
{
	int err=0;
	BTS_FUN(f);
	input_unregister_device(bts_context_obj->idev);        
	sysfs_remove_group(&bts_context_obj->idev->dev.kobj,
				&bts_attribute_group);
	
	if((err = misc_deregister(&bts_context_obj->mdev)))
	{
		BTS_ERR("misc_deregister fail: %d\n", err);
	}
	kfree(bts_context_obj);
	return 0;
}

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_EARLYSUSPEND)
static void bts_early_suspend(struct early_suspend *h) 
{
	atomic_set(&(bts_context_obj->early_suspend), 1);
	if(!atomic_read(&bts_context_obj->wake)) //not wake up, disable in early suspend
	{
		bts_real_enable(BTS_SUSPEND);
	}
	BTS_LOG(" bts_early_suspend ok------->hwm_obj->early_suspend=%d \n",atomic_read(&(bts_context_obj->early_suspend)));
	return ;
}
/*----------------------------------------------------------------------------*/
static void bts_late_resume(struct early_suspend *h)
{
	atomic_set(&(bts_context_obj->early_suspend), 0);
	if(!atomic_read(&bts_context_obj->wake) && resume_enable_status) //not wake up, disable in early suspend
	{
		bts_real_enable(BTS_RESUME);
	}
	BTS_LOG(" bts_late_resume ok------->hwm_obj->early_suspend=%d \n",atomic_read(&(bts_context_obj->early_suspend)));
	return ;
}
#endif

static int bts_suspend(struct platform_device *dev, pm_message_t state) 
{
	atomic_set(&(bts_context_obj->suspend), 1);
	if(!atomic_read(&bts_context_obj->wake)) //not wake up, disable in early suspend
	{
		bts_real_enable(BTS_SUSPEND);
	}
	BTS_LOG(" bts_early_suspend ok------->hwm_obj->suspend=%d \n",atomic_read(&(bts_context_obj->suspend)));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int bts_resume(struct platform_device *dev)
{
	atomic_set(&(bts_context_obj->suspend), 0);
	if(!atomic_read(&bts_context_obj->wake) && resume_enable_status) //not wake up, disable in early suspend
	{
		bts_real_enable(BTS_RESUME);
	}
	BTS_LOG(" bts_resume ok------->hwm_obj->suspend=%d \n",atomic_read(&(bts_context_obj->suspend)));
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id m_bts_pl_of_match[] = {
	{ .compatible = "mediatek,m_bts_pl", },
	{},
};
#endif

static struct platform_driver bts_driver =
{
	.probe      = bts_probe,
	.remove     = bts_remove,    
#if !defined(CONFIG_HAS_EARLYSUSPEND) || !defined(USE_EARLY_SUSPEND)
	.suspend    = bts_suspend,
	.resume     = bts_resume,
#endif
	.driver     = 
	{
		.name = BTS_PL_DEV_NAME,
		#ifdef CONFIG_OF
		.of_match_table = m_bts_pl_of_match,
		#endif
	}
};

int bts_driver_add(struct bts_init_info* obj) 
{
	int err=0;
	
	BTS_FUN();
	BTS_LOG("register bringtosee driver for the first time\n");
	if(platform_driver_register(&bringtosee_driver))
	{
		BTS_ERR("failed to register gensor driver already exist\n");
	}
	if(NULL == bringtosee_init)
	{
		obj->platform_diver_addr = &bringtosee_driver;
		bringtosee_init = obj;
	}

	if(NULL==bringtosee_init)
	{
		BTS_ERR("BTS driver add err \n");
		err=-1;
	}
	
	return err;
}
EXPORT_SYMBOL_GPL(bts_driver_add);

static int __init bts_init(void) 
{
	BTS_FUN();

	if(platform_driver_register(&bts_driver))
	{
		BTS_ERR("failed to register bts driver\n");
		return -ENODEV;
	}
	
	return 0;
}

static void __exit bts_exit(void)
{
	platform_driver_unregister(&bts_driver); 
	platform_driver_unregister(&bringtosee_driver);      
}

late_initcall(bts_init);
//module_init(bts_init);
//module_exit(bts_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BTS device driver");
MODULE_AUTHOR("Mediatek");

