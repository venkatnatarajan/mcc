/*
 * mcc_linux.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/mvf-ipc.h>
#include <linux/mvf-sema4.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

#include <mcc_config.h>
#include "mcc_linux.h"
#include <mcc_common.h>
/*
 * Linux Device Driver Names:
 * /dev/mvf_shared_iram
 * /dev/mvf_sema4
 * /dev/mvf_ipc
 */
//Devices for Sema4, IP
int sem_dev;
int ipc_dev;
int shared_iram_dev;
unsigned int gate_num = 0;
unsigned int vector_num = 0;

//TODO: determine the proper place for this define.
#define SHARED_IRAM_SIZE (64*1024)

/* This field contains CPU-to-CPU interrupt vector numbers for all device cores */
static const unsigned int mcc_cpu_to_cpu_vectors[] = { 48, 16 };

/*!
 * \brief This function initializes the hw sempahore by opening the sema4 linux driver.
 *
 * \param[in] sem_num - SEMA4 gate number.
 */
int mcc_init_semaphore(unsigned int sem_num)
{
	printf("Opening the Semaphore Driver\n");
	if ((sem_dev = open("/dev/mvf_sema4", O_RDWR)) < 0)
	{
		printf ("Failed to open Sema4 Driver\n");
		return MCC_ERR_DEV;
	}
	gate_num = sem_num;
	return MCC_SUCCESS;
}

int mcc_init_shared_iram()
{
	int shared_iram_pointer;
	printf("Opening the iram Driver\n");
	if ((shared_iram_dev = open("/dev/mvf_shared_iram", O_RDWR, 0)) < 0)
	{
		printf("Unable to open /dev/mvf_shared_iram\n");
		return MCC_ERR_DEV;
	}

	shared_iram_pointer = (int) mmap(0, SHARED_IRAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shared_iram_dev, 0);
	if (shared_iram_pointer <= 0)
	{
			printf("Error: failed to map shared_iram device to memory.\n");
			return  MCC_ERR_DEV;
	}
	printf("Memory map successful, pointer at address %x\n",(unsigned int)shared_iram_pointer);
	return shared_iram_pointer;
}

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
static void mcc_cpu_to_cpu_isr()
{
    MCC_SIGNAL serviced_signal;
    int return_value;

	printf("In mcc_cpu_to_cpu_isr user space handler\n");
	return_value = mcc_get_semaphore();

	if(return_value == MCC_SUCCESS)
	{
		serviced_signal = bookeeping_data->signals_received[MCC_CORE_NUMBER][bookeeping_data->signal_queue_head[MCC_CORE_NUMBER]];
    	bookeeping_data->signal_queue_head[MCC_CORE_NUMBER] = (bookeeping_data->signal_queue_head[MCC_CORE_NUMBER] == MCC_MAX_OUTSTANDING_SIGNALS) ? 0 : bookeeping_data->signal_queue_head[MCC_CORE_NUMBER] + 1;

    	mcc_release_semaphore();
        /*
        if(serviced_signal.type == BUFFER_QUEUED) {
            // TODO unblock receiver, in case of asynchronous communication
        }
        else if(serviced_signal.type == BUFFERED_FREED) {
            // TODO unblock sender, in case of asynchronous communication
        }*/
	}
	else
		printf("Error queuing buffer in ISR\n");
}

int mcc_register_cpu_to_cpu_isr(void)
{
	unsigned short interrupt_id;
	struct sigaction sig;
	int ret_code;

	printf("Opening the ipc Driver\n");
	if ((ipc_dev = open("/dev/mvf_ipc", O_RDWR, 0)) < 0)
	{
		printf("Unable to open IPC Driver\n");
		return MCC_ERR_DEV;
	}

	vector_num = mcc_get_cpu_to_cpu_vector(MCC_CORE_NUMBER);

	interrupt_id = CPU_INT1_ID;

	ret_code = ioctl(ipc_dev, MVF_REGISTER_PID_WITH_CPU_INTERRUPT, &interrupt_id);
	if(ret_code != 0)
	{
		printf("Error registering PID with interrupt. Error code:%d\n",ret_code);
		return MCC_ERR_INT;
	}

	printf("User application realtime signal number:%d\n",interrupt_id);
	sig.sa_sigaction = mcc_cpu_to_cpu_isr;
	sig.sa_flags = SA_SIGINFO;
	ret_code = sigaction(interrupt_id, &sig, NULL);
	if(ret_code != 0)
	{
		printf("Error registering signal. Error code:%d\n",ret_code);
		return MCC_ERR_INT;
	}

	return MCC_SUCCESS;
}

void mcc_unregister_cpu_to_cpu_isr(void)
{
	close(ipc_dev);
}

void mcc_deinit_semaphore(unsigned int sem_num)
{
    close(sem_dev);
}

void mcc_deinit_shared_iram()
{
	close(shared_iram_dev);
}

unsigned int mcc_get_cpu_to_cpu_vector(unsigned int core)
{
   if (core < (sizeof(mcc_cpu_to_cpu_vectors)/sizeof(mcc_cpu_to_cpu_vectors[0]))) {
      return  mcc_cpu_to_cpu_vectors[core];

   }
   return 0;
}

int mcc_get_semaphore(void)
{
	int ret_code;

	ret_code = ioctl(sem_dev, MVF_SEMA4_LOCK, &gate_num);
    if(ret_code != 0)
    {
		printf("Error obtaining lock. Error code:%d\n",ret_code);
		return MCC_ERR_SEMAPHORE;
	}

	//printf("Lock obtained\n");
	return MCC_SUCCESS;
}

int mcc_release_semaphore(void)
{
	int ret_code;

    ret_code = ioctl(sem_dev, MVF_SEMA4_UNLOCK, &gate_num);
    if(ret_code != 0)
    {
		printf("Error unlocking. Error code:%d\n",ret_code);
		return MCC_ERR_SEMAPHORE;
	}

    //printf("Unlocked\n");
	return MCC_SUCCESS;
}

int mcc_generate_cpu_to_cpu_interrupt(void)
{
	int ret_code = 0;
	cpu_interrupt_info_t interrupt_info;

	interrupt_info.interrupt_type = CP0_INTERRPUT_ONLY;
	interrupt_info.interrupt_id = CPU_INT1_ID;

	ret_code =  ioctl(ipc_dev, MVF_GENERATE_CPU_TO_CPU_INTERRUPT, &interrupt_info);
	if(ret_code != 0)
	{
		printf("Error requesting interrupt generation. Error code: %d\n",ret_code);
		return MCC_ERR_INT;
	}
	//printf("Interrupt successfully generated\n");
	return MCC_SUCCESS;
}

int mcc_memcpy(void *src, void *dest, unsigned int size)
{
	int dest_mem_ptr;
	dest_mem_ptr = (int) memcpy(dest, src, size);

	if(dest_mem_ptr == (int)null)
		return MCC_ERR_NOMEM;

	return MCC_SUCCESS;
}

unsigned int mcc_time_get_microseconds(void)
{
	return 0;
}
