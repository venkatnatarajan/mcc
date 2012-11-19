/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 *
 *  Freescale SEMA4 device driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef __MVF_SEMA4_H__
#define __MVF_SEMA4_H__

#include <linux/types.h>

#define MVF_SEMA4_LOCK_SUCCESS						(0)
#define MVF_SEMA4_LOCK_FAIL_LOCK_HELD_BY_LINUX 		(1)
#define MVF_SEMA4_LOCK_FAIL_LOCK_HELD_BY_OTHER_OS	(2)
#define	 MVF_SEMA4_LOCK_FAIL_GATE_STATE_UNLOCKED	(3)
#define	 MVF_SEMA4_UNLOCK_FAIL_LOCK_NOT_HELD		(4)

#define MVF_SEMA4_LOCK		_IOW('S', 1, __u8)
#define MVF_SEMA4_UNLOCK	_IOW('S', 2, __u8)
#define MVF_SEMA4_TRYLOCK	_IOW('S', 3, __u8)

#ifdef __KERNEL__

#define MAX_MVF_SEMA4_GATES (16)

#define CPU_LOGICAL_NUMBER			(0x0)

#define SEMA4_GATE_UNLOCKED 		0x0
#define SEMA4_GATE_LOCKED_BY_P0		0x1
#define SEMA4_GATE_LOCKED_BY_P1		0x2

#define SEMA4_GATE_LOCKED_BY_LINUX	((CPU_LOGICAL_NUMBER == 0x0) ? \
			SEMA4_GATE_LOCKED_BY_P0 : SEMA4_GATE_LOCKED_BY_P1)

#define SEMA4_GATE_LOCKED_BY_OTHER_OS	((CPU_LOGICAL_NUMBER == 0x0) ? \
			SEMA4_GATE_LOCKED_BY_P1 : SEMA4_GATE_LOCKED_BY_P0)

#define SEMA4_DRIVER_NAME ("mvf_sema4")

#ifdef USE_SEMA4_INTERRUPT
#define SEMA4_CPnINE_OFFSET 0x40
//TODO: verify the below address derivation is correct. Is it gate or cpu number in address offset??
#define SEMA4_GATEn_ENABLE_INTERRUPT(gate)	writew(1<<gate, sema4_base + SEMA4_CPnINE_OFFSET + 8*(u8)(gate/8));
#endif

#define SEMA4_GATEn_READ(gate)			readb(sema4_base + gate);
#define SEMA4_GATEn_WRITE(lock, gate) 	writeb(lock, sema4_base + gate);

#endif /* __KERNEL__ */
#endif /* __MVF_SEMA4_H__ */
