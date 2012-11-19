/*
 * MVF IPC support
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
#include <mach/mvf-ipc.h>

#include <asm/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>

static __iomem void *mscm_base = MVF_IO_ADDRESS(MVF_MSCM_BASE_ADDR);
struct task_struct *user_pid[MAX_MVF_CPU_TO_CPU_INTERRUPTS];

void signal_userspace(int interrupt_id)
{
	struct siginfo info;
	memset(&info, 0, sizeof(struct siginfo));
	info.si_signo = SIG_CPU_TO_CPU_INT;
	info.si_code = SI_MESGQ;
	info.si_int = interrupt_id;
	/* TODO: Do we need to check for the return value and reschedule the ISR? */
	send_sig_info(SIG_CPU_TO_CPU_INT, &info, user_pid[interrupt_id]);    //send the signal
}

static irqreturn_t cpu_to_cpu_irq_handler(int irq, void *dev_id)
{
	int interrupt_id = irq - MVF_INT_CPU_INT0;
	//Clear the interrupt status
	MSCM_WRITE((MSCM_IRCPnIR_INT0_MASK<<interrupt_id), MSCM_IRCPnIR);

	if(user_pid[interrupt_id] != NULL)
		signal_userspace(interrupt_id);

	return IRQ_HANDLED;
}

void generate_cpu_to_cpu_interrupt(cpu_interrupt_info_t * interrupt_info)
{
	u32 data;
	data = (interrupt_info->interrupt_id << MSCM_IRCPGIR_INTID_SHIFT);
	if(interrupt_info->interrupt_type == INTERRUPT_ALL_CORES_EXCEPT_REQUESTOR)
	{
		data |= 0x1 << MSCM_IRCPGIR_TLF_SHIFT;
	}
	else if(interrupt_info->interrupt_type == INTERRUPT_REQUESTING_CORE)
	{
		data |= 0x2 << MSCM_IRCPGIR_TLF_SHIFT;
	}
	else
	{
		data |=	(interrupt_info->interrupt_type << MSCM_IRCPGIR_CPUTL_SHIFT);
	}

	MSCM_WRITE(data, MSCM_IRCPGIR);
}

static long mvf_ipc_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case MVF_GENERATE_CPU_TO_CPU_INTERRUPT:
		{
			cpu_interrupt_info_t interrupt_info;

			if (copy_from_user(&interrupt_info, (struct cpu_interrupt_info_t *) arg,
								 sizeof(cpu_interrupt_info_t)))
				return -EFAULT;

			if(interrupt_info.interrupt_id >= MAX_MVF_CPU_TO_CPU_INTERRUPTS)
				return -EINVAL;
			generate_cpu_to_cpu_interrupt(&interrupt_info);
		}
		break;
	case MVF_REGISTER_PID_WITH_CPU_INTERRUPT:
		{
			u8 __user *argp = (void __user *)arg;
			u8 interrupt_id;
			if (get_user(interrupt_id, argp))
				return -EFAULT;

			if(interrupt_id >= MAX_MVF_CPU_TO_CPU_INTERRUPTS)
				return -EINVAL;

			if(put_user(SIG_CPU_TO_CPU_INT,argp))
				return -EFAULT;

			user_pid[interrupt_id] = current;
		}
		break;
	default:
		return -EINVAL;
		break;
	}
	return ret;
}

static struct file_operations mvf_ipc_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mvf_ipc_ioctl,
};

static struct miscdevice mvf_ipc_dev = {
	MISC_DYNAMIC_MINOR,
	IPC_DRIVER_NAME,
	&mvf_ipc_fops
};

static int __init mvf_ipc_init(void)
{
	int ret;
	u8 count;

	for(count=0; count < MAX_MVF_CPU_TO_CPU_INTERRUPTS; count++)
	{
		user_pid[count] = NULL;
		//Register the interrupt handler
		if (request_irq(MVF_INT_CPU_INT0 + count, cpu_to_cpu_irq_handler, 0, IPC_DRIVER_NAME, mscm_base) != 0)
		{
			printk("Failed to register MVF CPU TO CPU interrupt:%d\n",count);
			return -EIO;
		}
	}

	ret = misc_register(&mvf_ipc_dev);
	if (ret) {
		printk(KERN_ERR "mvf_ipc: can't be registerd as misc_register\n");
		return ret;
	}

	printk("MVF_IPC driver initialized\n");

	return 0;
}

static void __exit mvf_ipc_exit(void)
{
	u8 count;
	for(count=0; count < MAX_MVF_CPU_TO_CPU_INTERRUPTS; count++)
	{
		free_irq(MVF_INT_CPU_INT0 + count, mscm_base);
	}
	misc_deregister(&mvf_ipc_dev);
}

module_init(mvf_ipc_init);
module_exit(mvf_ipc_exit);
