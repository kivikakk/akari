include ../Makefile.inc

PACKAGES = ATA MBR Kb Shell

all: $(PACKAGES)

$(PACKAGES): force_subdir
	cd $@; $(MAKE) $(MFLAGS) all

force_subdir:
	@true

