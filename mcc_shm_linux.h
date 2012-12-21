#ifndef __MCC_SHM_LINUX_H__
#define __MCC_SHM_LINUX_H__

//extern struct mcc_bookeeping_struct *bookeeping_data;

#define VIRT_TO_MQX(a) ((a == 0) ? 0 :SHARED_IRAM_START | ((unsigned)a & 0x0000ffff))
#define MQX_TO_VIRT(a) ((a == 0) ? 0 : (unsigned)bookeeping_data | ((unsigned)a & 0x0000ffff))

int mcc_initialize_shared_mem(void);
void mcc_deinitialize_shared_mem(void);
void print_bookeeping_data(void);
int mcc_map_shared_memory(void);

#endif /* __MCC_SHM_LINUX_H__ */
