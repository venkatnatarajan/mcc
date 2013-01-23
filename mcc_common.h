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
#ifndef __MCC_COMMON__
#define __MCC_COMMON__

typedef unsigned int MCC_BOOLEAN;
typedef unsigned int MCC_MEM_SIZE;
#define null ((void*)0)

/*
 * End points
 */
typedef unsigned int MCC_CORE;
typedef unsigned int MCC_NODE;
typedef unsigned int MCC_PORT;

#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
__packed
#endif
struct mcc_endpoint {
	MCC_CORE core;
	MCC_NODE node;
	MCC_PORT port;
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_endpoint MCC_ENDPOINT;

/*
 * receive buffers and list structures
 */
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
__packed
#endif
struct mcc_receive_buffer {
	struct mcc_receive_buffer *next;
	int data_len;
	char data [MCC_ATTR_BUFFER_SIZE_IN_KB * 1024];
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_receive_buffer MCC_RECEIVE_BUFFER;

#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
__packed
#endif
struct mcc_receive_list {
	MCC_RECEIVE_BUFFER * head;
	MCC_RECEIVE_BUFFER * tail;
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_receive_list MCC_RECEIVE_LIST;

/*
 * Signals and signal queues
 */
#define BUFFER_QUEUED (0)
#define BUFFER_FREED (1)
typedef unsigned char MCC_SIGNAL_TYPE;
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
__packed
#endif
struct mcc_signal {
	MCC_SIGNAL_TYPE type;
	MCC_ENDPOINT    destination;
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_signal MCC_SIGNAL;

/*
 * Endpoint registration table
 */
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
__packed
#endif
struct endpoint_map_struct {
	MCC_ENDPOINT      endpoint;
	MCC_RECEIVE_LIST *list;
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
};
#else
}__attribute__((packed));
#endif
typedef struct endpoint_map_struct MCC_ENDPOINT_MAP_ITEM;

/*
 * Share Memory data - Bookkeeping data and buffers.
 */

#define MCC_INIT_STRING "mccisrdy"

#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
__packed
#endif
struct mcc_bookeeping_struct {
	/* String that indicates if this struct has been already initialized */
	char init_string[8]; // temporary till next mqx release`

	/* List of buffers for each endpoint */
	MCC_RECEIVE_LIST r_lists[MCC_ATTR_MAX_RECEIVE_ENDPOINTS];

	/* List of free buffers */
	MCC_RECEIVE_LIST free_list;

	/* Each core has it's own queue of received signals */
	MCC_SIGNAL signals_received[MCC_NUM_CORES][MCC_MAX_OUTSTANDING_SIGNALS];
	unsigned int signal_queue_head[MCC_NUM_CORES], signal_queue_tail[MCC_NUM_CORES];

	/* Endpoint map */
	MCC_ENDPOINT_MAP_ITEM endpoint_table[MCC_ATTR_MAX_RECEIVE_ENDPOINTS];

	/* Receive buffers */
	MCC_RECEIVE_BUFFER r_buffers[MCC_ATTR_NUM_RECEIVE_BUFFERS];
#if defined(__IAR_SYSTEMS_ICC__) && (__IAR_SYSTEMS_ICC__ != 0)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_bookeeping_struct MCC_BOOKEEPING_STRUCT;

extern MCC_BOOKEEPING_STRUCT * bookeeping_data;

/*
 * Common Macros
 */
#define MCC_RESERVED_PORT_NUMBER  (0)

/*
 * Errors
 */
#define MCC_SUCCESS         (0) /* function returned successfully */
#define MCC_ERR_TIMEOUT     (1) /* blocking function timed out before completing */
#define MCC_ERR_INVAL       (2) /* invalid input parameter */
#define MCC_ERR_NOMEM       (3) /* out of shared memory for message transmission */
#define MCC_ERR_ENDPOINT    (4) /* invalid endpoint / endpoint doesn't exist */
#define MCC_ERR_SEMAPHORE   (5) /* semaphore handling error */
#define MCC_ERR_DEV         (6) /* Device Open Error*/
#define MCC_ERR_INT         (7) /* Interrupt Error*/

/*
 * OS Selection
 */
#define MCC_LINUX         (1) /* Linux OS used */
#define MCC_MQX           (2) /* MQX RTOS used */

MCC_RECEIVE_LIST * mcc_get_endpoint_list(MCC_ENDPOINT endpoint);
MCC_RECEIVE_BUFFER * mcc_dequeue_buffer(MCC_RECEIVE_LIST *list);
void mcc_queue_buffer(MCC_RECEIVE_LIST *list, MCC_RECEIVE_BUFFER * r_buffer);
int mcc_remove_endpoint(MCC_ENDPOINT endpoint);
int mcc_register_endpoint(MCC_ENDPOINT endpoint);
int mcc_queue_signal(MCC_CORE core, MCC_SIGNAL signal);
int mcc_dequeue_signal(MCC_CORE core, MCC_SIGNAL *signal);

#define MCC_SIGNAL_QUEUE_FULL(core) (((bookeeping_data->signal_queue_tail[core] + 1) % MCC_MAX_OUTSTANDING_SIGNALS) == bookeeping_data->signal_queue_head[core])
#define MCC_SIGNAL_QUEUE_EMPTY(core) (bookeeping_data->signal_queue_head[core] == bookeeping_data->signal_queue_tail[core])

#endif /* __MCC_COMMON__ */

