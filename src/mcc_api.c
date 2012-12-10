#include "mcc_config.h"
#include "mcc_common.h"
#include "mcc_api.h"

#if (MCC_OS_USED == MCC_LINUX)
#include "mcc_linux.h"
#elif (MCC_OS_USED == MCC_MQX)
#include "mcc_mqx.h"
extern LWSEM_STRUCT   lwsem_buffer_queued[MCC_NUM_CORES];
extern LWSEM_STRUCT   lwsem_buffer_freed[MCC_NUM_CORES];
#endif

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
    _lwsem_create(&lwsem_buffer_queued[MCC_CORE_NUMBER], 0);
    _lwsem_create(&lwsem_buffer_freed[MCC_CORE_NUMBER], 0);
    _DCACHE_DISABLE(); //TODO should be in the OS-specific initialization
#endif
    /* Initialize synchronization module */
    return_value = mcc_init_semaphore(MCC_SEMAPHORE_NUMBER);
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* Register CPU-to-CPU interrupt for inter-core signalling */
    //mcc_register_cpu_to_cpu_isr(MCC_CORE0_CPU_TO_CPU_VECTOR);
    return_value = mcc_register_cpu_to_cpu_isr();
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* Initialize the bookeeping structure */
    if(bookeeping_data->init_flag == 0) {

    	/* Set init_flag in case it has not been set yet by another core */
        bookeeping_data->init_flag = 1;

        /* Initialize the free list */
        bookeeping_data->free_list.head = &bookeeping_data->r_buffers[0];
        bookeeping_data->free_list.tail = &bookeeping_data->r_buffers[MCC_ATTR_NUM_RECEIVE_BUFFERS-1];

        /* Initialize receive buffers */
        for(i=0; i<MCC_ATTR_NUM_RECEIVE_BUFFERS-1; i++) {
        	bookeeping_data->r_buffers[i].next = &bookeeping_data->r_buffers[i+1];
        }
        bookeeping_data->r_buffers[MCC_ATTR_NUM_RECEIVE_BUFFERS-1].next = &bookeeping_data->r_buffers[0];

        /* Initialize signal queues */
        for(i=0; i<MCC_NUM_CORES; i++) {
        	for(j=0; j<MCC_MAX_OUTSTANDING_SIGNALS; j++) {
        	    bookeeping_data->signals_received[i][j] = tmp_signals_received;
        	}
        	bookeeping_data->signal_queue_head[i] = 0;
            bookeeping_data->signal_queue_tail[i] = 0;
        }
    }
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
    _lwsem_destroy(&lwsem_buffer_queued[MCC_CORE_NUMBER]);
    _lwsem_destroy(&lwsem_buffer_freed[MCC_CORE_NUMBER]);
#endif

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
    if(return_value != MCC_SUCCESS)
    	return return_value;

    /* All endpoints of the particular node have to be removed from the endpoint table */
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
 * \brief This function destrois an endpoint.
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
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffff = blocking call .
 */
