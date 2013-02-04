CFLAGS+=-Iinclude/
all: build/libmcc.a build/libmcc.so

build/libmcc.o: src/libmcc.c
	mkdir -p build/
	$(CC) -c -fPIC $< -o $@ $(CFLAGS)

build/libmcc.a: build/libmcc.o
	$(AR) rcs $@ $<

build/libmcc.so: build/libmcc.o
	$(CC) -shared -o $@  $<

install:
	mkdir -p $(DESTDIR)/usr/{lib,include}
	cp -f build/libmcc.{so,a} $(DESTDIR)/usr/lib
	cp -f include/*.h $(DESTDIR)/usr/include

clean:
	rm -rf build
