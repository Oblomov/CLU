# OpenCL Utility library Makefile.
# This should not be invoked directly

MAJOR=0
MINOR=0
PATCHLEVEL=1

VERSION=$(MAJOR).$(MINOR).$(PATCHLEVEL)

LIBBASE=libCLU.so

OUTDIR=$(CURDIR)/lib

SONAME=$(LIBBASE).$(MAJOR)
SOPATH=$(OUTDIR)/$(SONAME)
LIBNAME=$(LIBBASE).$(VERSION)
LIBPATH=$(OUTDIR)/$(LIBNAME)
DEVPATH=$(OUTDIR)/$(LIBBASE)

# Use ?= so PREFIX can still be set on the command line.
PREFIX ?= /usr/local

export OUTDIR SONAME LIBPATH

$(DEVPATH): $(SOPATH)
	ln -sf $(SONAME) $(DEVPATH)

$(SOPATH): $(LIBPATH) $(OUTDIR)
	ln -sf $(LIBNAME) $(SOPATH)

$(OUTDIR) :
	mkdir -p $@

$(LIBPATH) : $(OUTDIR)
	$(MAKE) -C src $(LIBPATH)

install: $(LIBPATH)
	install -d $(PREFIX)/lib
	install --mode=644 --target-directory=$(PREFIX)/lib $(LIBPATH)
	ln -sf $(LIBNAME) $(PREFIX)/lib/$(SONAME)
	ln -sf $(SONAME) $(PREFIX)/lib/$(LIBBASE)	
	install -d $(PREFIX)/include/CL
	install --mode=644 --target-directory=$(PREFIX)/include/CL include/CL/clu.h

.PHONY : $(LIBPATH) install
