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
	$(CXX) $(KERNEL_DEFINE) -D__CPLUSPLUS $(CXXOPTS) -c -o $@ $<

obj:
	if [ ! -e obj ]; then mkdir obj; fi

clean:
	@-rm $(TARGET) $(OBJS)

