include ../Makefile.inc

PACKAGES = ATA

all: $(PACKAGES)

$(PACKAGES): force_subdir
	cd $@; $(MAKE) $(MFLAGS) all

force_subdir:
	true

