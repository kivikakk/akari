include ../Makefile.inc

PACKAGES = ATA Shell Kb

all: $(PACKAGES)

$(PACKAGES): force_subdir
	cd $@; $(MAKE) $(MFLAGS) all

force_subdir:
	@true

