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
* $FileName: mcc_common.h.c$
* $Version : 3.8.1.0$
* $Date    : Jul-3-2012$
*
* Comments:
*
*   This is the MCC library header file
*
*END************************************************************************/

#ifndef __MCC_COMMON__
#define __MCC_COMMON__

#define MCC_INIT_STRING "mccisrdy"
#define MCC_VERSION_STRING "1.0"

typedef unsigned int MCC_BOOLEAN;
typedef unsigned int MCC_MEM_SIZE;
#define null ((void*)0)

/*
 * End points
 */
typedef unsigned int MCC_CORE;
typedef unsigned int MCC_NODE;
typedef unsigned int MCC_PORT;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct mcc_endpoint {
	MCC_CORE core;
	MCC_NODE node;
	MCC_PORT port;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_endpoint MCC_ENDPOINT;

/*
 * receive buffers and list structures
 */
#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct mcc_receive_buffer {
	struct mcc_receive_buffer *next;
	int data_len;
	char data [MCC_ATTR_BUFFER_SIZE_IN_KB * 1024];
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_receive_buffer MCC_RECEIVE_BUFFER;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct mcc_receive_list {
	MCC_RECEIVE_BUFFER * head;
	MCC_RECEIVE_BUFFER * tail;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_receive_list MCC_RECEIVE_LIST;

/*
 * Signals and signal queues
 */
#define BUFFER_QUEUED (0)
#define BUFFER_FREED  (1)
typedef unsigned char MCC_SIGNAL_TYPE;
#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct mcc_signal {
	MCC_SIGNAL_TYPE type;
	MCC_ENDPOINT    destination;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_signal MCC_SIGNAL;

/*
 * Endpoint registration table
 */
#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct mcc_endpoint_map_item {
	MCC_ENDPOINT      endpoint;
	MCC_RECEIVE_LIST  list;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_endpoint_map_item MCC_ENDPOINT_MAP_ITEM;

/*
 * MCC info structure
 */
#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct mcc_info_struct {
	/* <major>.<minor> - minor is changed whenever patched, major indicates compatibility */
	char version_string[sizeof(MCC_VERSION_STRING)];
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct mcc_info_struct MCC_INFO_STRUCT;

/*
 * Share Memory data - Bookkeeping data and buffers.
 */

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct mcc_bookeeping_struct {
	/* String that indicates if this struct has been already initialized */
	char init_string[sizeof(MCC_INIT_STRING)];

	/* String that indicates the MCC library version */
	char version_string[sizeof(MCC_VERSION_STRING)];

	/* List of free buffers */
	MCC_RECEIVE_LIST free_list;

	/* Each core has it's own queue of received signals */
	MCC_SIGNAL signals_received[MCC_NUM_CORES][MCC_MAX_OUTSTANDING_SIGNALS];
	unsigned int signal_queue_head[MCC_NUM_CORES], signal_queue_tail[MCC_NUM_CORES];

	/* Endpoint map */
	MCC_ENDPOINT_MAP_ITEM endpoint_table[MCC_ATTR_MAX_RECEIVE_ENDPOINTS];

	/* Receive buffers */
	MCC_RECEIVE_BUFFER r_buffers[MCC_ATTR_NUM_RECEIVE_BUFFERS];
#if defined(__IAR_SYSTEMS_ICC__)
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

#define MCC_SIGNAL_QUEUE_FULL(core)  (((bookeeping_data->signal_queue_tail[core] + 1) % MCC_MAX_OUTSTANDING_SIGNALS) == bookeeping_data->signal_queue_head[core])
#define MCC_SIGNAL_QUEUE_EMPTY(core) (bookeeping_data->signal_queue_head[core] == bookeeping_data->signal_queue_tail[core])
#define ENDPOINTS_EQUAL(e1, e2)      ((e1.core == e2.core) && (e1.node == e2.node) && (e1.port == e2.port))

#endif /* __MCC_COMMON__ */

