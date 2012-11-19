/*
 * MVF Shared IRAM support
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

#include <linux/init.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>

#define SHARED_IRAM_START (0x3F040000)

static int shared_iram_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	//TODO: Check for the valid size!!!

	ret = remap_pfn_range(vma,
		vma->vm_start,
		SHARED_IRAM_START >> PAGE_SHIFT,
		vma->vm_end-vma->vm_start,
		vma->vm_page_prot);

	if (ret != 0)
		return -EAGAIN;

	return 0;
}

static const struct file_operations shared_iram_fops = {
	.owner			= THIS_MODULE,
	.mmap			= shared_iram_mmap,
};

static struct miscdevice shared_iram_dev = {
	MISC_DYNAMIC_MINOR,
	"mvf_shared_iram",
	&shared_iram_fops
};

static int __init shared_iram_init(void)
{
	int ret;

	ret = misc_register(&shared_iram_dev);
	if (ret) {
		printk(KERN_ERR "shared_iram: can't be registerd as misc_register\n");
		return ret;
	}
	printk(KERN_INFO "SHARED_IRAM device registered\n");
	return 0;
}


static void __exit shared_iram_cleanup_module(void)
{
	misc_deregister(&shared_iram_dev);
}

module_init(shared_iram_init);
module_exit(shared_iram_cleanup_module);
