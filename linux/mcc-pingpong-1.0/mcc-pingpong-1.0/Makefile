INSTALL_PATH=$(DESTDIR)/usr/bin

LDFLAGS+=-lmcc

all: build/mcc-pingpong

build/mcc-pingpong: src/pingpong.c
	mkdir -p build/
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

install:
	mkdir -p $(INSTALL_PATH)
	cp -f build/mcc-pingpong $(INSTALL_PATH)

clean:
	rm -rf build
