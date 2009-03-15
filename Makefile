TARGET = Akari

LDFILE = akari.lnk
COPYDEST = c:/akari
ASOPTS = -gstabs --32
COPTS = -Wall -Iinc -fleading-underscore -fno-builtin -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -ggdb3 -fno-stack-protector -m32
CXXOPTS = -Wall -Iinc -fleading-underscore -fno-builtin -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -ggdb3 -fno-stack-protector -m32 -nostartfiles -nostdlib -fno-rtti -fno-exceptions
LDOPTS = -melf_i386

ASMSRCS := $(wildcard *.s)
CSRCS := $(wildcard *.c)
CXXSRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.s,obj/%.s.o,$(ASMSRCS)) $(patsubst %.c,obj/%.c.o,$(CSRCS)) $(patsubst %.cpp,obj/%.cpp.o,$(CXXSRCS))

all: $(TARGET)-copy

$(TARGET)-copy: $(TARGET)
	$(MTOOLS_BIN)/mcopy -D o $(TARGET) $(COPYDEST)

$(TARGET): $(OBJS) $(LDFILE)
	$(LD) $(LDOPTS) -T$(LDFILE) $(OBJS) -o $(TARGET)

obj/%.s.o: %.s
	$(AS) $(ASOPTS) -o $@ $<
obj/%.c.o: %.c
	$(CC) $(COPTS) -c -o $@ $<
obj/%.cpp.o: %.cpp
	$(CXX) -D__CPLUSPLUS $(CXXOPTS) -c -o $@ $<

clean:
	@-rm $(TARGET) $(OBJS)

