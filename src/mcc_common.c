/**HEADER********************************************************************
* 
* Copyright 2013 Freescale, Inc.
*
*************************************************************************** 
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
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
* WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: mcc_common.c$
* $Version : 3.8.1.0$
* $Date    : Jul-3-2012$
*
* Comments:
*
*   This file contains MCC library common functions
*
*END************************************************************************/

#include "mcc_config.h"
#include "mcc_common.h"

#if (MCC_OS_USED == MCC_LINUX)
#include "linux/mcc_linux.h"
#include "linux/mcc_shm_linux.h"
#include <mach/hardware.h>
#include <linux/kernel.h>
#endif

#if (MCC_OS_USED == MCC_MQX)
MCC_BOOKEEPING_STRUCT * bookeeping_data;
#endif

/*!
 * \brief This function registers an endpoint.
 *
 * Register an endpoint with specified structure / params (core, node and port).
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[Out] Status
 */
int mcc_register_endpoint(MCC_ENDPOINT endpoint)
{
	int i;

	/* must be valid */
	if(endpoint.port == MCC_RESERVED_PORT_NUMBER)
		return MCC_ERR_ENDPOINT;

	/* check not already registered */
	if(mcc_get_endpoint_list(endpoint))
		return MCC_ERR_ENDPOINT;

	for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++) {

		if(bookeeping_data->endpoint_table[i].endpoint.port == MCC_RESERVED_PORT_NUMBER) {
			bookeeping_data->endpoint_table[i].endpoint.core = endpoint.core;
			bookeeping_data->endpoint_table[i].endpoint.node = endpoint.node;
			bookeeping_data->endpoint_table[i].endpoint.port = endpoint.port;
			return MCC_SUCCESS;
		}
	}
	return MCC_ERR_ENDPOINT;
}

/*!
 * \brief This function removes an endpoint.
 *
 * Removes an endpoint with specified structure / params (core, node and port).
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[Out] Status
 */
int mcc_remove_endpoint(MCC_ENDPOINT endpoint)
{
	int i=0;
	for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++) {

		if(ENDPOINTS_EQUAL(bookeeping_data->endpoint_table[i].endpoint, endpoint)) {
			/* clear the queue */
#if (MCC_OS_USED == MCC_LINUX)
			MCC_RECEIVE_BUFFER * buffer = mcc_dequeue_buffer((MCC_RECEIVE_LIST *)MQX_TO_VIRT(&bookeeping_data->endpoint_table[i].list));
			while(buffer) {
				mcc_queue_buffer((MCC_RECEIVE_LIST *)MQX_TO_VIRT(&bookeeping_data->free_list), buffer);
				buffer = mcc_dequeue_buffer((MCC_RECEIVE_LIST *)MQX_TO_VIRT(&bookeeping_data->endpoint_table[i].list));
			}
#elif (MCC_OS_USED == MCC_MQX)
			MCC_RECEIVE_BUFFER * buffer = mcc_dequeue_buffer((MCC_RECEIVE_LIST *)&bookeeping_data->endpoint_table[i].list);
			while(buffer) {
				mcc_queue_buffer(&bookeeping_data->free_list, buffer);
				buffer = mcc_dequeue_buffer((MCC_RECEIVE_LIST *)&bookeeping_data->endpoint_table[i].list);
			}
#endif
			/* indicate free */
			bookeeping_data->endpoint_table[i].endpoint.port = MCC_RESERVED_PORT_NUMBER;
			return MCC_SUCCESS;
		}
	}
	return MCC_ERR_ENDPOINT;
}

/*!
 * \brief This function dequeues the buffer.
 *
 * Dequeues the buffer from the list.
 *
 * \param[in] Pointer to the MCC_RECEIVE_LIST structure.
 * \param[Out] Pointer to MCC_RECEIVE_BUFFER
 */
MCC_RECEIVE_BUFFER * mcc_dequeue_buffer(MCC_RECEIVE_LIST *list)
{
	MCC_RECEIVE_BUFFER * next_buf = list->head;

#if (MCC_OS_USED == MCC_LINUX)
	MCC_RECEIVE_BUFFER * next_buf_virt = (MCC_RECEIVE_BUFFER *)MQX_TO_VIRT(next_buf);
	if(next_buf) {
		list->head = next_buf_virt->next;
		if(list->tail == next_buf)
			list->tail = null;
	}
	return next_buf_virt;
#elif (MCC_OS_USED == MCC_MQX)
	if(next_buf) {
		list->head = next_buf->next;
		if(list->tail == next_buf)
			list->tail = null;
	}
	return next_buf;
#endif
}

