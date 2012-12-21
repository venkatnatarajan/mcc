all: build/libmcc.a build/libmcc.so

build/libmcc.o: src/libmcc.c
	mkdir -p build/
	$(CC) -c -fPIC $< -o $@

build/libmcc.a: build/libmcc.o
	$(AR) rcs $@ $<

build/libmcc.so: build/libmcc.o
	$(CC) -shared -Wl -o $@  $<

install:
	mkdir -p $(DESTDIR)/usr/{lib,include}
	cp -f build/libmcc.{so,a} $(DESTDIR)/usr/lib
	cp -f src/libmcc.h $(DESTDIR)/usr/include

clean:
	rm -rf build
