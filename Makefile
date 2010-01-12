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

TARGET = Akari

LDFILE = akari.lnk
COPYDEST = c:/Akari
ASOPTS = -gstabs --32
COPTS = -Wall -Iinc -fleading-underscore -fno-builtin -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -ggdb3 -fno-stack-protector -m32 -DDEBUG
CXXOPTS = -Wall -Iinc -fleading-underscore -fno-builtin -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -ggdb3 -fno-stack-protector -m32 -nostartfiles -nostdlib -fno-rtti -fno-exceptions -DDEBUG
LDOPTS = -melf_i386

ASMSRCS := $(wildcard *.s)
CSRCS := $(wildcard *.c)
CXXSRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.s,obj/%.s.o,$(ASMSRCS)) $(patsubst %.c,obj/%.c.o,$(CSRCS)) $(patsubst %.cpp,obj/%.cpp.o,$(CXXSRCS))

all: clean $(TARGET)-copy

KERNEL_DEFINE = `if [ \`echo $< | grep ^U_ | wc -l\` = 0 ]; then echo "-D__AKARI_KERNEL__"; fi` 

$(TARGET)-copy: $(TARGET)
	$(MTOOLS_BIN)/mcopy -D o $(TARGET) $(COPYDEST)

$(TARGET): $(OBJS) $(LDFILE) obj
	$(LD) $(LDOPTS) -T$(LDFILE) $(OBJS) -o $(TARGET)

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

