DESTDIR=/tftpboot/rfs
INSTALL_PATH=$(DESTDIR)/usr/bin
CFLAGS+=-I../../mcc-kmod-1.0/mcc-kmod-1.0
CFLAGS+=-I../../libmcc-1.0/libmcc-1.0/include/

#LDFLAGS+=-lmcc
LDFLAGS+=-L"/media/Data/Timesys/Faraday/MCC/mccReleaseFinal/build_armv7l-timesys-linux-gnueabi/libmcc-1.0/libmcc-1.0/build"

all: build/mcc-pingpong

build/mcc-pingpong: src/pingpong.c
	mkdir -p build/
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

install:
	mkdir -p $(INSTALL_PATH)
	cp -f build/mcc-pingpong $(INSTALL_PATH)

clean:
	rm -rf build
