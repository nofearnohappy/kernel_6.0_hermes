/*
 * Copyright (C) 2012 Invensense, Inc.
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

#ifndef ICM30628_INTERRUPT_H
#define ICM30628_INTERRUPT_H
#include <linux/irq.h>
#include <linux/interrupt.h>
#include "icm30628.h"
#include "icm30628_debug.h"

#define INV_IRQ_NORMAL 29 // GPIO 1
#define INV_IRQ_WAKEUP 73 // GPIO 0

#ifdef MTK_PLATFORM
int invensense_interrupt_normal_initialize(int *irq, void * id,  irqreturn_t (*irq_handler)(int , void *),  void (*thread_handler)(struct work_struct *));
int invensense_interrupt_wakeup_initialize(int *irq, void * id,  irqreturn_t (*irq_handler)(int , void *),  void (*thread_handler)(struct work_struct *));
#else
int invensense_interrupt_normal_initialize(int *irq, void * id,  irqreturn_t (*irq_handler)(int , void *),  irqreturn_t (*thread_handler)(int , void *));
int invensense_interrupt_wakeup_initialize(int *irq, void * id,  irqreturn_t (*irq_handler)(int , void *),  irqreturn_t (*thread_handler)(int , void *));
#endif
#endif /* ICM30628_INTERRUPT_H */

