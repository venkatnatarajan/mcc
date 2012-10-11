#ifndef __MCC_MQX__
#define __MCC_MQX__

/* Semaphore-related functions */
int mcc_init_semaphore(unsigned int);
int mcc_deinit_semaphore(unsigned int);
int mcc_get_semaphore(void);
int mcc_release_semaphore(void);

/* CPU-to-CPU interrupt-related functions */
int mcc_register_cpu_to_cpu_isr(void);
void mcc_generate_cpu_to_cpu_interrupt(void);

/* Memory management-related functions */
void mcc_memcpy(void*, void*, unsigned int);

/* Time-related functions */
unsigned int mcc_time_get_microseconds(void);

#endif /* __MCC_MQX__ */