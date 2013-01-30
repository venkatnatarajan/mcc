/*HEADER*********************************************************************
*
* Copyright (c) 2013 Freescale Semiconductor;
* All Rights Reserved
*
***************************************************************************
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
***************************************************************************
* $FileName: mcc_api.c$
* $Version : 3.8.1.0$
* $Date    : Jul-3-2012$
*
* Comments:
*
*   This file contains MCC library API functions implementation
*
*END************************************************************************/

#include <string.h>
#include "mcc_config.h"
#include "mcc_common.h"
#include "mcc_api.h"

#if (MCC_OS_USED == MCC_LINUX)
#include "mcc_linux.h"
#elif (MCC_OS_USED == MCC_MQX)
#include "mcc_mqx.h"
extern LWEVENT_STRUCT lwevent_buffer_queued[MCC_NUM_CORES];
extern LWEVENT_STRUCT lwevent_buffer_freed[MCC_NUM_CORES];
#endif

const char * const init_string    = MCC_INIT_STRING;
const char * const version_string = MCC_VERSION_STRING;

/*!
 * \brief This function initializes the Multi Core Communication.
 *
 * XXX
 *
 * \param[in] node Node number.
 */
int mcc_initialize(MCC_NODE node)
{
    int i,j = 0;
    int return_value = MCC_SUCCESS;
    MCC_SIGNAL tmp_signals_received = {(MCC_SIGNAL_TYPE)0, (MCC_CORE)0, (MCC_NODE)0, (MCC_PORT)0};

#if (MCC_OS_USED == MCC_MQX)
    //TODO move to ??? (where?)
    _lwevent_create(&lwevent_buffer_queued[MCC_CORE_NUMBER],0);
    _lwevent_create(&lwevent_buffer_freed[MCC_CORE_NUMBER],0);
    //_DCACHE_DISABLE(); //TODO should be in the OS-specific initialization
#endif
    /* Initialize synchronization module */
    return_value = mcc_init_semaphore(MCC_SEMAPHORE_NUMBER);
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* Register CPU-to-CPU interrupt for inter-core signaling */
    //mcc_register_cpu_to_cpu_isr(MCC_CORE0_CPU_TO_CPU_VECTOR);
    return_value = mcc_register_cpu_to_cpu_isr();
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* Initialize the bookeeping structure */
    MCC_DCACHE_INVALIDATE_MLINES(bookeeping_data, sizeof(MCC_BOOKEEPING_STRUCT));
    if(strcmp(bookeeping_data->init_string, init_string) != 0) {

    	/* Zero it all - no guarantee Linux or uboot didnt touch it before it was reserved */
    	_mem_zero((void*) bookeeping_data, (_mem_size) sizeof(struct mcc_bookeeping_struct));

    	/* Set init_string in case it has not been set yet by another core */
    	mcc_memcpy((void*)init_string, bookeeping_data->init_string, (unsigned int)sizeof(bookeeping_data->init_string));

    	/* Set version_string */
    	mcc_memcpy((void*)version_string, bookeeping_data->version_string, (unsigned int)sizeof(bookeeping_data->version_string));

        /* Initialize the free list */
        bookeeping_data->free_list.head = &bookeeping_data->r_buffers[0];
        bookeeping_data->free_list.tail = &bookeeping_data->r_buffers[MCC_ATTR_NUM_RECEIVE_BUFFERS-1];

        /* Initialize receive buffers */
        for(i=0; i<MCC_ATTR_NUM_RECEIVE_BUFFERS-1; i++) {
        	bookeeping_data->r_buffers[i].next = &bookeeping_data->r_buffers[i+1];
        }
        bookeeping_data->r_buffers[MCC_ATTR_NUM_RECEIVE_BUFFERS-1].next = null;

        /* Initialize signal queues */
        for(i=0; i<MCC_NUM_CORES; i++) {
        	for(j=0; j<MCC_MAX_OUTSTANDING_SIGNALS; j++) {
        	    bookeeping_data->signals_received[i][j] = tmp_signals_received;
        	}
        	bookeeping_data->signal_queue_head[i] = 0;
            bookeeping_data->signal_queue_tail[i] = 0;
        }

        /* Mark all endpoint ports as free */
    	for(i=0; i<MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++) {
    		bookeeping_data->endpoint_table[i].endpoint.port = MCC_RESERVED_PORT_NUMBER;
    	}
    }
    MCC_DCACHE_FLUSH_MLINES(bookeeping_data, sizeof(MCC_BOOKEEPING_STRUCT));
    return return_value;
}

