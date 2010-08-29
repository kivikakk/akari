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


# This is from the perspective of a package.
include ../../Makefile.inc

ASMSRCS := $(wildcard *.s)
CXXSRCS := $(wildcard *.cpp)
CXXDEPS := $(patsubst %.cpp,obj/%.cpp.d,$(CXXSRCS))
OBJS := $(patsubst %.s,obj/%.s.o,$(ASMSRCS)) $(patsubst %.cpp,obj/%.cpp.o,$(CXXSRCS))

COPTS += -I../../inc -I../../user/inc
CXXOPTS += -I../../inc -I../../user/inc

all: $(TARGET)-copy

$(TARGET)-copy: $(TARGET)
	@echo "[MCOPY] $<"
	@$(MTOOLS_BIN)/mcopy -D o $(TARGET) $(COPYDEST)

$(TARGET): $(OBJS) ../../$(LINKAGE_TARGET) obj
	@echo "[LD]    $@"
	@$(LD) -melf_i386 $(OBJS) ../../$(LINKAGE_TARGET) -o $(TARGET)
	@echo "[STRIP] $@"
	@$(STRIP) $(TARGET)

-include $(CXXDEPS)

obj/%.cpp.d: %.cpp
	@if [ ! -e obj ]; then mkdir obj; fi
	@echo "[DEP]   $<"
	@$(CXX) -M $(CXXOPTS) -D__CPLUSPLUS $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

obj/%.s.o: %.s
	@if [ ! -e obj ]; then mkdir obj; fi
	@echo "[AS]    $<"
	@$(AS) $(ASOPTS) -o $@ $<

obj/%.cpp.o: %.cpp
	@if [ ! -e obj ]; then mkdir obj; fi
	@echo "[CXX]   $<"
	@$(CXX) $(CXXOPTS) -D__CPLUSPLUS -c -o $@ $<

clean:
	@-rm $(TARGET) $(OBJS) $(CXXDEPS)

# vim: filetype=make:
