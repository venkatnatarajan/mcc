#ifndef libmcc_H_INCLUDED
#define libmcc_H_INCLUDED

#include "../../../mcc_config.h"
#include "../../../mcc_common.h"

int mcc_initialize(MCC_NODE);
int mcc_destroy(MCC_NODE);
int mcc_create_endpoint(MCC_ENDPOINT*, MCC_PORT);
int mcc_destroy_endpoint(MCC_ENDPOINT*);
int mcc_send(MCC_ENDPOINT*, void*, MCC_MEM_SIZE, unsigned int);
int mcc_recv_copy(MCC_ENDPOINT*, void*, MCC_MEM_SIZE, MCC_MEM_SIZE*, unsigned int);
int mcc_recv_nocopy(MCC_ENDPOINT*, void**, MCC_MEM_SIZE*, unsigned int);
int mcc_msgs_available(MCC_ENDPOINT*, unsigned int*);
int mcc_free_buffer(MCC_ENDPOINT*, void*);

#endif /* libmcc_H_INCLUDED */
