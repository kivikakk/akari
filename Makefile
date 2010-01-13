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
COPYDEST = c:/Akari

ASMSRCS := $(wildcard *.s)
CSRCS := $(wildcard *.c)
CXXSRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.s,obj/%.s.o,$(ASMSRCS)) $(patsubst %.c,obj/%.c.o,$(CSRCS)) $(patsubst %.cpp,obj/%.cpp.o,$(CXXSRCS))

all: clean $(TARGET)-copy

$(TARGET)-copy: $(TARGET)
	$(MTOOLS_BIN)/mcopy -D o $(TARGET) $(COPYDEST)

$(TARGET): $(OBJS) $(LDFILE) packages obj
	$(LD) $(LDOPTS) -T$(LDFILE) $(OBJS) -o $(TARGET)

packages: force_subdir
	cd packages; $(MAKE) $(MFLAGS) all

obj/%.s.o: %.s obj
	$(AS) $(ASOPTS) -o $@ $<
obj/%.c.o: %.c obj
	$(CC) $(KERNEL_DEFINE) $(COPTS) -c -o $@ $<
obj/%.cpp.o: %.cpp obj
	$(CXX) $(KERNEL_DEFINE) $(CXXOPTS) -D__CPLUSPLUS -c -o $@ $<

obj:
	if [ ! -e obj ]; then mkdir obj; fi

clean:
	@-rm $(TARGET) $(OBJS)

force_subdir:
	@true
