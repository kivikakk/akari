
LINKAGE_TARGET = AkariLib.a
ASOPTS = -gstabs --32
COPTS = -Wall -Iinc -fleading-underscore -fno-builtin -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -ggdb3 -fno-stack-protector -m32 -DDEBUG
CXXOPTS = -Wall -Iinc -fleading-underscore -fno-builtin -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -ggdb3 -fno-stack-protector -m32 -nostartfiles -nostdlib -fno-rtti -fno-exceptions -DDEBUG
LDOPTS = -melf_i386

