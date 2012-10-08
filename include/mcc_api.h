#ifndef __MCC_API__
#define __MCC_API__

int mcc_initialize(mcc_node);
int mcc_destroy(mcc_node);
int mcc_create_endpoint(mcc_endpoint*, mcc_port);
int mcc_send(mcc_endpoint*, void*, size_t, unsigned int);
int mcc_recv_copy(mcc_endpoint*, void*, size_t, size_t*, unsigned int);
int mcc_recv_nocopy(mcc_endpoint*, void**, size_t*, unsigned int);
int mcc_msgs_available(mcc_endpoint*, unsigned int*);
int mcc_free_buffer(mcc_endpoint*, void*);

#endif /* __MCC_API__ */