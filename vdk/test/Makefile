CFLAGS = -std=c99 -Wall -ggdb -D_GNU_SOURCE
PKG_CFG = `pkg-config --cflags --libs glib-2.0`
DEFS = -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_GNU_SOURCE
LIBS = -lvdk -lncurses 
prefix = /usr/local
libdir = ${prefix}/lib
headerdir = ${prefix}/include
includedir = -I${headerdir} -I./ -I/usr/include/
bindir = ${prefix}/bin

makefile: all

all: clean vdktest

vdktest:
	gcc $(CFLAGS) $(DEFS) $(PKG_CFG) $(includdir) pausing.c -o pausing $(LIBS)
	gcc $(CFLAGS) $(DEFS) $(PKG_CFG) $(includdir) metrics.c -o metrics $(LIBS)
	gcc $(CFLAGS) $(DEFS) $(PKG_CFG) $(includdir) colors.c -o colors $(LIBS)
	gcc $(CFLAGS) $(DEFS) $(PKG_CFG) $(includdir) widgets.c -o widgets $(LIBS)


clean: 
	rm -f *.o
	rm -f pausing
	rm -f metrics
	rm -f colors
	rm -f widgets
