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
#include <linux/types.h>
#include <linux/device.h>
#include <mach/hardware.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>

// common to MQX and Linux
// TODO the order of these should not matter
#include "mcc_config.h"
#include "mcc_common.h"

// linux only
#include "mcc_linux.h"
#include "mcc_shm_linux.h"
#include "mcc_sema4_linux.h"

int mcc_sema4_grab(unsigned char gate_num)
{
	unsigned char gate_val;
	int timeout_ms;

	if((gate_num < 0) || (gate_num >= MAX_SEMA4_GATES))
		return -EINVAL;

	gate_val = MCC_CORE_NUMBER + 1;
	SEMA4_GATEn_WRITE(gate_val, gate_num);

	/*
	The expectation is that holds of the semaphore are really short
	so it's ok to treat them like a spin lock (with protection from
	real hangs of 1/2 second).
	*/

	timeout_ms = 10;	// 1/100 second
	while(timeout_ms && (gate_val != SEMA4_GATEn_READ(gate_num)))
	{
		udelay(1000);	// 1 milli-second
		timeout_ms--;
	}

	return timeout_ms > 0 ? MCC_SUCCESS : -EBUSY;
}

int mcc_sema4_release(unsigned char gate_num)
{

	if((gate_num < 0) || (gate_num >= MAX_SEMA4_GATES))
		return -EINVAL;

	SEMA4_GATEn_WRITE(0, gate_num);

	if(SEMA4_GATEn_READ(gate_num) == 0)
		return MCC_SUCCESS;
	else
		return -EIO;
}

