#include "mcc_config.h"
#include "mcc_common.h"
#include "mcc_api.h"
#include "mcc_mqx.h"
#include "mcc_vf600.h"

/* Global variables */
CORE_MUTEX_PTR cm_ptr;
LWSEM_STRUCT   lwsem_buffer_queued[MCC_NUM_CORES];
LWSEM_STRUCT   lwsem_buffer_freed[MCC_NUM_CORES];

/*!
 * \brief This function is the CPU-to-CPU interrupt handler.
 *
 * Each core can interrupt the other. There are two logical signals:
 * \n - Receive data available for (Node,Port) - signaled when a buffer is queued to a Receive Data Queue.
 * \n - Buffer available - signaled when a buffer is queued to the Free Buffer Queue.
 * \n It is possible that several signals can occur while one interrupt is being processed. 
 *  Therefore, a Receive Signal Queue of received signals is also required - one for each core. 
 *  The interrupting core queues to the tail and the interrupted core pulls from the head.
 *  For a circular file, no semaphore is required since only the sender modifies the tail and only the receiver modifies the head.   
 *
 * \param[in] param Pointer to data being passed to the ISR.
 */
static void mcc_cpu_to_cpu_isr(void *param)
{
    MCC_SIGNAL serviced_signal;
    
    /* Clear the interrupt flag */
    mcc_clear_cpu_to_cpu_interrupt(MCC_CORE_NUMBER);

    while(bookeeping_data->signal_queue_head[MCC_CORE_NUMBER] != bookeeping_data->signal_queue_tail[MCC_CORE_NUMBER]) {
        serviced_signal = bookeeping_data->signals_received[MCC_CORE_NUMBER][bookeeping_data->signal_queue_head[MCC_CORE_NUMBER]];
        if (MCC_MAX_OUTSTANDING_SIGNALS-1 > bookeeping_data->signal_queue_head[MCC_CORE_NUMBER]) {
    	    bookeeping_data->signal_queue_head[MCC_CORE_NUMBER]++;
        }
        else {
    	    bookeeping_data->signal_queue_head[MCC_CORE_NUMBER] = 0;
        }
    
        if(serviced_signal.type == BUFFER_QUEUED) {
            /* Unblock receiver, in case of asynchronous communication */
            _lwsem_post(&lwsem_buffer_queued[serviced_signal.destination.core]);
        }
        else if(serviced_signal.type == BUFFER_FREED) {
            /* Unblock sender, in case of asynchronous communication */
            _lwsem_post(&lwsem_buffer_freed[MCC_CORE_NUMBER]);
        }
    }
}

/*!
 * \brief This function initializes the hw semaphore (SEMA4).
 *
 * Calls MQX core-mutex driver to create a core mutex.  
 *
 * \param[in] sem_num SEMA4 gate number.
 */
int mcc_init_semaphore(unsigned int sem_num)
{
    /* Create a core mutex */
    cm_ptr = _core_mutex_create(0, sem_num, MQX_TASK_QUEUE_FIFO);
    //TODO: MCC_CORE_NUMBER???, maybe pass it via param

    if(NULL == cm_ptr)
        return MCC_ERR_SEMAPHORE;
    else
        return MCC_SUCCESS;
}

/*!
 * \brief This function de-initializes the hw semaphore (SEMA4).
 *
 * Calls MQX core-mutex driver to destroy a core mutex.  
 *
 * \param[in] sem_num SEMA4 gate number.
 */
int mcc_deinit_semaphore(unsigned int sem_num)
{
    /* Destroy the core mutex */
    if(MQX_OK == _core_mutex_destroy(cm_ptr))
        return MCC_SUCCESS;
    else
        return MCC_ERR_SEMAPHORE;
}

/*!
 * \brief This function locks the specified core mutex.
 *
 * Calls MQX core-mutex driver to lock the core mutex.  
 *
 */
int mcc_get_semaphore(void)
{
    if(MQX_OK == _core_mutex_lock(cm_ptr))
        return MCC_SUCCESS;
    else
        return MCC_ERR_SEMAPHORE;
}

/*!
 * \brief This function unlocks the specified core mutex.
 *
 * Calls MQX core-mutex driver to unlock the core mutex.  
 *
 */
int mcc_release_semaphore(void)
{
    if(MQX_OK == _core_mutex_unlock(cm_ptr))
        return MCC_SUCCESS;
    else
        return MCC_ERR_SEMAPHORE;
}

/*!
 * \brief This function registers the CPU-to-CPU interrupt.
 *
 * Calls MQX interrupt component functions to install and enable the CPU-to-CPU interrupt.  
 *
 */
int mcc_register_cpu_to_cpu_isr(void)
{
    unsigned int vector_number;
    
    vector_number = mcc_get_cpu_to_cpu_vector(MCC_CORE_NUMBER);
    
    if(vector_number != 0) {
        _int_install_isr((_mqx_uint)vector_number, mcc_cpu_to_cpu_isr, NULL);
        _bsp_int_init(vector_number, 3, 0, TRUE); //TODO: handle priority value correctly, define in BSP?
        _bsp_int_enable(vector_number);
        return MCC_SUCCESS;
    }
    else {
    	return MCC_ERR_INT;
    }
}

/*!
 * \brief This function triggers an interrupt to other core(s).
 *
 */
int mcc_generate_cpu_to_cpu_interrupt(void)
{
    /* Assert directed CPU interrupts for all processors except the requesting core */
    mcc_triger_cpu_to_cpu_interrupt();
    
    //TODO: distinguish the core via the function parameter???

    return MCC_SUCCESS;
}

/*!
 * \brief This function copies data.
 *
 * Copies the number of single-addressable units from the source address to destination address.    
 *
 * \param[in] src Source address.
 * \param[in] dest Destination address.
 * \param[in] size Number of single-addressable units to copy.
 */
void mcc_memcpy(void *src, void *dest, unsigned int size)
{
    _mem_copy((pointer)src, (pointer)dest, (_mem_size)size);
}

/*!
 * \brief This function returns actual system time in microseconds.
 */
unsigned int mcc_time_get_microseconds(void)
{
    TIME_STRUCT mqx_time;

    _time_get(&mqx_time);
    return ((1000 * mqx_time.MILLISECONDS) + (1000000 * mqx_time.SECONDS));
}
