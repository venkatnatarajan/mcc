#ifndef __MCC_SEMA4_LINUX_H__
#define __MCC_SEMA4_LINUX_H__

int mcc_sema4_grab(unsigned char gate_num);
int mcc_sema4_release(unsigned char gate_num);

#endif /* __MCC_SEMA4_LINUX_H__ */
