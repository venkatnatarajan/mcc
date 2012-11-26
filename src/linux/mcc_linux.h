#ifndef __MCC_LINUX_H_
#define __MCC_LINUX_H_

/* Semaphore-related functions */
int mcc_init_semaphore(unsigned int);
void mcc_deinit_semaphore(unsigned int);
int mcc_get_semaphore(void);
int mcc_release_semaphore(void);

/* CPU-to-CPU interrupt-related functions */
int mcc_register_cpu_to_cpu_isr(void);
void mcc_unregister_cpu_to_cpu_isr(void);
int mcc_generate_cpu_to_cpu_interrupt(void);
unsigned int mcc_get_cpu_to_cpu_vector(unsigned int);

/* Memory management-related functions */
int mcc_init_shared_iram();
void mcc_deinit_semphore();
int mcc_memcpy(void*, void*, unsigned int);

/* Time-related functions */
unsigned int mcc_time_get_microseconds(void);

#endif /* __MCC_LINUX_H_ */
