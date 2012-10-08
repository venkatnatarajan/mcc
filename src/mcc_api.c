#include "mcc_config.h"
#include "mcc_common.h"
#include "mcc_api.h"

#if MCC_OS_USED == MCC_LINUX
#include "mcc_linux.h"
#elif MCC_OS_USED == MCC_MQX
#include "mcc_mqx.h"
#endif

/*!
 * \brief This function initializes the Multi Core Communication.
 *
 * XXX  
 *
 * \param[in] node Node number.
 */
int mcc_initialize(mcc_node node)
{
    /* Initialize synchronization module */
    mcc_init_semaphore(MCC_SEMAPHORE_NUMBER);
    
    /* Register CPU-to-CPU interrupt for inter-core signalling */
    mcc_register_cpu_to_cpu_isr(MCC_CORE0_CPU_TO_CPU_VECTOR); //??
}

/*!
 * \brief This function de-initializes the Multi Core Communication.
 *
 * Clear local data, clean semaphore and share memory resources ...... 
 *
 * \param[in] node Node number to be deinitialized.
 */
int mcc_destroy(mcc_node node)
{
    int i = 0;
    
    /* Semaphore-protected section start */
    mcc_get_semaphore();

    /* All endpoints of the particular node have to be removed from the endpoint table */
    for(i = 0, i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS, i++) {
        if (bookeeping_data->endpoint_table[i].endpoint.node == node) {
            /* Remove the endpoint from the table */
            mcc_remove_endpoint(bookeeping_data->endpoint_table[i].endpoint);
        }
    }
    
    /* Semaphore-protected section end */
    mcc_release_semaphore();
    
    /* Deinitialize synchronization module */
    mcc_deinit_semaphore(MCC_SEMAPHORE_NUMBER);
    
    // TODO: Clear signal queue, Unregister the CPU-to-CPU interrupt????
}

/*!
 * \brief This function creates an endpoint.
 *
 * Create an endpoint on the local node with the specified port number.  
 *
 * \param[in] endpoint Pointer to the endpoint structure to be filled.
 * \param[in] port Port number.
 */
int mcc_create_endpoint(mcc_endpoint *endpoint, mcc_port port)
{
    int return_value = MCC_SUCCESS;
    
    /* Semaphore-protected section start */
    mcc_get_semaphore();
    
    /* Add new endpoint data into the book-keeping structure */
    return_value = mcc_register_endpoint(endpoint);
    
    /* Semaphore-protected section end */
    mcc_release_semaphore();
    
    return return_value;
}

/*!
 * \brief This function sends a message.
 *
 * The message is copied into the MCC buffer and the other core is signaled.  
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[in] msg Pointer to the meassge to be sent.
 * \param[in] msg_size Size of the meassge to be sent.
 * \param[in] timeout_us Timeout in microseconds.
 */
int mcc_send(mcc_endpoint *endpoint, void *msg, size_t msg_size, unsigned int timeout_us)
{
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf;

    /* Semaphore-protected section start */
    mcc_get_semaphore();

    /* Dequeue the buffer from the free list */
    buf = dequeue_buffer(bookeeping_data->free_list);
    
    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(endpoint);

    /* Enqueue the buffer into the endpoint buffer list */
    queue_buffer(list, buf);
    
    /* Semaphore-protected section end */
    mcc_release_semaphore();

    /* Copy the message into the MCC receive buffer */
    mcc_memcpy(msg, (void*)buf, (unsigned int)msg_size);
    
    /* Signal the other core by generating the CPU-to-CPU interrupt */
    mcc_generate_cpu_to_cpu_interrupt(void); //TODO identify the core in the parameter of this function    
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
 * \param[in] timeout_us Timeout in microseconds.
 */
int mcc_recv_copy(mcc_endpoint *endpoint, void *buffer, size_t buffer_size, size_t *recv_size, unsigned int timeout_us)
{
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf;

    /* Semaphore-protected section start */
    mcc_get_semaphore();

    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(endpoint);

    /* Semaphore-protected section end */
    mcc_release_semaphore();
    
    if(list->head != NULL) {
        /* Copy the message from the MCC receive buffer into the user-app. buffer */
        mcc_memcpy((void*)list->head.data, buffer, MCC_ATTR_BUFFER_SIZE_IN_KB * 1024);
        //TODO: what about recv_size??
    }
    
    /* Semaphore-protected section start */
    mcc_get_semaphore();

    /* Dequeue the buffer from the endpoint list */
    buf = dequeue_buffer(list);

    /* Enqueue the buffer into the free list */
    queue_buffer(bookeeping_data->free_list, buf);
    
    /* Semaphore-protected section end */
    mcc_release_semaphore();

    //TODO: implement timeout
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
 * \param[in] timeout_us Timeout in microseconds.
 */
int mcc_recv_nocopy(mcc_endpoint *endpoint, void **buffer_p, size_t *recv_size, unsigned int timeout_us)
{
    MCC_RECEIVE_LIST *list;

    /* Semaphore-protected section start */
    mcc_get_semaphore();

    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(endpoint);

    /* Semaphore-protected section end */
    mcc_release_semaphore();
    
    if(list->head != NULL) {
        /* Get the message pointer from the head of the receive buffer list */
        buffer_p = list->head.data;
        //TODO: what about recv_size??
    }

    //TODO: implement timeout
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
int mcc_msgs_available(mcc_endpoint *endpoint, unsigned int *num_msgs)
{
    unsigned int count = 0;
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf;
    
    /* Semaphore-protected section start */
    mcc_get_semaphore();

    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(endpoint);
    
    buf = list->head;
    while(buf != NULL) {
        count++;
        buf = buf->next;
    }
    *num_msgs = count;

    /* Semaphore-protected section end */
    mcc_release_semaphore();
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
int mcc_free_buffer(mcc_endpoint *endpoint, void *buffer)
{
    MCC_RECEIVE_LIST *list;
    MCC_RECEIVE_BUFFER * buf, buf_tmp;
    
    /* Semaphore-protected section start */
    mcc_get_semaphore();
    
    /* Get list of buffers kept by the particular endpoint */
    list = mcc_get_endpoint_list(endpoint);
    
    /* Dequeue the buffer from the endpoint list */
    if(list->head == (MCC_RECEIVE_BUFFER*)buffer) {
        buf = dequeue_buffer(list);
    }
    else {
        buf_tmp = list->head;
        while(buf_tmp->next != (MCC_RECEIVE_BUFFER*)buffer) {buf_tmp = buf_tmp->next};
        buf = buf_tmp->next;
        *buf_tmp->next = buf_tmp->next->next;
    }

    /* Enqueue the buffer into the free list */
    queue_buffer(bookeeping_data->free_list, buf);
    
    /* Semaphore-protected section end */
    mcc_release_semaphore();
}