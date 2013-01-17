/*
 * Copyright 2013 Freescale Semiconductor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

// added by ed
#include <linux/slab.h>
#include <mach/hardware.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

// common to MQX and Linux
// TODO the order of these should not matter
#include "mcc_config.h"
#include "mcc_common.h"

// linux only
#include "mcc_linux.h"
#include "mcc_shm_linux.h"
#include "mcc_sema4_linux.h"

// ************************************ Local Data *************************************************

typedef enum write_mode_enum {MODE_IMAGE_LOAD, MODE_MCC} WRITE_MODE;

#define MAX_LOAD_SIZE (128*1024)

struct mcc_private_data
{
	int idx;
	MCC_ENDPOINT recv_endpoint;
	MCC_ENDPOINT send_endpoint;
	WRITE_MODE write_mode;
	struct mqx_boot_info_struct mqx_boot_info;
	char *virt_load_addr;
};

static dev_t first;
static struct cdev c_dev;
static struct class *cl;

static __iomem void *mscm_base = MVF_IO_ADDRESS(MVF_MSCM_BASE_ADDR);

DECLARE_WAIT_QUEUE_HEAD(wait_queue);

static int other_cores[] = MCC_OTHER_CORES;

// ************************************ Utility functions *************************************************

static int signal_queued(MCC_ENDPOINT endpoint)
{
	MCC_SIGNAL signal;

	signal.type = BUFFER_QUEUED;
	signal.destination.core = endpoint.core;
	signal.destination.node = endpoint.node;
	signal.destination.port = endpoint.port;

	// add to the signal queue
	if(mcc_queue_signal(endpoint.core, signal))
		return -ENOMEM;

	// send BUFFER_QUEUED to receiveing core
	MSCM_WRITE(MCC_INTERRUPT(endpoint.core), MSCM_IRCPGIR);

	return MCC_SUCCESS;
}

static int signal_freed(void)
{
	MCC_SIGNAL signal;
	int i;

	signal.type = BUFFER_FREED;

	// signal all other cores
	for(i=0; i<sizeof(other_cores)/sizeof(other_cores[0]); i++)
	{
		signal.destination.core = other_cores[i]; // node and port are n/a
		mcc_queue_signal(signal.destination.core, signal);

		// send BUFFER_QUEUED to receiveing core
		MSCM_WRITE(MCC_INTERRUPT(signal.destination.core), MSCM_IRCPGIR);
	}

	return MCC_SUCCESS;
}

// ************************************ Interrupt handler *************************************************

static irqreturn_t cpu_to_cpu_irq_handler(int irq, void *dev_id)
{
	MCC_SIGNAL signal;

	int interrupt_id = irq - MVF_INT_CPU_INT0;

	wake_up_interruptible(&wait_queue);

	// TODO order processing based on order in signal queue
	// purger the signal queue
	while(mcc_dequeue_signal(MCC_CORE_NUMBER, &signal));

	//Clear the interrupt status
	MSCM_WRITE((MSCM_IRCPnIR_INT0_MASK<<interrupt_id), MSCM_IRCPnIR);

	return IRQ_HANDLED;
}

// ************************************ Driver entry points *************************************************

static int mcc_open(struct inode *i, struct file *f)
{

	struct mcc_private_data *priv_p = (struct mcc_private_data *)kmalloc(sizeof(struct mcc_private_data), GFP_KERNEL);
	if(!priv_p)
		return -ENOMEM;
	memset(priv_p, 0, sizeof(struct mcc_private_data));

	priv_p->write_mode = MODE_MCC;

	f->private_data = priv_p;

	return MCC_SUCCESS;
}

static int mcc_close(struct inode *i, struct file *f)
{
	struct mcc_private_data *priv_p = f->private_data;

	if(priv_p->write_mode == MODE_IMAGE_LOAD)
	{
		iounmap(priv_p->virt_load_addr);
		release_mem_region(priv_p->mqx_boot_info.phys_load_addr, MAX_LOAD_SIZE);
	}

	if(f->private_data)
		kfree(f->private_data);
	return MCC_SUCCESS;
}

static ssize_t mcc_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	struct mcc_private_data *priv_p = f->private_data;
	MCC_RECEIVE_BUFFER * buffer;
	MCC_RECEIVE_LIST * recv_list;
	int copy_len = 0;

	// get my receive list
	if(mcc_sema4_grab(MCC_SEMAPHORE_NUMBER))
		return -EBUSY;
	if(!(recv_list = mcc_get_endpoint_list(priv_p->recv_endpoint)))
	{
		mcc_sema4_release(MCC_SEMAPHORE_NUMBER);
		return -EINVAL;
	}
	mcc_sema4_release(MCC_SEMAPHORE_NUMBER);

	// block unless asked not to
	if(!(f->f_flags & O_NONBLOCK))
	{
		if(wait_event_interruptible(wait_queue, recv_list->head))
			return -ERESTARTSYS;
	}

	// dequeue the buffer (if any)
	if(mcc_sema4_grab(MCC_SEMAPHORE_NUMBER))
		return -EBUSY;
	buffer = mcc_dequeue_buffer(recv_list);
	mcc_sema4_release(MCC_SEMAPHORE_NUMBER);

	if(buffer)
	{
		// dont copy more than will fit
		copy_len = buffer->data_len < len ? buffer->data_len : len;
		
		// load the data
		if (copy_to_user(buf, &buffer->data, copy_len))
			return -EFAULT;
		
		if(mcc_sema4_grab(MCC_SEMAPHORE_NUMBER))
			return -EBUSY;
		// free the buffer
		mcc_queue_buffer(&bookeeping_data->free_list, buffer);

		// signal all other cores a buffer has been made available
		if(signal_freed())
		{
			mcc_sema4_release(MCC_SEMAPHORE_NUMBER);
			return -ENOMEM;
		}

		mcc_sema4_release(MCC_SEMAPHORE_NUMBER);

		// wake yourself up again in case anyone here waiting for free buf
		//wake_up_interruptible(&wait_queue);
	}

	return copy_len;
}

static ssize_t mcc_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	struct mcc_private_data *priv_p = f->private_data;
	MCC_RECEIVE_BUFFER * buffer;
	MCC_RECEIVE_LIST * send_list;

	if(priv_p->write_mode == MODE_IMAGE_LOAD)
	{
		if (copy_from_user(priv_p->virt_load_addr + f->f_pos, buf, len))
			return -EFAULT;

		*off = f->f_pos + len;
		return len;
	}

	// not too big
	if(len > sizeof(buffer->data))
		return -EINVAL;

	// get the target endpoint
	if(mcc_sema4_grab(MCC_SEMAPHORE_NUMBER))
		return -EBUSY;
	if(!(send_list = mcc_get_endpoint_list(priv_p->send_endpoint)))
	{
		mcc_sema4_release(MCC_SEMAPHORE_NUMBER);
		return -EINVAL;
	}
	mcc_sema4_release(MCC_SEMAPHORE_NUMBER);

	// block unless asked not to
	if(!(f->f_flags & O_NONBLOCK))
	{
		if(wait_event_interruptible(wait_queue, bookeeping_data->free_list.head))
		return -ERESTARTSYS;
	}

	// get a free data buffer
	if(mcc_sema4_grab(MCC_SEMAPHORE_NUMBER))
		return -EBUSY;
	if(null == (buffer = mcc_dequeue_buffer(&bookeeping_data->free_list)))
	{
		mcc_sema4_release(MCC_SEMAPHORE_NUMBER);
		return -ENOMEM;
	}
	mcc_sema4_release(MCC_SEMAPHORE_NUMBER);

	// load the data
	if (copy_from_user(&buffer->data, buf, len))
		return -EFAULT;
	buffer->data_len = len;

	if(mcc_sema4_grab(MCC_SEMAPHORE_NUMBER))
		return -EBUSY;
	// queue it
	mcc_queue_buffer(send_list, buffer);

	// signal the other side a buffer has been made available
	if(signal_queued(priv_p->send_endpoint))
	{
		mcc_sema4_release(MCC_SEMAPHORE_NUMBER);
		return -ENOMEM;
	}

	mcc_sema4_release(MCC_SEMAPHORE_NUMBER);

	return len;
}

static long mcc_ioctl(struct file *f, unsigned cmd, unsigned long arg)
{
	struct mcc_private_data *priv_p = f->private_data;
	void __user *buf = (void __user *)arg;
	MCC_ENDPOINT endpoint;
	MCC_ENDPOINT *endpoint_p;

	int retval = 0;

	switch(cmd)
	{

	case MCC_CREATE_ENDPOINT:
	case MCC_DESTROY_ENDPOINT:
		if (copy_from_user(&endpoint, buf, sizeof(endpoint)))
			return -EFAULT;

		if(mcc_sema4_grab(MCC_SEMAPHORE_NUMBER))
			return -EBUSY;

		retval = cmd == MCC_CREATE_ENDPOINT ? mcc_register_endpoint(endpoint) : mcc_remove_endpoint(endpoint);

		mcc_sema4_release(MCC_SEMAPHORE_NUMBER);
		break;

	case MCC_SET_RECEIVE_ENDPOINT:
	case MCC_SET_SEND_ENDPOINT:
		if (copy_from_user(&endpoint, buf, sizeof(endpoint)))
			return -EFAULT;

		// has it been registered ?
		if(!mcc_get_endpoint_list(endpoint))
			return -EINVAL;

		endpoint_p = cmd == MCC_SET_SEND_ENDPOINT ? &priv_p->send_endpoint : &priv_p->recv_endpoint;

		endpoint_p->core = endpoint.core;
		endpoint_p->node = endpoint.node;
		endpoint_p->port = endpoint.port;
		break;

	case MCC_SET_MODE_LOAD_MQX_IMAGE:
		if (copy_from_user(&priv_p->mqx_boot_info, buf, sizeof(priv_p->mqx_boot_info)))
			return -EFAULT;

		if (request_mem_region(priv_p->mqx_boot_info.phys_load_addr, MAX_LOAD_SIZE, "mqx_boot"))
		{
			priv_p->virt_load_addr = ioremap_nocache(priv_p->mqx_boot_info.phys_load_addr, MAX_LOAD_SIZE);
			if (!priv_p->virt_load_addr)
			{
				printk(KERN_ERR "unable to map region\n");
				release_mem_region(priv_p->mqx_boot_info.phys_load_addr, MAX_LOAD_SIZE);
				return -ENOMEM;
			}
		}
		else
		{
			printk(KERN_ERR "unable to reserve imaqge load memory region\n");
			return -ENOMEM;
		}

		priv_p->write_mode = MODE_IMAGE_LOAD;
		break;

	case  MCC_BOOT_MQX_IMAGE:
		//0x4006E028 - GPR2 Register write Image Entry Point as per the Memory Map File of the Binary
		writel(priv_p->mqx_boot_info.phys_start_addr, MVF_IO_ADDRESS(0x4006E028));
		// 0x4006B08C - CCM_CCOWR Register - Set bit 16 - AUX_CORE_WKUP to enable M4 clock.
		writel(0x15a5a, MVF_IO_ADDRESS(0x4006B08C));
		break;

	default:
		printk(KERN_ERR "Unknown ioctl command (0x%08X)\n", cmd);
		retval = -ENOIOCTLCMD;
	}

	return retval;
}

static struct file_operations mcc_fops =
{
	.owner = THIS_MODULE,
	.open = mcc_open,
	.release = mcc_close,
	.read = mcc_read,
	.write = mcc_write,
	.unlocked_ioctl = mcc_ioctl,
};
 
static int __init mcc_init(void) /* Constructor */
{
	int count;

	if (alloc_chrdev_region(&first, 0, 1, "mcc") < 0)
	{
		return -EIO;
	}

	if ((cl = class_create(THIS_MODULE, "mcc")) == NULL)
	{
		unregister_chrdev_region(first, 1);
		return -EIO;
	}

	if (device_create(cl, NULL, first, NULL, "mcc") == NULL)
	{
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -EIO;
	}

	cdev_init(&c_dev, &mcc_fops);
	if (cdev_add(&c_dev, first, 1) == -1)
	{
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -EIO;
	}

	if(mcc_map_shared_memory())
	{
		cdev_del(&c_dev);
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -EIO;
	}

	// Initialize shared memory
	if(mcc_initialize_shared_mem())
		return -EIO;

	//Register the interrupt handler
	for(count=0; count < MAX_MVF_CPU_TO_CPU_INTERRUPTS; count++)
	{
		if (request_irq(MVF_INT_CPU_INT0 + count, cpu_to_cpu_irq_handler, 0, MCC_DRIVER_NAME, mscm_base) != 0)
		{
			printk(KERN_ERR "Failed to register MVF CPU TO CPU interrupt:%d\n",count);

			// free the shared mem
			mcc_deinitialize_shared_mem();

			return -EIO;
		}
	}

print_bookeeping_data();
	printk(KERN_INFO "mcc registered major=%d, sizeof(struct mcc_bookeeping_struct)=%d\n",MAJOR(first), sizeof(struct mcc_bookeeping_struct));
	return MCC_SUCCESS;
}
 
static void __exit mcc_exit(void) /* Destructor */
{
	int count;
	// make sure to let go of interrupts and shmem
	for(count=0; count < MAX_MVF_CPU_TO_CPU_INTERRUPTS; count++)
		free_irq(MVF_INT_CPU_INT0 + count, mscm_base);
	mcc_deinitialize_shared_mem();

	cdev_del(&c_dev);
	device_destroy(cl, first);
	class_destroy(cl);
	unregister_chrdev_region(first, 1);

	printk(KERN_INFO "mcc unregistered\n");
}
 
module_init(mcc_init);
module_exit(mcc_exit);

MODULE_LICENSE("GPL");


