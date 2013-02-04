INSTALL_PATH=$(DESTDIR)/usr/bin

all: build/mqxboot

build/mqxboot: src/mqxboot.c
	mkdir -p build/
	$(CC) $< -o $@ $(CFLAGS)

install:
	mkdir -p $(INSTALL_PATH)
	cp -f build/mqxboot $(INSTALL_PATH)

clean:
	rm -rf build
