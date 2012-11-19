/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 *
 *  Freescale IPC device driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef __MVF_IPC_H__
#define __MVF_IPC_H__

#include <linux/types.h>

typedef enum
{
	CP0_INTERRPUT_ONLY = 1,
	CP1_INTERRUPT_ONLY = 2,
	CP0_AND_CP1_INTERRUPT = 3,
	INTERRUPT_ALL_CORES_EXCEPT_REQUESTOR = 4,
	INTERRUPT_REQUESTING_CORE = 5
}CPU_INTERRUPT_TYPE;

#define CPU_INT0_ID		(0)
#define CPU_INT1_ID		(1)
#define CPU_INT2_ID		(2)
#define CPU_INT3_ID		(3)

typedef struct
{
	CPU_INTERRUPT_TYPE interrupt_type;
	u8 interrupt_id;
}cpu_interrupt_info_t;

#define MVF_GENERATE_CPU_TO_CPU_INTERRUPT 		_IOW('I', 1, cpu_interrupt_info_t)
#define MVF_REGISTER_PID_WITH_CPU_INTERRUPT		_IOWR('I', 2, __u8)

#define SIG_CPU_TO_CPU_INT (SIGRTMIN+10)

#ifdef __KERNEL__

/* TODO: This needs to be defined some place else */
#define CPU_LOGICAL_NUMBER			(0x0)

#define IPC_DRIVER_NAME ("mvf_ipc")
#define MSCM_IRCPnIR	((CPU_LOGICAL_NUMBER == 0x0) ? 0x800 : 0x804)
#define MSCM_IRCPGIR	0x820
#define MSCM_IRSPRCn	0x880

#define MSCM_IRCPnIR_INT0_MASK		(0x00000001)
#define MSCM_IRCPGIR_INTID_SHIFT	(0)
#define MSCM_IRCPGIR_CPUTL_SHIFT	(16)
#define MSCM_IRCPGIR_TLF_SHIFT		(24)

#define MSCM_READ(offset)						readl(mscm_base + offset)
#define MSCM_WRITE(data, offset)				writel(data, mscm_base + offset)
#define MSCM_IRSPRCn_WRITE(interrupt_control)	writew(interrupt_control, mscm_base+MSCM_IRSPRCn)

#define MAX_MVF_CPU_TO_CPU_INTERRUPTS (4)

#endif /* __KERNEL__ */
#endif /* __MVF_IPC_H__ */