/*!
 * \brief This function de-initializes the Multi Core Communication.
 *
 * Clear local data, clean semaphore and share memory resources ......
 *
 * \param[in] node Node number to be deinitialized.
 */
int mcc_destroy(MCC_NODE node)
{
    int i = 0, return_value;

#if (MCC_OS_USED == MCC_MQX)
    _lwevent_destroy(&lwevent_buffer_queued[MCC_CORE_NUMBER]);
    _lwevent_destroy(&lwevent_buffer_freed[MCC_CORE_NUMBER]);
#endif

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* All endpoints of the particular node have to be removed from the endpoint table */
    MCC_DCACHE_INVALIDATE_MLINES(&bookeeping_data->endpoint_table[0], MCC_ATTR_MAX_RECEIVE_ENDPOINTS * sizeof(MCC_ENDPOINT_MAP_ITEM));
    for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++) {
        if (bookeeping_data->endpoint_table[i].endpoint.node == node) {
            /* Remove the endpoint from the table */
            mcc_remove_endpoint(bookeeping_data->endpoint_table[i].endpoint);
        }
    }

    /* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* Deinitialize synchronization module */
    mcc_deinit_semaphore(MCC_SEMAPHORE_NUMBER);

    // TODO: Clear signal queue, Unregister the CPU-to-CPU interrupt????
    return return_value;
}

/*!
 * \brief This function creates an endpoint.
 *
 * Create an endpoint on the local node with the specified port number.
 *
 * \param[out] endpoint Pointer to the endpoint structure.
 * \param[in] port Port number.
 */
int mcc_create_endpoint(MCC_ENDPOINT *endpoint, MCC_PORT port)
{
    int return_value = MCC_SUCCESS;

    /* Fill the endpoint structure */
    endpoint->core = (MCC_CORE)MCC_CORE_NUMBER;
    endpoint->node = (MCC_NODE)0; //TODO: define node number
    endpoint->port = (MCC_PORT)port;

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    /* Add new endpoint data into the book-keeping structure */
    return_value = mcc_register_endpoint(*endpoint);
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* Semaphore-protected section end */
    return_value =  mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    return return_value;
}

/*!
 * \brief This function destroys an endpoint.
 *
 * Destroy an endpoint on the local node.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 */
int mcc_destroy_endpoint(MCC_ENDPOINT *endpoint)
{
    int return_value = MCC_SUCCESS;

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
	if(return_value != MCC_SUCCESS)
		return return_value;

    /* Add new endpoint data into the book-keeping structure */
    return_value = mcc_remove_endpoint(*endpoint);
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* Semaphore-protected section end */
    return_value =  mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    return return_value;
}

/*!
 * \brief This function sends a message.
 *
 * The message is copied into the MCC buffer and the other core is signaled.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[in] msg Pointer to the message to be sent.
 * \param[in] msg_size Size of the message to be sent in bytes.
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffffffff = blocking call .
 */