/*!
 * \brief This function queues the buffer.
 *
 * Queues the buffer in the list.
 *
 * \param[in] Pointer to the MCC_RECEIVE_LIST structure and pointer to MCC_RECEIVE_BUFFER.
 * \param[Out] none
 */
void mcc_queue_buffer(MCC_RECEIVE_LIST *list, MCC_RECEIVE_BUFFER * r_buffer)
{
#if (MCC_OS_USED == MCC_LINUX)
	MCC_RECEIVE_BUFFER * last_buf = (MCC_RECEIVE_BUFFER *)MQX_TO_VIRT(list->tail);
	MCC_RECEIVE_BUFFER * r_buffer_mqx = (MCC_RECEIVE_BUFFER *)VIRT_TO_MQX(r_buffer);
	if(last_buf)
		last_buf->next = r_buffer_mqx;
	else
		list->head = r_buffer_mqx;
	r_buffer->next = null;
	list->tail = r_buffer_mqx;
#elif (MCC_OS_USED == MCC_MQX)
	MCC_RECEIVE_BUFFER * last_buf = list->tail;
	if(last_buf)
		last_buf->next = r_buffer;
	else
		list->head = r_buffer;
	r_buffer->next = null;
	list->tail = r_buffer;
#endif
}

/*!
 * \brief This function returns the endpoint list.
 *
 * Returns the MCC_RECEIVE_LIST respective to the endpoint structure provided.
 *
 * \param[in] Pointer to the MCC_ENDPOINT structure.
 * \param[Out] MCC_RECEIVE_LIST / null.
 */
MCC_RECEIVE_LIST * mcc_get_endpoint_list(MCC_ENDPOINT endpoint)
{
	int i=0;
	for(i = 0; i<MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++) {

		if(ENDPOINTS_EQUAL(bookeeping_data->endpoint_table[i].endpoint, endpoint))
#if (MCC_OS_USED == MCC_LINUX)
			return (MCC_RECEIVE_LIST *)MQX_TO_VIRT(&bookeeping_data->endpoint_table[i].list);
#elif (MCC_OS_USED == MCC_MQX)
			return (MCC_RECEIVE_LIST *)&bookeeping_data->endpoint_table[i].list;
#endif
	}
	return null;
}

/*
 * Signal circula queue rules:
 * 	tail points to next free slot
 * 	head points to first occupied slot
 * 	head == tail indicates empty
 * 	(tail + 1) % len = fill
 * This method costs 1 slot since you need to differentiate
 * between full and empty (if you fill the last slot it looks
 * like empty since h == t)
 */

/*!
 * \brief This function queues a signal
 *
 * Returns MCC_SUCCESS on success, MCC_ERR_NOMEM if queue full
 *
 * \param[in] Core number to signal
 * \param[in] signal
 * \param[Out] MCC_RECEIVE_LIST / null.
 */
int mcc_queue_signal(MCC_CORE core, MCC_SIGNAL signal)
{

	int tail = bookeeping_data->signal_queue_tail[core];
	int new_tail = tail == (MCC_MAX_OUTSTANDING_SIGNALS-1) ? 0 : tail+1;

	if(MCC_SIGNAL_QUEUE_FULL(core))
		return MCC_ERR_NOMEM;

	bookeeping_data->signals_received[core][tail].type = signal.type;
	bookeeping_data->signals_received[core][tail].destination.core = signal.destination.core;
	bookeeping_data->signals_received[core][tail].destination.node = signal.destination.node;
	bookeeping_data->signals_received[core][tail].destination.port = signal.destination.port;

	bookeeping_data->signal_queue_tail[core] = new_tail;

	return MCC_SUCCESS;
}

/*!
 * \brief This function queues a signal
 *
 * Returns the number of signals dequeued (0 or 1).
 *
 * \param[in] Core number to signal
 * \param[in] signal
 * \param[Out] MCC_RECEIVE_LIST / null.
 */
int mcc_dequeue_signal(MCC_CORE core, MCC_SIGNAL *signal)
{
	int head = bookeeping_data->signal_queue_head[core];

	if(MCC_SIGNAL_QUEUE_EMPTY(core))
		return 0;

	signal->type = bookeeping_data->signals_received[core][head].type;
	signal->destination.core = bookeeping_data->signals_received[core][head].destination.core;
	signal->destination.node = bookeeping_data->signals_received[core][head].destination.node;
	signal->destination.port = bookeeping_data->signals_received[core][head].destination.port;

	bookeeping_data->signal_queue_head[core] = head == (MCC_MAX_OUTSTANDING_SIGNALS-1) ? 0 : head+1;

	return 1;
}



