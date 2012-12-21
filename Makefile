obj-m += mcc.o
mcc-y = mcc_linux.o mcc_shm_linux.o mcc_sema4_linux.o mcc_common.o
header-y += mcc_linux.h mcc_common.h mcc_config.h

PWD := $(shell pwd)

EXTRA_CFLAGS += -I$(KERNELDIR)/include -Wno-format

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
