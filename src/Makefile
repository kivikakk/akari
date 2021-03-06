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

include Makefile.inc

TARGET = Akari

AKARI_MAJOR = 0
AKARI_MINOR = 1
AKARI_VERSION = \"$(AKARI_MAJOR).$(AKARI_MINOR)\"

LDFILE = akari.lnk
COPYDEST = c:/Akari
MENUDEST = c:/boot/grub/menu.lst

ASMSRCS := $(wildcard *.s)
CXXSRCS := $(wildcard *.cpp)
CXXDEPS := $(patsubst %.cpp,obj/%.cpp.d,$(CXXSRCS))
OBJS := $(patsubst %.s,obj/%.s.o,$(ASMSRCS)) $(patsubst %.cpp,obj/%.cpp.o,$(CXXSRCS))

all: $(TARGET)-copy

$(TARGET)-copy: $(TARGET) menu.lst
	@echo "[MCOPY] $(TARGET)"
	@$(MTOOLS_BIN)/mcopy -D o $(TARGET) $(COPYDEST)
	@echo "[MCOPY] menu.lst"
	@$(MTOOLS_BIN)/mcopy -D o menu.lst $(MENUDEST)

$(TARGET): $(OBJS) $(LDFILE) user packages ../test
	@echo "[LD]    $@"
	@$(LD) $(LDOPTS) -T$(LDFILE) $(OBJS) -o $(TARGET)
	@echo "[STRIP] $@"
	@$(STRIP) $(TARGET)

../test: force_subdir
	@echo "[TEST]"
	@cd ../test; $(MAKE) $(MFLAGS) all

packages: force_subdir user
	@cd packages; $(MAKE) $(MFLAGS) all

user: force_subdir
	@cd user; $(MAKE) $(MFLAGS) all

-include $(CXXDEPS)

obj/%.cpp.d: %.cpp
	@echo "[DEP] $<"
	@$(CXX) -M $(CXXOPTS) -D__AKARI_KERNEL -D__CPLUSPLUS $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

obj/%.s.o: %.s
	@echo "[AS]  $<"
	@$(AS) $(ASOPTS) -o $@ $<
obj/%.cpp.o: %.cpp
	@echo "[CXX] $<"
	@$(CXX) $(CXXOPTS) -D__AKARI_VERSION=$(AKARI_VERSION) -D__AKARI_KERNEL -D__CPLUSPLUS -c -o $@ $<

clean:
	@-rm $(TARGET) $(OBJS) $(CXXDEPS)
	@-cd user; make clean
	@-cd packages; make clean

force_subdir:
	@true
