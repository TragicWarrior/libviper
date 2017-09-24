CFLAGS = -Wall -O2 -std=gnu99 -ggdb 
DEFS = -D_REENTRANT
prefix = /usr/local
libdir = ${prefix}/lib
includedir = ${prefix}/include

makefile: all

all: clean libviper
	strip libviper.so

debug: clean libviper

vdk.o:
	cd vdk && $(MAKE)

wide: clean
	sh -c "echo '#ifndef _VIPER_WIDE'; echo '#define _VIPER_WIDE 1';echo '#endif';echo '#include <viper.h>'" > viper_wide.h
	gcc $(CFLAGS) $(DEFS) -D_VIPER_WIDE -c -fpic *.c
	gcc $(CFLAGS) -shared -o libviper_wide.so *.o
	strip libviper_wide.so

libviper: clean
	gcc $(CFLAGS) $(DEFS) -c -fpic *.c
	gcc $(CFLAGS) -shared -o libviper.so *.o

clean:
	rm -f *.o
	rm -f *.so
	rm -f viper_wide.h

install:
	chmod 644 viper.h
	cp -f viper.h $(includedir)
	chmod 755 libviper.so
	cp -f libviper.so $(libdir)
	ldconfig	

install_wide:
	chmod 644 viper.h
	cp -f viper.h $(includedir)
	chmod 644 viper_wide.h
	cp -f viper_wide.h $(includedir)
	chmod 755 libviper_wide.so
	cp -f libviper_wide.so $(libdir)
	ldconfig	

