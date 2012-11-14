#ifndef __MCC_VF600__
#define __MCC_VF600__

unsigned int mcc_get_cpu_to_cpu_vector(unsigned int);
void mcc_clear_cpu_to_cpu_interrupt(unsigned int);
void mcc_triger_cpu_to_cpu_interrupt(void);

#endif /* __MCC_VF600__ */
