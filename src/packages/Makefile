# This file is part of Akari.
# Copyright 2010 Arlen Cuss
# 
# Akari is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Akari is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Akari.  If not, see <http://www.gnu.org/licenses/>.

include ../Makefile.inc

PACKAGES = acpi ata fat init keyboard pci piix3 ps rtl8139 sh shutdown vfs

all: $(PACKAGES)

$(PACKAGES): force_subdir
	@cd $@; $(MAKE) $(MFLAGS) all

generate:
	@if [ -z "$$PACKAGE" ]; then \
	echo "Usage: make generate PACKAGE=xyzzy"; \
	exit 1; \
	fi
	cp -r template $$PACKAGE
	sed -i "s/template/$$PACKAGE/g" $$PACKAGE/.gitignore $$PACKAGE/Makefile $$PACKAGE/main.cpp $$PACKAGE/main.hpp

force_subdir:
	@true

clean:
	@for i in $(PACKAGES); do (cd $$i; $(MAKE) $(MFLAGS) clean); done
