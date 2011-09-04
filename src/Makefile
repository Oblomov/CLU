MAJOR=0
VERSION=$(MAJOR).0.1
LIBBASENAME=libCLU.so
SONAME=$(LIBBASENAME).$(MAJOR)
LIBNAME=$(LIBBASENAME).$(VERSION)

.DEFAULT_GOAL:=$(LIBNAME)

CFLAGS+= -std=c99

SRC=clu.c \
    clu_init.inc \
    clu_generic.inc \
    clu_platform.inc \
    clu_device.inc \
    clu_program.inc \
    clu_ctxque.inc
HDR=clu.h clu_private.h

INCPATH=-I../include
LIBS=-lOpenCL -lpthread

clu.o : $(SRC) $(ΗDR)
	$(CC) -v -v -v -fPIC $(CFLAGS) $(INCPATH) -c $<

$(LIBNAME) : clu.o
	$(LD) -shared -soname=$(SONAME) -o $(LIBNAME) $+ $(LIBS)

