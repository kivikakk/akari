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

TARGET = Bootloader

LDFILE = bootloader.lnk

ASMSRCS := $(wildcard *.s)
OBJS := $(patsubst %.s,obj/%.s.o,$(ASMSRCS))

all: $(TARGET)-copy

$(TARGET)-copy: $(TARGET)
	echo "none yet"

$(TARGET): $(OBJS)
	$(LD) $(LDOPTS) -T$(LDFILE) $(OBJS) -o $(TARGET)

obj/%.s.o: %.s
	$(AS) $(ASOPTS) -o $@ $<

clean:
	-rm $(TARGET) $(OBJS)

