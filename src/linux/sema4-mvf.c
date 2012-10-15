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

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <mach/hardware.h>
#include <asm-generic/bug.h>
#include <asm/mach/irq.h>
#include <linux/delay.h>

typedef enum
{
	GATE_ZERO = 0,
	GATE_ONE,
	GATE_TWO,
	GATE_THREE,
	GATE_FOUR,
	GATE_FIVE,
	GATE_SIX,
	GATE_SEVEN,
	GATE_EIGHT,
	GATE_NINE,
	GATE_TEN,
	GATE_ELEVEN,
	GATE_TWELVE,
	GATE_THIRTEEN,
	GATE_FOURTEEN,
	GATE_FIFTEEN,
	MAX_GATE_NUMBER
}GATE_NUMBER;

typedef enum
{
	SUCCESS = 0,
	LOCK_FAIL_LOCK_HELD_BY_A5,
	LOCK_FAIL_LOCK_HELD_BY_M4,
	LOCK_FAIL_GATE_STATE_UNLOCKED,
	UNLOCK_FAIL_LOCK_NOT_HELD
}LOCK_OPERATION_RESULT;

enum
{
	UNLOCKED = 0,
	LOCK_HELD_BY_A5,
	LOCK_HELD_BY_M4
}LOCK_STATUS;

#define SEMA4_GATE_UNLOCKED 		0x0
#define SEMA4_GATE_LOCKED_BY_P0		0x1
#define SEMA4_GATE_LOCKED_BY_P1		0x2

#define DRIVER_NAME ("MVF_SEMA4")

static DEFINE_MUTEX(sema4_mutex);

static __iomem void *sema4_base;

struct completion gate_unlock_by_a5[MAX_GATE_NUMBER];

#ifdef USE_SEMA4_INTERRUPT
struct completion gate_unlock_by_m4;
#define SEMA4_CPnINE_OFFSET 0x40
#define SEMA4_GATEn_ENABLE_INTERRUPT(gate)	writew(1<<gate, sema4_base + SEMA4_CPnINE_OFFSET + 8*(u8)(gate/8));
#endif

#define SEMA4_GATEn_READ(gate)			readb(sema4_base + gate);
#define SEMA4_GATEn_WRITE(lock, gate) 	writeb(lock, sema4_base + gate);

#ifdef USE_SEMA4_INTERRUPT
static irqreturn_t sema4_irq_handler(int irq, void *dev_id)
{
	complete(&gate_unlock_by_m4);
	return IRQ_HANDLED;
}
#endif

void __init sema4_init(void)
{
	sema4_base = MVF_IO_ADDRESS(MVF_SEMA4_BASE_ADDR);

#ifdef USE_SEMA4_INTERRUPT
	if (request_irq(MVF_INT_SEMA4, sema4_irq_handler, 0, DRIVER_NAME, sema4_base) != 0)
	{
		printk("MVF SEMA4 interrupt request failed\n");
	}
#endif
	printk("MVF SEMA4 driver initialized");
}

LOCK_OPERATION_RESULT sema4_spinlock(GATE_NUMBER gate)
{
	LOCK_OPERATION_RESULT retval;
	u8 gate_status;

	mutex_lock(&sema4_mutex);

	gate_status = SEMA4_GATEn_READ(gate);

	/* If gate is already locked by a process on A5 then wait till
	 * the gate is unlocked by the other process and try obtain a mutex lock
	 */
	if(gate_status == SEMA4_GATE_LOCKED_BY_P0)
	{
		do
		{
			/* Unlock the mutex so that the other A5 process can unlock
			 * the gate. Wait for unlock completion
			 */
			mutex_unlock(&sema4_mutex);
			wait_for_completion(&gate_unlock_by_a5[gate]);

			/* Obtain a mutex lock and check if some other process on A5 snuck
			 * in to obtain a gate lock
			 */
			mutex_lock(&sema4_mutex);
			gate_status = SEMA4_GATEn_READ(gate);
		}while(gate_status == SEMA4_GATE_LOCKED_BY_P0);
	}

	/* Once mutex lock is obtained and we check gate is not locked by A5 we
	 * can proceed to obtain a lock. Only M4 can sneak in at this point.
	 */

	do{

#ifdef USE_SEMA4_INTERRUPT
		if(gate_status == SEMA4_GATE_LOCKED_BY_P1)
		{
			init_completion(&gate_unlock_by_m4);
			SEMA4_GATEn_ENABLE_INTERRUPT(gate);
			wait_for_completion(&gate_unlock_by_m4);
		}
#endif

		SEMA4_GATEn_WRITE((u8)SEMA4_GATE_LOCKED_BY_P0, gate);
		gate_status = SEMA4_GATEn_READ(gate);
	}while(gate_status != SEMA4_GATE_LOCKED_BY_P0);

	init_completion(&gate_unlock_by_a5[gate]);
	mutex_unlock(&sema4_mutex);
	retval = SUCCESS;
	return retval;
}

LOCK_OPERATION_RESULT sema4_trylock(GATE_NUMBER gate)
{
	LOCK_OPERATION_RESULT retval;
	u8 gate_status;

	mutex_lock(&sema4_mutex);

	gate_status = SEMA4_GATEn_READ(gate);

	if(gate_status == SEMA4_GATE_UNLOCKED)
	{
		SEMA4_GATEn_WRITE((u8)SEMA4_GATE_LOCKED_BY_P0, gate);
		gate_status = SEMA4_GATEn_READ(gate);

		/* Check if we obtained lock or did M4 sneak in! */
		if(gate_status == SEMA4_GATE_LOCKED_BY_P0)
		{
			init_completion(&gate_unlock_by_a5[gate]);
			mutex_unlock(&sema4_mutex);
			retval = SUCCESS;
			return retval;
		}
	}

	mutex_unlock(&sema4_mutex);

	if(gate_status == SEMA4_GATE_LOCKED_BY_P0)
	{
		retval = LOCK_FAIL_LOCK_HELD_BY_A5;
	}
	else if(gate_status == SEMA4_GATE_LOCKED_BY_P1)
	{
		retval = LOCK_FAIL_LOCK_HELD_BY_M4;
	}
	else
	{
		retval = LOCK_FAIL_GATE_STATE_UNLOCKED;
	}
	return retval;
}
EXPORT_SYMBOL(sema4_trylock);

LOCK_OPERATION_RESULT sema4_unlock(GATE_NUMBER gate)
{
	LOCK_OPERATION_RESULT retval = UNLOCK_FAIL_LOCK_NOT_HELD;
	u8 gate_status;

	mutex_lock(&sema4_mutex);

	gate_status = SEMA4_GATEn_READ(gate);

	if(gate_status == SEMA4_GATE_LOCKED_BY_P0)
	{
		SEMA4_GATEn_WRITE((u8)SEMA4_GATE_UNLOCKED, gate);
		retval = SUCCESS;
		complete(&gate_unlock_by_a5[gate]);
	}

	mutex_unlock(&sema4_mutex);
	return retval;
}
EXPORT_SYMBOL(sema4_unlock);

