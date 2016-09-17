/*
* Copyright (C) 2014 Invensense, Inc.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "icm30628_interrupt.h"

#ifdef MTK_PLATFORM
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>
#include <mach/eint.h>
#ifdef MTK_EINT_ENABLE
struct work_struct eint_normal_work;
#endif
#ifdef MTK_EINT_WAKE_ENABLE
struct work_struct eint_wakeup_work;
#endif
#endif


#ifdef MTK_PLATFORM
int invensense_interrupt_normal_initialize(int *irq, void * id,  irqreturn_t (*irq_handler)(int , void *),  void (*thread_handler)(struct work_struct *))
#else
int invensense_interrupt_normal_initialize(int *irq, void * id,  irqreturn_t (*irq_handler)(int , void *),  irqreturn_t (*thread_handler)(int , void *))
#endif
{
	int ret = 0; 
#ifdef MTK_EINT_ENABLE
	int level;
#endif

	INV_DBG_FUNC_NAME;
#ifdef MTK_PLATFORM
#ifdef MTK_EINT_ENABLE
	if (CUST_EINT_SENSORHUB_NUM >= 0) {
		INV_INFO("normal eint = %d gpio = %d levle = %d\n", CUST_EINT_SENSORHUB_NUM, GPIO_SENSORHUB_EINT_PIN, level);
		mt_set_gpio_mode(GPIO_SENSORHUB_EINT_PIN, GPIO_MODE_00); //read level first
		mt_set_gpio_dir(GPIO_SENSORHUB_EINT_PIN, GPIO_DIR_IN);
		level = mt_get_gpio_in(GPIO_SENSORHUB_EINT_PIN);
		
		mt_set_gpio_mode(GPIO_SENSORHUB_EINT_PIN, GPIO_SENSORHUB_EINT_PIN_M_EINT);
		//mt_set_gpio_dir(GPIO_SENSORHUB_EINT_PIN, GPIO_DIR_IN);
		mt_set_gpio_pull_enable(GPIO_SENSORHUB_EINT_PIN, GPIO_PULL_ENABLE);
		mt_set_gpio_pull_select(GPIO_SENSORHUB_EINT_PIN, 0);
		
		INV_INFO("normal eint = %d gpio = %d levle = %d\n", CUST_EINT_SENSORHUB_NUM, GPIO_SENSORHUB_EINT_PIN, level);
		mt_eint_registration(CUST_EINT_SENSORHUB_NUM, EINTF_TRIGGER_RISING, irq_handler, 1);

		mt_eint_mask(CUST_EINT_SENSORHUB_NUM);
		INIT_WORK(&eint_normal_work, thread_handler);
		mt_eint_unmask(CUST_EINT_SENSORHUB_NUM);
    //enable_irq_wake(CUST_EINT_SENSORHUB_NUM); 
	}
#endif
#else
	*irq = gpio_to_irq(INV_IRQ_NORMAL);
	
	ret = request_threaded_irq(*irq, 
		irq_handler, 
		thread_handler, 
#if (FIRMWARE_VERSION >= 230)
		IRQF_TRIGGER_RISING | IRQF_SHARED | IRQF_ONESHOT,		
#else
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_SHARED, 
#endif		
		"inv_irq_normal", 
		id);	
	if(UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	enable_irq_wake(*irq); 
#endif

	return ret;
}

#ifdef MTK_PLATFORM
int invensense_interrupt_wakeup_initialize(int *irq, void * id,  irqreturn_t (*irq_handler)(int , void *),  void (*thread_handler)(struct work_struct *))
#else
int invensense_interrupt_wakeup_initialize(int *irq, void * id,  irqreturn_t (*irq_handler)(int , void *),  irqreturn_t (*thread_handler)(int , void *))
#endif
{
	int ret = 0;

	INV_DBG_FUNC_NAME;

#ifdef MTK_PLATFORM
#ifdef MTK_EINT_WAKE_ENABLE
	if (CUST_EINT_SENSORHUB_WAKE_UP_NUM >= 0) {
		mt_set_gpio_mode(GPIO_SENSORHUB_WAKE_UP, GPIO_SENSORHUB_WAKE_UP_M_EINT);
		mt_set_gpio_dir(GPIO_SENSORHUB_WAKE_UP, GPIO_DIR_IN);
		mt_set_gpio_pull_enable(GPIO_SENSORHUB_WAKE_UP, GPIO_PULL_ENABLE);
		mt_set_gpio_pull_select(GPIO_SENSORHUB_WAKE_UP, 0);
		
		mt_eint_registration(CUST_EINT_SENSORHUB_WAKE_UP_NUM, EINTF_TRIGGER_RISING, irq_handler, 1);

		mt_eint_mask(CUST_EINT_SENSORHUB_WAKE_UP_NUM);
		INIT_WORK(&eint_wakeup_work, thread_handler);
		mt_eint_unmask(CUST_EINT_SENSORHUB_WAKE_UP_NUM);
    //enable_irq_wake(CUST_EINT_SENSORHUB_WAKE_UP_NUM); 
	}

#endif
#else
	*irq = gpio_to_irq(INV_IRQ_WAKEUP);
	
	ret = request_threaded_irq(*irq, 
		irq_handler, 
		thread_handler, 
#if (FIRMWARE_VERSION >= 230)
		IRQF_TRIGGER_RISING | IRQF_SHARED | IRQF_ONESHOT,				
#else
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_SHARED, 
#endif		
		"inv_irq_wakeup", 
		id);	
	if(UNLIKELY(ret < 0)) {
		INV_ERR;
		return ret;
	}

	enable_irq_wake(*irq); 
#endif

	return ret;
}