int mcc_send(MCC_ENDPOINT *endpoint, void *msg, MCC_MEM_SIZE msg_size, unsigned int timeout_us)
{
	int return_value;
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf;
    MCC_SIGNAL affiliated_signal;
#if (MCC_OS_USED == MCC_MQX)
    unsigned int time_us_tmp;
    TIME_STRUCT time;
    MQX_TICK_STRUCT tick_time;
#endif

    /* Check if the size of the message to be sent does not exceed the size of the mcc buffer */
    if(msg_size > sizeof(bookeeping_data->r_buffers[0].data)) {
    	return MCC_ERR_INVAL;
    }

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(*endpoint);

    if(list == null) {
    	/* The endpoint does not exists (has not been registered so far), return immediately - error */
    	mcc_release_semaphore();
    	return MCC_ERR_ENDPOINT;
    }

    /* Dequeue the buffer from the free list */
    MCC_DCACHE_INVALIDATE_LINE((void*)&bookeeping_data->free_list);
    buf = mcc_dequeue_buffer(&bookeeping_data->free_list);

    while(buf == null) {
    	mcc_release_semaphore();

    	/* Non-blocking call */
        if(timeout_us == 0) {
        	return MCC_ERR_NOMEM;
        }
        /* Blocking call - wait forever */
        else if(timeout_us == 0xFFFFFFFF) {
#if (MCC_OS_USED == MCC_MQX)
        	_lwevent_wait_ticks(&lwevent_buffer_freed[endpoint->core], 1, TRUE, 0);
        	_lwevent_clear(&lwevent_buffer_freed[endpoint->core], 1);
#endif
        	MCC_DCACHE_INVALIDATE_LINE((void*)&bookeeping_data->free_list);
        	mcc_get_semaphore();
        	buf = mcc_dequeue_buffer(&bookeeping_data->free_list);
        }
        /* timeout_us > 0 */
        else {
#if (MCC_OS_USED == MCC_MQX)
        	time_us_tmp = mcc_time_get_microseconds();
        	time_us_tmp += timeout_us;
        	time.SECONDS = time_us_tmp/1000000;
        	time.MILLISECONDS = time_us_tmp*1000;
        	_time_to_ticks(&time, &tick_time);
        	_lwevent_wait_until(&lwevent_buffer_freed[endpoint->core], 1, TRUE, &tick_time);
        	_lwevent_clear(&lwevent_buffer_freed[endpoint->core], 1);
#endif
        	MCC_DCACHE_INVALIDATE_LINE((void*)&bookeeping_data->free_list);
        	mcc_get_semaphore();
        	buf = mcc_dequeue_buffer(&bookeeping_data->free_list);

        	if(buf == null) {
        		/* Buffer not dequeued before the timeout */
        		mcc_release_semaphore();
        		return MCC_ERR_TIMEOUT;
        	}
        }
    }

    /* Semaphore-protected section end */
    mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

	/* Copy the message into the MCC receive buffer */
    mcc_memcpy(msg, (void*)buf->data, (unsigned int)msg_size);
    MCC_DCACHE_FLUSH_MLINES((void*)buf->data, msg_size);
    buf->data_len = msg_size;
    MCC_DCACHE_FLUSH_LINE((void*)&buf->data_len);

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    /* Enqueue the buffer into the endpoint buffer list */
    mcc_queue_buffer(list, buf);

    /* Write the signal type into the signal queue of the particular core */
    affiliated_signal.type = BUFFER_QUEUED;
    affiliated_signal.destination = *endpoint;
    return_value = mcc_queue_signal(endpoint->core, affiliated_signal);

    /* Semaphore-protected section end */
    mcc_release_semaphore();

    if(return_value != MCC_SUCCESS)
     	return return_value;

    //TODO identify the core in the parameter of this function
    /* Signal the other core by generating the CPU-to-CPU interrupt */
    return_value = mcc_generate_cpu_to_cpu_interrupt();

    return return_value;
}

/*!
 * \brief This function receives a message. The data is copied into the user-app. buffer.
 *
 * This is the "receive with copy" version of the MCC receive function. This version is simple
 * to use but includes the cost of copying the data from shared memory into the user space buffer.
 * The user has no obligation or burden to manage shared memory buffers.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[in] buffer Pointer to the user-app. buffer data will be copied into.
 * \param[in] buffer_size Size of the user-app. buffer data will be copied into.
 * \param[out] recv_size Pointer to the number of received bytes.
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffffffff = blocking call .
 */
int mcc_recv_copy(MCC_ENDPOINT *endpoint, void *buffer, MCC_MEM_SIZE buffer_size, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
{
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf;
    MCC_SIGNAL affiliated_signal;
    MCC_ENDPOINT tmp_destination = {(MCC_CORE)0, (MCC_NODE)0, (MCC_PORT)0};
    int return_value, i = 0;
#if (MCC_OS_USED == MCC_MQX)
    unsigned int time_us_tmp;
    TIME_STRUCT time;
    MQX_TICK_STRUCT tick_time;
#endif

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
	if(return_value != MCC_SUCCESS)
		return return_value;

    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(*endpoint);

    /* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    /* The endpoint is not valid */
    if(list == null) {
    	return MCC_ERR_ENDPOINT;
    }

    if(list->head == (MCC_RECEIVE_BUFFER*)0) {
    	/* Non-blocking call */
    	if(timeout_us == 0) {
    		return MCC_ERR_TIMEOUT;
    	}
    	/* Blocking call */
    	else if(timeout_us == 0xFFFFFFFF) {
#if (MCC_OS_USED == MCC_MQX)
    		_lwevent_wait_ticks(&lwevent_buffer_queued[endpoint->core], 1<<endpoint->port, TRUE, 0);
#endif
    	    MCC_DCACHE_INVALIDATE_LINE((void*)list);
    	}
    	/* timeout_us > 0 */
    	else {
#if (MCC_OS_USED == MCC_MQX)
        	time_us_tmp = mcc_time_get_microseconds();
        	time_us_tmp += timeout_us;
        	time.SECONDS = time_us_tmp/1000000;
        	time.MILLISECONDS = time_us_tmp*1000;
        	_time_to_ticks(&time, &tick_time);
        	_lwevent_wait_until(&lwevent_buffer_queued[endpoint->core], 1<<endpoint->port, TRUE, &tick_time);
#endif
            MCC_DCACHE_INVALIDATE_LINE((void*)list);
    	}
    }

    /* Clear event bit specified for the particular endpoint in the lwevent_buffer_queued lwevent group */
    _lwevent_clear(&lwevent_buffer_queued[endpoint->core], 1<<endpoint->port);

    if(list->head == (MCC_RECEIVE_BUFFER*)0) {
    	/* Buffer not dequeued before the timeout */
    	return MCC_ERR_TIMEOUT;
    }

    /* Copy the message from the MCC receive buffer into the user-app. buffer */
    MCC_DCACHE_INVALIDATE_MLINES((void*)list->head->data, list->head->data_len);
    mcc_memcpy((void*)list->head->data, buffer, buffer_size);
    MCC_DCACHE_INVALIDATE_LINE((void*)list->head->data_len);
    *recv_size = (MCC_MEM_SIZE)(list->head->data_len);

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
	if(return_value != MCC_SUCCESS)
		return return_value;

    /* Dequeue the buffer from the endpoint list */
    buf = mcc_dequeue_buffer(list);

    /* Enqueue the buffer into the free list */
    MCC_DCACHE_INVALIDATE_LINE((void*)&bookeeping_data->free_list);
    mcc_queue_buffer(&bookeeping_data->free_list, buf);

    /* Notify all cores (except of itself) via CPU-to-CPU interrupt that a buffer has been freed */
    affiliated_signal.type = BUFFER_FREED;
    affiliated_signal.destination = tmp_destination;
    for (i=0; i<MCC_NUM_CORES; i++) {
        if(i != MCC_CORE_NUMBER) {
            mcc_queue_signal(i, affiliated_signal);
        }
    }
    mcc_generate_cpu_to_cpu_interrupt(); //TODO ensure interrupt generation into all cores

    /* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    return return_value;
}

/*!
 * \brief This function receives a message. The data is NOT copied into the user-app. buffer.
 *
 * This is the "zero-copy receive" version of the MCC receive function. No data is copied, just
 * the pointer to the data is returned. This version is fast, but requires the user to manage
 * buffer allocation. Specifically the user must decide when a buffer is no longer in use and
 * make the appropriate API call to free it.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[out] buffer_p Pointer to the MCC buffer of the shared memory where the received data is stored.
 * \param[out] recv_size Pointer to the number of received bytes.
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffffffff = blocking call .
 */
int mcc_recv_nocopy(MCC_ENDPOINT *endpoint, void **buffer_p, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
{
    MCC_RECEIVE_LIST *list;
    int return_value;
#if (MCC_OS_USED == MCC_MQX)
    unsigned int time_us_tmp;
    TIME_STRUCT time;
    MQX_TICK_STRUCT tick_time;
#endif

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
	if(return_value != MCC_SUCCESS)
		return return_value;

    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(*endpoint);

    /* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    if(list->head == (MCC_RECEIVE_BUFFER*)0) {
    	/* Non-blocking call */
    	if(timeout_us == 0) {
    		return MCC_ERR_TIMEOUT;
    	}
    	/* Blocking call */
    	else if(timeout_us == 0xFFFFFFFF) {
#if (MCC_OS_USED == MCC_MQX)
    		_lwevent_wait_ticks(&lwevent_buffer_queued[endpoint->core], 1<<endpoint->port, TRUE, 0);
#endif
    	    MCC_DCACHE_INVALIDATE_LINE((void*)list);
    	}
    	/* timeout_us > 0 */
    	else {
#if (MCC_OS_USED == MCC_MQX)
        	time_us_tmp = mcc_time_get_microseconds();
        	time_us_tmp += timeout_us;
        	time.SECONDS = time_us_tmp/1000000;
        	time.MILLISECONDS = time_us_tmp*1000;
        	_time_to_ticks(&time, &tick_time);
        	_lwevent_wait_until(&lwevent_buffer_queued[endpoint->core], 1<<endpoint->port, TRUE, &tick_time);
#endif
            MCC_DCACHE_INVALIDATE_LINE((void*)list);
    	}
    }

    /* Clear event bit specified for the particular endpoint in the lwevent_buffer_queued lwevent group */
    _lwevent_clear(&lwevent_buffer_queued[endpoint->core], 1<<endpoint->port);

    if(list->head == (MCC_RECEIVE_BUFFER*)0) {
		/* Buffer not dequeued before the timeout */
		return MCC_ERR_TIMEOUT;
	}

    /* Get the message pointer from the head of the receive buffer list */
    MCC_DCACHE_INVALIDATE_MLINES((void*)list->head->data, list->head->data_len);
    buffer_p = (void**)&list->head->data;
    MCC_DCACHE_INVALIDATE_LINE((void*)list->head->data_len);
    *recv_size = (MCC_MEM_SIZE)(list->head->data_len);

    return return_value;
}

/*!
 * \brief This function returns number of available buffers for the specified endpoint.
 *
 * Checks if messages are available on a receive endpoint. The call only checks the
 * availability of messages and does not dequeue them.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[out] num_msgs Pointer to the number of messages that are available in the receive queue and waiting for processing.
 */
int mcc_msgs_available(MCC_ENDPOINT *endpoint, unsigned int *num_msgs)
{
    unsigned int count = 0;
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf;
    int return_value;

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
	if(return_value != MCC_SUCCESS)
		return return_value;

    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(*endpoint);

    buf = list->head;
    while(buf != (MCC_RECEIVE_BUFFER*)0) {
        count++;
        buf = (MCC_RECEIVE_BUFFER*)buf->next;
    }
    *num_msgs = count;

    /* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    return return_value;
}

/*!
 * \brief This function frees a buffer.
 *
 * Once the zero-copy mechanism of receiving data is used this function
 * has to be called to free a buffer and to make it available for the next data
 * transfer.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[in] buffer Pointer to the buffer to be freed.
 */
int mcc_free_buffer(MCC_ENDPOINT *endpoint, void *buffer)
{
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER *buf, *buf_tmp;
    MCC_SIGNAL affiliated_signal;
    MCC_ENDPOINT tmp_destination = {(MCC_CORE)0, (MCC_NODE)0, (MCC_PORT)0};
    int return_value, i = 0;

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
	if(return_value != MCC_SUCCESS)
		return return_value;

    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(*endpoint);

    /* Dequeue the buffer from the endpoint list */
    if(list->head == (MCC_RECEIVE_BUFFER*)buffer) {
        buf = mcc_dequeue_buffer(list);
    }
    else {
        buf_tmp = (MCC_RECEIVE_BUFFER*)list->head;
        while((MCC_RECEIVE_BUFFER*)buf_tmp->next != (MCC_RECEIVE_BUFFER*)buffer) {
            buf_tmp = (MCC_RECEIVE_BUFFER*)buf_tmp->next;
        }
        buf = (MCC_RECEIVE_BUFFER*)buf_tmp->next;
        buf_tmp->next = buf_tmp->next->next;
    }

    /* Enqueue the buffer into the free list */
    MCC_DCACHE_INVALIDATE_LINE((void*)&bookeeping_data->free_list);
    mcc_queue_buffer(&bookeeping_data->free_list, buf);

    /* Notify all cores (except of itself) via CPU-to-CPU interrupt that a buffer has been freed */
    affiliated_signal.type = BUFFER_FREED;
    affiliated_signal.destination = tmp_destination;
    for (i=0; i<MCC_NUM_CORES; i++) {
    	if(i != MCC_CORE_NUMBER) {
    		mcc_queue_signal(i, affiliated_signal);
        }
    }
    mcc_generate_cpu_to_cpu_interrupt(); //TODO ensure interrupt generation into all cores

    /* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    return return_value;
}

/*!
 * \brief This function returns info structure.
 *
 * Returns implementation specific information.
 *
 * \param[in] node Node number.
 * \param[out] info_data Pointer to the MCC info structure.
 */
int mcc_get_info(MCC_NODE node, MCC_INFO_STRUCT* info_data)
{
    int return_value;

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
	if(return_value != MCC_SUCCESS)
		return return_value;

	mcc_memcpy(bookeeping_data->version_string, (void*)info_data->version_string, (unsigned int)sizeof(bookeeping_data->version_string));

	/* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    return return_value;
}
