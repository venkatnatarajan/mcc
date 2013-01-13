/*
  * Copyright 2013 Freescale, Inc.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  *
  * - Redistributions of source code must retain the above copyright
  *   notice, this list of conditions and the following disclaimer.
  * - Redistributions in binary form must reproduce the above copyright
  *   notice, this list of conditions and the following disclaimer in the
  *   documentation and/or other materials provided with the
  *   distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
  * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  * POSSIBILITY OF SUCH DAMAGE.
  */
#ifndef __MCC_LINUX_H__
#define __MCC_LINUX_H__

#include <linux/ioctl.h>

#define MCC_DRIVER_NAME ("mcc")

// TODO is this where this should go?
#define SHARED_IRAM_START (MVF_IRAM_BASE_ADDR + 0x00040000)
#define SHARED_IRAM_SIZE (64*1024)

// ioctls
#define MCC_CREATE_ENDPOINT						_IOW('M', 1, MCC_ENDPOINT)
#define MCC_DESTROY_ENDPOINT						_IOW('M', 2, MCC_ENDPOINT)
#define MCC_SET_RECEIVE_ENDPOINT					_IOW('M', 3, MCC_ENDPOINT)
#define MCC_SET_SEND_ENDPOINT						_IOW('M', 4, MCC_ENDPOINT)

/* sets the load adress and subsequent writes will be to load data there */
struct mqx_boot_info_struct {
	unsigned int phys_load_addr;
	unsigned int phys_start_addr;
};

#define MCC_SET_MODE_LOAD_MQX_IMAGE					_IOW('M', 5, struct mqx_boot_info_struct)
#define MCC_BOOT_MQX_IMAGE						_IO('M', 6)

// for interrupts
#define MAX_MVF_CPU_TO_CPU_INTERRUPTS (4)
#define MSCM_IRCPnIR_INT0_MASK		(0x00000001)
#define MSCM_IRCPnIR	((CPU_LOGICAL_NUMBER == 0x0) ? 0x800 : 0x804)
#define CPU_LOGICAL_NUMBER			(MCC_CORE_NUMBER)
#define MSCM_IRCPGIR_CPUTL_SHIFT	(16)
#define MSCM_WRITE(data, offset)	writel(data, mscm_base + offset)
#define MSCM_IRCPGIR	0x820
#define MCC_INTERRUPT(n) ((n == 0 ? 1 : 2) << MSCM_IRCPGIR_CPUTL_SHIFT)

// for semaphores
#define MAX_SEMA4_GATES 16
#define SEMA4_GATEn_READ(gate)		readb(MVF_IO_ADDRESS(MVF_SEMA4_BASE_ADDR) + gate)
#define SEMA4_GATEn_WRITE(lock, gate) 	writeb(lock, MVF_IO_ADDRESS(MVF_SEMA4_BASE_ADDR) + gate)

#endif /* __MCC_LINUX_H__ */
