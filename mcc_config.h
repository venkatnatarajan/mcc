#ifndef __MCC_CONFIG__
#define __MCC_CONFIG__

/* used OS */
#define MCC_OS_USED                    (MCC_LINUX)

/* base address of shared memory */
#define MCC_BASE_ADDRESS               (BSP_SHARED_RAM_START)

/* size and number of receive buffers */
#define MCC_ATTR_NUM_RECEIVE_BUFFERS   (10)
#define MCC_ATTR_BUFFER_SIZE_IN_KB     (1)

/* maximum number of receive endpoints */
#define MCC_ATTR_MAX_RECEIVE_ENDPOINTS (5)

/* size of the signal queue */
#define MCC_MAX_OUTSTANDING_SIGNALS    (10)

/* number of cores */
#define MCC_NUM_CORES                  (2)

/* core number */
#define MCC_CORE_NUMBER                (0)

/* other cores, besides this participating in mcc */
#define MCC_OTHER_CORES			{1}

/* semaphore number */
#define MCC_SEMAPHORE_NUMBER           (1)

#endif /* __MCC_CONFIG__ */
