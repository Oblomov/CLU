# OpenCL Utility library Makefile.
# This should not be invoked directly

ifeq ($(MAKELEVEL), 0)
$(error Please do not run make from the src directory)
endif

CFLAGS+= -std=c99

SRC=clu.c \
    clu_init.inc \
    clu_generic.inc \
    clu_platform.inc \
    clu_device.inc \
    clu_ctxque.inc \
    clu_program.inc
HDR=clu.h clu_private.h

INCPATH=-I../include
LIBS=-lOpenCL -lpthread


$(LIBPATH) : $(OUTDIR)/clu.o
	$(LD) -shared -soname=$(SONAME) -o $@ $+ $(LIBS)

$(OUTDIR)/clu.o : $(SRC) $(ΗDR)
	$(CC) -o $@ -fPIC $(CFLAGS) $(INCPATH) -c $<


