MAJOR=0
VERSION=$(MAJOR).0.1
LIBBASENAME=libCLU.so
SONAME=$(LIBBASENAME).$(MAJOR)
LIBNAME=$(LIBBASENAME).$(VERSION)

.DEFAULT_GOAL:=$(LIBNAME)

CFLAGS+= -std=c99


clu.o : clu.c clu.h
	$(CC) -fPIC $(CFLAGS) -c clu.c

$(LIBNAME) : clu.o
	$(LD) -shared -soname=$(SONAME) -o $(LIBNAME) $+ -lOpenCL

