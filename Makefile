obj-m += mcc.o
mcc-y = mcc_linux.o mcc_shm_linux.o mcc_sema4_linux.o ../../mcc_common.o

PWD := $(shell pwd)

EXTRA_CFLAGS += -I$(KDIR)/include -Wno-format

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
