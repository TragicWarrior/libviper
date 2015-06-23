CFLAGS = -Wall -std=c99 -ggdb 
DEFS = -D_REENTRANT  -D_GNU_SOURCE -D_DEBUG
PKG_CFG = `pkg-config --cflags glib-2.0`
PREFIX_DIR = /usr
INCLUDE_DIR = ${PREFIX_DIR}/include/vdk
ARCH := $(shell arch)

ifeq ($(ARCH),x86_64)
LIB_ARCH = lib64
else
LIB_ARCH = lib
endif


LIB_DIR = ${PREFIX_DIR}/${LIB_ARCH}

makefile: all

all: clean vdk
	strip libvdk.so

debug: clean vdk

vdk: clean
	gcc $(CFLAGS) -I./ $(DEFS) -c -fpic *.c $(PKG_CFG)
	gcc $(CFLAGS) -I./ -shared -o libvdk.so *.o

wide: clean
	gcc $(CFLAGS) -I./ $(DEFS) -D_USE_VDK_WIDE -c -fpic *.c $(PKG_CFG)
	gcc $(CFLAGS) -I./ -shared -o libvdk_w.so *.o

clean:
	rm -f *.o
	rm -f *.so

install:
	mkdir -p $(INCLUDE_DIR)
	cp -f *.h $(INCLUDE_DIR)
	chmod 644 ${INCLUDE_DIR}/*.h
	cp -f libvdk.so ${LIB_DIR}/
	chmod 755 ${LIB_DIR}/libvdk.so
	ldconfig	

install_wide:
	mkdir -p $(INCLUDE_DIR)
	cp -f *.h $(INCLUDE_DIR)
	chmod 644 ${INCLUDE_DIR}/*.h
	cp -f libvdk_w.so ${LIB_DIR}/
	chmod 755 ${LIB_DIR}/libvdk_w.so
	ldconfig	

