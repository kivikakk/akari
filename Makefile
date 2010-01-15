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
# along with Akari.  If not, see <http://www.gnu.org/licenses/

include Makefile.inc

TARGET = Akari

LDFILE = akari.lnk
COPYDEST = a:/Akari

ASMSRCS := $(wildcard *.s)
CXXSRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.s,obj/%.s.o,$(ASMSRCS)) $(patsubst %.cpp,obj/%.cpp.o,$(CXXSRCS))

# Linkage target set in Makefile.inc, since packages need to know that too.
LINKAGE_CXXSRCS := $(wildcard User*.cpp)
LINKAGE_OBJS := $(patsubst %.cpp,obj/linkage/%.cpp.o,$(LINKAGE_CXXSRCS))

all: clean $(TARGET)-copy

$(TARGET)-copy: $(TARGET) menu.lst
	$(MTOOLS_BIN)/mcopy -D o $(TARGET) $(COPYDEST)
	$(MTOOLS_BIN)/mcopy -D o menu.lst a:/boot/grub/menu.lst

$(TARGET): $(OBJS) $(LDFILE) packages obj $(LINKAGE_TARGET)
	$(LD) $(LDOPTS) -T$(LDFILE) $(OBJS) -o $(TARGET)

packages: force_subdir $(LINKAGE_TARGET)
	cd packages; $(MAKE) $(MFLAGS) all

obj/%.s.o: %.s obj
	$(AS) $(ASOPTS) -o $@ $<
obj/%.cpp.o: %.cpp obj
	$(CXX) $(CXXOPTS) -D__AKARI_KERNEL -D__CPLUSPLUS -c -o $@ $<

obj:
	if [ ! -e obj ]; then mkdir obj; fi

obj/linkage: obj
	if [ ! -e obj/linkage ]; then mkdir obj/linkage; fi

$(LINKAGE_TARGET): $(LINKAGE_OBJS)
	$(AR) rcs $@ $(LINKAGE_OBJS)

obj/linkage/%.cpp.o: %.cpp obj/linkage
	$(CXX) $(CXXOPTS) -D__AKARI_LINKAGE -D__CPLUSPLUS -c -o $@ $<

clean:
	@-rm $(TARGET) $(OBJS) $(LINKAGE_TARGET) $(LINKAGE_OBJS)

force_subdir:
	@true
