#ifndef __MCC_CONFIG__
#define __MCC_CONFIG__

/* used OS */
#define MCC_OS_USED   MCC_MQX

/* base address of shared memory */
#define MCC_BASE_ADDRESS BSP_SHARED_RAM_START

/* size and number of receive buffers */
#define MCC_ATTR_NUM_RECEIVE_BUFFERS 10
#define MCC_ATTR_BUFFER_SIZE_IN_KB 1

/* maximum number of receive endpoints per core */
#define MCC_ATTR_MAX_RECEIVE_ENDPOINTS 5

/* size of the signal queue */
#define MCC_MAX_OUTSTANDING_SIGNALS 10

/* number of cores */
#define MCC_NUM_CORES    2

/* core number */
#define MCC_CORE_NUMBER  (_psp_core_num())

/* semaphore number */
#define MCC_SEMAPHORE_NUMBER  1 //????, see core_mutex MQX example

/* core0 CPU-to-CPU interrupt vector number */
#define MCC_CORE0_CPU_TO_CPU_VECTOR   48

/* core1 CPU-to-CPU interrupt vector number */
#define MCC_CORE1_CPU_TO_CPU_VECTOR   16

#endif /* __MCC_CONFIG__ */