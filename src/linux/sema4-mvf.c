/*
 * MVF SEMA4 support
 * Copyright 2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <mach/hardware.h>
#include <asm-generic/bug.h>
#include <asm/mach/irq.h>
#include <linux/delay.h>
#include <mach/mvf-sema4.h>

static DEFINE_MUTEX(sema4_mutex);
static __iomem void *sema4_base;

/* TODO: MAX_MVF_SEMA4_GATES should be based on SOC? */
struct completion gate_unlock_by_linux[MAX_MVF_SEMA4_GATES];

#ifdef USE_SEMA4_INTERRUPT

struct completion gate_unlock_by_other_os;

static irqreturn_t sema4_irq_handler(int irq, void *dev_id)
{
	complete(&gate_unlock_by_other_os);
	return IRQ_HANDLED;
}
#endif

u8 sema4_spinlock(u8 gate)
{
	u8 retval;
	u8 gate_status;

	mutex_lock(&sema4_mutex);

	gate_status = SEMA4_GATEn_READ(gate);

	/* If gate is already locked by a process on A5 then wait till
	 * the gate is unlocked by the other process and try obtain a mutex lock
	 */
	if(gate_status == SEMA4_GATE_LOCKED_BY_LINUX)
	{
		do
		{
			/* Unlock the mutex so that the other A5 process can unlock
			 * the gate. Wait for unlock completion
			 */
			mutex_unlock(&sema4_mutex);
			wait_for_completion(&gate_unlock_by_linux[gate]);

			/* Obtain a mutex lock and check if some other process on A5 snuck
			 * in to obtain a gate lock
			 */
			mutex_lock(&sema4_mutex);
			gate_status = SEMA4_GATEn_READ(gate);
		}while(gate_status == SEMA4_GATE_LOCKED_BY_LINUX);
	}

	/* Once mutex lock is obtained and we check gate is not locked by A5 we
	 * can proceed to obtain a lock. Only M4 can sneak in at this point.
	 */

	do{

#ifdef USE_SEMA4_INTERRUPT
		if(gate_status == SEMA4_GATE_LOCKED_BY_OTHER_OS)
		{
			init_completion(&gate_unlock_by_other_os);
			SEMA4_GATEn_ENABLE_INTERRUPT(gate);
			wait_for_completion(&gate_unlock_by_other_os);
		}
#endif

		SEMA4_GATEn_WRITE((u8)SEMA4_GATE_LOCKED_BY_LINUX, gate);
		gate_status = SEMA4_GATEn_READ(gate);
	}while(gate_status != SEMA4_GATE_LOCKED_BY_LINUX);

	init_completion(&gate_unlock_by_linux[gate]);
	mutex_unlock(&sema4_mutex);
	retval = MVF_SEMA4_LOCK_SUCCESS;
	return retval;
}

u8 sema4_trylock(u8 gate)
{
	u8 retval;
	u8 gate_status;

	mutex_lock(&sema4_mutex);

	gate_status = SEMA4_GATEn_READ(gate);

	if(gate_status == SEMA4_GATE_UNLOCKED)
	{
		SEMA4_GATEn_WRITE((u8)SEMA4_GATE_LOCKED_BY_LINUX, gate);
		gate_status = SEMA4_GATEn_READ(gate);

		/* Check if we obtained lock or did M4 sneak in! */
		if(gate_status == SEMA4_GATE_LOCKED_BY_LINUX)
		{
			init_completion(&gate_unlock_by_linux[gate]);
			mutex_unlock(&sema4_mutex);
			retval = MVF_SEMA4_LOCK_SUCCESS;
			return retval;
		}
	}

	mutex_unlock(&sema4_mutex);

	if(gate_status == SEMA4_GATE_LOCKED_BY_LINUX)
	{
		retval = MVF_SEMA4_LOCK_FAIL_LOCK_HELD_BY_LINUX;
	}
	else if(gate_status == SEMA4_GATE_LOCKED_BY_OTHER_OS)
	{
		retval = MVF_SEMA4_LOCK_FAIL_LOCK_HELD_BY_OTHER_OS;
	}
	else
	{
		retval = MVF_SEMA4_LOCK_FAIL_GATE_STATE_UNLOCKED;
	}
	return retval;
}

u8 sema4_unlock(u8 gate)
{
	u8 retval = MVF_SEMA4_UNLOCK_FAIL_LOCK_NOT_HELD;
	u8 gate_status;

	mutex_lock(&sema4_mutex);

	gate_status = SEMA4_GATEn_READ(gate);

	if(gate_status == SEMA4_GATE_LOCKED_BY_LINUX)
	{
		SEMA4_GATEn_WRITE((u8)SEMA4_GATE_UNLOCKED, gate);
		retval = MVF_SEMA4_LOCK_SUCCESS;
		complete(&gate_unlock_by_linux[gate]);
	}

	mutex_unlock(&sema4_mutex);
	return retval;
}

static long mvf_sema4_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int __user *argp = (void __user *)arg;
	u8 gate;
	long result;

	if (get_user(gate, argp))
		return -EFAULT;

	if(gate >= MAX_MVF_SEMA4_GATES)
		return -EINVAL;

	switch (cmd) {
	case MVF_SEMA4_LOCK:
		{
			result = sema4_spinlock(gate);
			break;
		}
	case MVF_SEMA4_UNLOCK:
		{
			result = sema4_unlock(gate);
			break;
		}
	case MVF_SEMA4_TRYLOCK:

		{
			result = sema4_trylock(gate);
			break;
		}
	default:
		return -EINVAL;
		break;
	}
	return result;
}

static struct file_operations mvf_sema4_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mvf_sema4_ioctl,
};

static struct miscdevice mvf_sema4_dev = {
	MISC_DYNAMIC_MINOR,
	SEMA4_DRIVER_NAME,
	&mvf_sema4_fops
};

static int __init mvf_sema4_init(void)
{
	int ret;

	ret = misc_register(&mvf_sema4_dev);
	if (ret) {
		printk(KERN_ERR "mvf_sema4: can't be registerd as misc_register\n");
		return ret;
	}

	sema4_base = MVF_IO_ADDRESS(MVF_SEMA4_BASE_ADDR);

#ifdef USE_SEMA4_INTERRUPT
	/* TODO: if this section is enabled, handle unregistering the device appropriately */
	if (request_irq(MVF_INT_SEMA4, sema4_irq_handler, 0, SEMA4_DRIVER_NAME, sema4_base) != 0)
	{
		printk("MVF SEMA4 interrupt request failed\n");
	}
#endif
	printk("MVF_SEMA4 driver initialized\n");
	return 0;
}

static void __exit mvf_sema4_exit(void)
{
#ifdef USE_SEMA4_INTERRUPT
	free_irq(MVF_INT_SEMA4,sema4_base);
#endif
	misc_deregister(&mvf_sema4_dev);
}

module_init(mvf_sema4_init);
module_exit(mvf_sema4_exit);

