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

TARGET = ../$(LINKAGE_TARGET)

ASMSRCS := $(wildcard *.s)
CXXSRCS := $(wildcard *.cpp)
CXXDEPS := $(patsubst %.cpp,obj/%.cpp.d,$(CXXSRCS))
OBJS := $(patsubst %.s,obj/%.s.o,$(ASMSRCS)) $(patsubst %.cpp,obj/%.cpp.o,$(CXXSRCS))
LINKAGE_CXXSRCS := $(wildcard ../User*.cpp)
LINKAGE_CXXDEPS := $(patsubst ../User%.cpp,obj/User%.cpp.d,$(LINKAGE_CXXDEPS))
LINKAGE_OBJS := $(patsubst ../User%.cpp,obj/User%.cpp.o,$(LINKAGE_CXXSRCS))

COPTS += -I../inc
CXXOPTS += -I../inc

all: $(TARGET)

$(TARGET): $(OBJS) $(LINKAGE_OBJS)
	@echo "[AR]  $@"
	@$(AR) rcs $@ $(OBJS) $(LINKAGE_OBJS)

-include $(CXXDEPS)
-include $(LINKAGE_CXXDEPS)

obj/User%.cpp.d: ../User%.cpp
	@echo "[DEP] $<"
	@$(CXX) -M $(CXXOPTS) -D__AKARI_KERNEL -D__CPLUSPLUS $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

obj/%.cpp.d: %.cpp
	@echo "[DEP] $<"
	@$(CXX) -M $(CXXOPTS) -D__AKARI_KERNEL -D__CPLUSPLUS $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

obj/User%.cpp.o: ../User%.cpp
	@echo "[CXX] $<"
	@$(CXX) $(CXXOPTS) -D__AKARI_LINKAGE -D__CPLUSPLUS -c -o $@ $<

obj/%.s.o: %.s
	@echo "[AS]  $<"
	@$(AS) $(ASOPTS) -o $@ $<

obj/%.cpp.o: %.cpp
	@echo "[CXX] $<"
	@$(CXX) $(CXXOPTS) -D__CPLUSPLUS -c -o $@ $<

clean:
	@-rm $(TARGET) $(OBJS) $(LINKAGE_OBJS) $(LINKAGE_CXXDEPS) $(CXXDEPS)