int mcc_send(MCC_ENDPOINT *endpoint, void *msg, MCC_MEM_SIZE msg_size, unsigned int timeout_us)
{
	int return_value;
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf;
    MCC_SIGNAL affiliated_signal;
    unsigned int time_us_tmp;
    MCC_BOOLEAN timeout = TRUE;

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

    if(list != null) {
        /* Dequeue the buffer from the free list */
        buf = mcc_dequeue_buffer(&bookeeping_data->free_list);

        while(buf == null) {
        	/* Non-blocking call */
        	if(timeout_us == 0) {
        		mcc_release_semaphore();
#if (MCC_OS_USED == MCC_MQX)
        		_lwsem_wait(&lwsem_buffer_freed[endpoint->core]);
#endif
        		mcc_get_semaphore();
        		buf = mcc_dequeue_buffer(&bookeeping_data->free_list);
        	}
        	/* Blocking call - wait forever TBD */
        	else if(timeout_us == 0xFFFF) {
        	}
        	/* timeout_us > 0 */
        	else {
        		mcc_release_semaphore();
        		time_us_tmp = mcc_time_get_microseconds();
        		while((mcc_time_get_microseconds() - time_us_tmp) < timeout_us) {
        			mcc_get_semaphore();
        			/* Dequeue the buffer from the free list */
        			buf = mcc_dequeue_buffer(&bookeeping_data->free_list);
        			if(buf ==  null) {
        				mcc_release_semaphore();
        			}
        			else {
        				timeout = FALSE;
        				break;
        			}
        		}
        		if(timeout == FALSE) {
        			/* Buffer dequeued before the timeout */
        			break;
        		}
        		else {
        			/* Buffer not dequeued before the timeout */
        			return MCC_ERR_TIMEOUT;
        		}
        	}
        }

        /* Enqueue the buffer into the endpoint buffer list */
        mcc_queue_buffer(list, buf);

        /* Write the signal type into the signal queue of the particular core */
        affiliated_signal.type = BUFFER_QUEUED;
        affiliated_signal.destination = *endpoint;
        bookeeping_data->signals_received[endpoint->core][bookeeping_data->signal_queue_tail[endpoint->core]] = affiliated_signal;
        if (MCC_MAX_OUTSTANDING_SIGNALS-1 > bookeeping_data->signal_queue_tail[endpoint->core]) {
     	    bookeeping_data->signal_queue_tail[endpoint->core]++;
        }
        else {
    	    bookeeping_data->signal_queue_tail[endpoint->core] = 0;
        }
    }
    else {
    	/* The endpoint does not exists (has not been registered so far), return immediately - error */
    	mcc_release_semaphore();
    	return MCC_ERR_ENDPOINT;
    }

    /* Semaphore-protected section end */
    mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    /* Copy the message into the MCC receive buffer */
    mcc_memcpy(msg, (void*)buf->data, (unsigned int)msg_size);

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
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffff = blocking call .
 */
int mcc_recv_copy(MCC_ENDPOINT *endpoint, void *buffer, MCC_MEM_SIZE buffer_size, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
{
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf;
    MCC_SIGNAL affiliated_signal;
    MCC_ENDPOINT tmp_destination = {(MCC_CORE)0, (MCC_NODE)0, (MCC_PORT)0};
    int return_value, i = 0;
    unsigned int time_us_tmp;
    MCC_BOOLEAN timeout = TRUE;

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

    while(list->head == (MCC_RECEIVE_BUFFER*)0) {
    	/* Non-blocking call */
    	if(timeout_us == 0) {
#if (MCC_OS_USED == MCC_MQX)
    		_lwsem_wait(&lwsem_buffer_queued[endpoint->core]);
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
    	}
    	/* Blocking call TODO */
    	else if(timeout_us == 0xFFFF) {

    	}
    	/* timeout_us > 0 */
    	else {
    		time_us_tmp = mcc_time_get_microseconds();
    		while((mcc_time_get_microseconds() - time_us_tmp) < timeout_us) {
    			mcc_get_semaphore();
    			/* Get list of buffers kept by the particular endpoint */
    			list = mcc_get_endpoint_list(*endpoint);
    			mcc_release_semaphore();
    			if(list->head != (MCC_RECEIVE_BUFFER*)0) {
    				timeout = FALSE;
    				break;
    			}
    		}
			if(timeout == FALSE) {
				/* Buffer dequeued before the timeout */
				break;
			}
			else {
				/* Buffer not dequeued before the timeout */
				return MCC_ERR_TIMEOUT;
			}
    	}
    }

    /* Copy the message from the MCC receive buffer into the user-app. buffer */
    mcc_memcpy((void*)list->head->data, buffer, buffer_size);

    /* Semaphore-protected section start */
    return_value = mcc_get_semaphore();
	if(return_value != MCC_SUCCESS)
		return return_value;

    /* Dequeue the buffer from the endpoint list */
    buf = mcc_dequeue_buffer(list);

    /* Enqueue the buffer into the free list */
    mcc_queue_buffer(&bookeeping_data->free_list, buf);

    /* Notify all cores (except of itself) via CPU-to-CPU interrupt that a buffer has been freed */
    affiliated_signal.type = BUFFER_FREED;
    affiliated_signal.destination = tmp_destination;
    for (i=0; i<MCC_NUM_CORES; i++) {
        if(i != MCC_CORE_NUMBER) {
    	    bookeeping_data->signals_received[i][bookeeping_data->signal_queue_tail[i]] = affiliated_signal;
            if (MCC_MAX_OUTSTANDING_SIGNALS-1 > bookeeping_data->signal_queue_tail[i]) {
    	        bookeeping_data->signal_queue_tail[i]++;
            }
            else {
        	    bookeeping_data->signal_queue_tail[i] = 0;
            }
        }
    }
    mcc_generate_cpu_to_cpu_interrupt(); //TODO ensure interrupt generation into all cores

    /* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    //TODO: what about recv_size??

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
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffff = blocking call .
 */
int mcc_recv_nocopy(MCC_ENDPOINT *endpoint, void **buffer_p, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
{
    MCC_RECEIVE_LIST *list;
    int return_value;
    unsigned int time_us_tmp;
    MCC_BOOLEAN timeout = TRUE;

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

    while(list->head == (MCC_RECEIVE_BUFFER*)0) {
    	/* Non-blocking call */
    	if(timeout_us == 0) {
#if (MCC_OS_USED == MCC_MQX)
    		_lwsem_wait(&lwsem_buffer_queued[endpoint->core]);
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
    	}
    	/* Blocking call TODO */
    	else if(timeout_us == 0xFFFF) {

    	}
    	/* timeout_us > 0 */
    	else {
    		time_us_tmp = mcc_time_get_microseconds();
    		while((mcc_time_get_microseconds() - time_us_tmp) < timeout_us) {
    			mcc_get_semaphore();
    			/* Get list of buffers kept by the particular endpoint */
    			list = mcc_get_endpoint_list(*endpoint);
    			mcc_release_semaphore();
    			if(list->head != (MCC_RECEIVE_BUFFER*)0) {
    				timeout = FALSE;
    				break;
    			}
    		}
			if(timeout == FALSE) {
				/* Buffer dequeued before the timeout */
				break;
			}
			else {
				/* Buffer not dequeued before the timeout */
				return MCC_ERR_TIMEOUT;
			}
    	}
    }

    /* Get the message pointer from the head of the receive buffer list */
    buffer_p = (void**)&list->head->data;
    //TODO: what about recv_size??

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
    mcc_queue_buffer(&bookeeping_data->free_list, buf);

    /* Notify all cores (except of itself) via CPU-to-CPU interrupt that a buffer has been freed */
    affiliated_signal.type = BUFFER_FREED;
    affiliated_signal.destination = tmp_destination;
    for (i=0; i<MCC_NUM_CORES; i++) {
    	if(i != MCC_CORE_NUMBER) {
    	    bookeeping_data->signals_received[i][bookeeping_data->signal_queue_tail[i]] = affiliated_signal;
            if (MCC_MAX_OUTSTANDING_SIGNALS-1 > bookeeping_data->signal_queue_tail[i]) {
    	        bookeeping_data->signal_queue_tail[i]++;
            }
            else {
        	    bookeeping_data->signal_queue_tail[i] = 0;
            }
        }
    }
    mcc_generate_cpu_to_cpu_interrupt(); //TODO ensure interrupt generation into all cores

    /* Semaphore-protected section end */
    return_value = mcc_release_semaphore();
    if(return_value != MCC_SUCCESS)
     	return return_value;

    return return_value;
}
