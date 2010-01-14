// This file is part of Akari.
// Copyright 2010 Arlen Cuss
//
// Akari is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Akari is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Akari.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __ELF_HPP__
#define __ELF_HPP__

#include <arch.hpp>

#define EI_MAG0		0		// 0x7f
#define EI_MAG1		1		// 'E'
#define EI_MAG2		2		// 'L'
#define EI_MAG3		3		// 'F'
#define EI_CLASS	4		// ELFCLASS*; 	file class
#define EI_DATA		5		// ELFDATA*;	data encoding
#define EI_VERSION	6		// EV_*;	file version
#define EI_OSABI	7		// ELFOSABI*;	OS/ABI ident
#define EI_ABIVERSION	8		// ABI version (0 if none?)
#define EI_PAD		9		// start of padding bytes (0 or left alone for compatibility with future)
#define EI_NIDENT 	16

typedef enum {
	ELFCLASSNONE	= 0,
	ELFCLASS32	= 1,
	ELFCLASS64	= 2
} ElfClass;

typedef enum {
	ELFDATANONE	= 0,
	ELFDATA2LSB	= 1,	// little endian
	ELFDATA2MSB	= 2
} ElfDataEncoding;

typedef enum {
	ELFOSABI_NONE	= 0
} ElfOSABI;

typedef unsigned long Elf32_Addr;
typedef unsigned long Elf32_Off;
typedef unsigned short Elf32_Half;
typedef unsigned long Elf32_Word;
typedef signed long Elf32_Sword ;
// To be defined: Elf64_Ehdr, typedefs for Elf64

typedef struct {
	unsigned char	e_ident[EI_NIDENT];	// mark as object file, machine-independent data
	Elf32_Half	e_type;			// see ET_ constants
	Elf32_Half	e_machine;		// EM_
	Elf32_Word	e_version;		// EV_
	Elf32_Addr	e_entry;		// the virtual address to which the system first transfers control, starting the process. (entry point) (0 if none)
	Elf32_Off	e_phoff;		// program header table's file offset in bytes (0 if none)
	Elf32_Off	e_shoff;		// section header table's file offset in bytes (0 if none)
	Elf32_Word	e_flags;		// processor-specific flags. EF_*
	Elf32_Half	e_ehsize;		// ELF header's size in bytes
	Elf32_Half	e_phentsize;		// size in bytes of one entry in file's program header table (all entries are same size)
	Elf32_Half	e_phnum;		// number of entries in program header table (0 of no program header)
	Elf32_Half	e_shentsize;		// section header's size in bytes
	Elf32_Half	e_shnum;		// number of section header table entries
	Elf32_Half	e_shstrndx;		// the section header table index of the entry associated with the section name string table (if none, SHN_UNDEF) more complicated rules follow.
} Elf32_Ehdr;

typedef enum {
	ET_NONE		= 0,
	ET_REL		= 1,
	ET_EXEC		= 2,
	ET_DYN		= 3,
	ET_CORE		= 4,
	ET_LOOS		= 0xfe00,
	ET_HIOS		= 0xfeff,
	ET_LOPROC	= 0xff00,
	ET_HIPROC	= 0xffff
} ElfType;

typedef enum {
	EM_NONE		= 0,
	EM_386		= 3
} ElfMachine;

typedef enum {
	EV_NONE		= 0,
	EV_CURRENT	= 1
} ElfVersion;

typedef enum {
	SHN_UNDEF	= 0,
	SHN_LORESERVE	= 0xff00,
	SHN_LOPROC	= 0xff00,
	SHN_HIPROC	= 0xff1f,
	SHN_LOOS	= 0xff20,
	SHN_HIOS	= 0xff3f,
	SHN_ABS		= 0xfff1,
	SHN_COMMON	= 0xfff2,
	SHN_XINDEX	= 0xffff,
	SHN_HIRESERVE	= 0xffff
} ElfSectionHeaderIndex;

typedef struct {
	Elf32_Word	sh_name;	// name of the section; an index into the section header string table section
	Elf32_Word	sh_type;	// SHT_*; categorises section's contents & semantics
	Elf32_Word	sh_flags;	// SHF_*; supports 1-bit flags that describe misc. things
	Elf32_Addr	sh_addr;	// if the section will appear in the memory image of a process, this is the address where the first address should be. otherwise 0.
	Elf32_Off	sh_offset;	// the byte offset (from the beginning of the file) to the first byte in the section. type=SH_NOBITS has no space, and this field is thus conceptual in that case
	Elf32_Word	sh_size;	// the length in bytes. type=SH_NOBITS may have non-zero size, but occupies no space in the file
	Elf32_Word	sh_link;	// a section header table index link, interpretation depends on the section type
	Elf32_Word	sh_info;	// extra info.
	Elf32_Word	sh_addralign;	// alignment constraints
	Elf32_Word	sh_entsize;	// fixed-size entries, e.g. symbol table. this is size in bytes of each entry. 0 if no such table exists
} Elf32_Shdr;

typedef enum {
	SHT_NULL	= 0,		// inactive
	SHT_PROGBITS	= 1,		// holds info defined by the program, program-dependent
	SHT_SYMTAB	= 2,		// holds a symbol table. SYMTAB provides symbols for link editing. DYNSYM for dynamic linking symbols. typically 1 of each max.
	SHT_STRTAB	= 3,		// string table - may be many.
	SHT_RELA	= 4,		// relocation entries with explicit addends
	SHT_HASH	= 5,		// symbol hash table. typically max 1.
	SHT_DYNAMIC	= 6,		// info for dynamic linking. typically max 1.
	SHT_NOTE	= 7,
	SHT_NOBITS	= 8,		// occupies no space in FILE, otherwise resembles PROGBITS
	SHT_REL		= 9,		// relocation entries w/o explicit addendss
	SHT_SHLIB	= 10,		// reserved, unspecified semantics
	SHT_DYNSYM	= 11,		// see SYMTAB
	SHT_INIT_ARRAY	= 14,		// array of pointers to initialisation fns. each ptr is void return, void arguments
	SHT_FINI_ARRAY	= 15,		// array of pointers to termination fns. void, void.
	SHT_PREINIT_ARRAY = 16,		// array of pointers to fns invoked before all other init fns. void, void.
	SHT_GROUP	= 17,		// a section group. a set of sections that are related and treated specially by linker. only in relocatable options (ET_REL). must appear in SHT before entries for any sections that are members of the group
	SHT_SYMTAB_SHNDX = 18,		// associated with SHT_SYMTAB, required if any of section header indexes referenced by that symbol table contain SHN_XINDEX. array of Elf32_Word values. each value corresponds 1:1 with a symbol table entry & appear in the same order as those entries. the values represent the section header idxs against which the symbol table ents are defined
	SHT_LOOS	= 0x60000000,	// os-specific
	SHT_HIOS	= 0x6fffffff,	// " "
	SHT_LOPROC	= 0x70000000,	// processor-specific
	SHT_HIPROC	= 0x7fffffff,	// " "
	SHT_LOUSER	= 0x80000000,	// reserved for apps
	SHT_HIUSER	= 0xffffffff,	// " " "
} ElfSectionHeaderType;

// Index 0 is always empty. 0, SHT_NULL, 0, 0, 0, unspecified, unspecified, 0, 0, 0. It is mentioned in the table.

typedef enum {
	SHF_WRITE		= 0x1,		// data that should be writable during process exec
	SHF_ALLOC		= 0x2,		// this section occupies memory during execution. (some do not reside in memory image - this bit is off for those)
	SHF_EXECINSTR		= 0x4,		// executable machine instructions
	SHF_MERGE		= 0x10,		// data may be merged to eliminate duplication. see docs.
	SHF_STRINGS		= 0x20,		// consists of null-terminated char strings. size of each character is in sh_entsize
	SHF_INFO_LINK		= 0x40,		// sh_info of this section header contains a sectino header table index
	SHF_LINK_ORDER		= 0x80,		// adds special ordering requirements for link editors. see docs
	SHF_OS_NONCONFORMING	= 0x100,		// section requires special OS-specific processing
	SHF_GROUP		= 0x200,		// is a member of a section group, must be referenced by section of SHT_GROUP. only set for sections in relocatable objects (ET_REL)
	SHF_TLS			= 0x400,		// holds thread-local storage; each thread has its own distinct instance of this data. implementations don't have to support this
	SHF_MASKOS		= 0x0ff00000,	// all bits in this mask reserved for os-specific
	SHF_MASKPROC		= 0xf0000000	// " " " " " " " processor-specific
} ElfSectionHeaderFlag;

#define GRP_COMDAT		0x1
#define GRP_MASKOS		0x0ff00000
#define GRP_MASKPROC		0xf0000000

// symbol table entry
typedef struct {
	Elf32_Word	st_name;		// index into object file's symbol string table; !0 -> index which gives symbol name, 0 ->  no name.
	Elf32_Addr	st_value;		// value of associated symbol. see below
	Elf32_Word	st_size;
	unsigned char 	st_info;		// type & binding attributes. see macros below to extract
	unsigned char	st_other;		// a symbol's visibility
	Elf32_Half	st_shndx;		// section header table index.
} Elf32_Sym;	// TODO 64-bit?

#define ELF32_ST_BIND(i)	((i)>>4)
#define ELF32_ST_TYPE(i)	((i)&0xf)
#define ELF32_ST_INFO(b,t)	(((b)<<4)+((t)&0xf))
// 64-bit ones?

#define ELF32_ST_VISIBILITY(o)	((o)&0x3)
// 64-bit?

// ST_BIND()
#define STB_LOCAL	0	// not visible outside object file containing their definition. local symbols w/ same name may exist in multiple files w/o conflict
#define STB_GLOBAL	1	// visible to all object files being combined. satisfies another file's undefined reference to this global symbol
#define STB_WEAK	2	// resemble globals, but have lower precedence. multiple weaks+1 global < global used.
#define STB_LOOS	10	
#define STB_HIOS	12
#define STB_LOPROC	13
#define STB_HIPROC	15

// ST_TYPE()
#define STT_NOTYPE	0
#define STT_OBJECT	1	// data object
#define STT_FUNC	2	// function/other execable code
#define STT_SECTION	3	// a section.
#define STT_FILE	4	// name of source file associated with object file
#define STT_COMMON	5	// uninit'd common block
#define STT_TLS		6	// thread-local storage. no need to implement.
#define STT_LOOS	10
#define STT_HIOS	12
#define STT_LOPROC	13
#define STT_HIPROC	15

// ST_VISIBILITY()
#define STV_DEFAULT	0	// as per binding type
#define STV_INTERNAL	1	// may be defined by processor supplements
#define STV_HIDDEN	2	// hidden if its name is not visible to other components. protected. control external interface. may be referred if addr is passed out
#define STV_PROTECTED	3	// ? see docs

// symbol table entry idx 0: 0, 0, 0, 0, 0, SHN_UNDEF.

// st_value interpretations:
// relocatable files: st_value holds alignment constraints for a symbol whose section index is SHN_COMMON
// relocatable files: st_value holds a section offset for a defined symbol. st_value is an offset from the beginning of the section that st_shndx identifies
// executable and shared objects: st_value holds a virtual address. to make these files' symbols more useful for the dynamic linker, the section offset (file interpretation) gives way to a virtual address (memory interpretation) for which the section number is irrelevant

typedef struct {
	Elf32_Addr	r_offset;	// gives the location at which to apply the relocation action. for a relocatable file, the value is the byte offset from beginning of the section to the storage unit affected by the relocation. for an executable/shared object, the value is the virtual address of the storage unit affected by the relocation
	Elf32_Word	r_info;		// member gives both symbol tbl index with respect to which the relocation must be made, and the type fo relocation to apply.
} Elf32_Rel;

typedef struct {
	Elf32_Addr	r_offset;	// ^
	Elf32_Word	r_info;		// ^
	Elf32_Sword	r_addend;	// specifies a constant addend used to compute the value to be stored into the relocatable field
} Elf32_Rela;

#define ELF32_R_SYM(i)		((i)>>8)
#define ELF32_R_TYPE(i)		((unsigned char)(i))
#define ELF32_R_INFO(s,t)	(((s)<<8)+(unsigned char)(t))

// relocatables: r_offset holds a section offset. the relocation section itself describes how to modify another section in the file; relocation offsets designate a storage unit within the 2nd section
// exe/so: r_offset holds a virtual address. see docs for more if useful.

// NOTE XXX THE BELOW IS NOT ENGLISH.
// If multiple consecutive (i.e. contiguous within single relocation section) relocation records are applied to the same relocation location (r_offset), compose:
// * in all but last relocation operation of a composed sequence, the result of the relocation expression is retained, rather than having part extracted + placed in the relocated full.
//     the result is retained at full ptr precision of the applicable ABI processor supplement.
// * in all but the first relocation op of a composed seq, the addend used is the retained result of the previous relocation op, rather than that implied by the reloc type

// note that the consequence of the above rules is that the location specified is ignored in all but the first and last. see docs.

typedef struct {
	Elf32_Word	p_type;		// PT_*
	Elf32_Off	p_offset;	// offset from beginning of file where first byte of segment resides
	Elf32_Addr	p_vaddr;	// virtual addr at which first bit of segment resides in memory
	Elf32_Addr	p_paddr;	// where physical addressing is relevant, this is reserved for the segment's physical addr. [how does that work?]
	Elf32_Word	p_filesz;	// number of bytes in the file image of the segment; may be zero.
	Elf32_Word	p_memsz;	// number of bytes in the memory image of the segment; may be zero.
	Elf32_Word	p_flags;	// PF_*
	Elf32_Word	p_align;	// p_vaddr and p_offset must be congruent, modulo the page size. this gives the value to which segments are aligned; 0/1 mean nothing required. otherwise positive integral power of 2, p_vaddr == p_offset % p_align
} Elf32_Phdr;

typedef enum {
	PT_NULL		= 0,			// unused
	PT_LOAD		= 1,			// array element specifies a loadable segment, filesz and memsz are relevant. map to beginning of memory segment. if memsz>filesz, extra bytes following segments initialised area hold 0. filesz>memsz is an error. loadable segment entries in pht appear in ascending order, sorted by vaddr
	PT_DYNAMIC	= 2,			// dynamic linking info.
	PT_INTERP	= 3,			// specifies the location and size of a null-terminated pathname to invoke as an interpreter.
	PT_NOTE		= 4,			// aux
	PT_SHLIB	= 5,			// reserved, unspecified semantics.
	PT_PHDR		= 6,			// specifies location and size of pht itself, both in file and in memory image of program. may occur not more than once. may only occur if pht is part of the memory image of the program. if present, must precede any loadable segment entry
	PT_TLS		= 7,			// TLS, unnecessary
	PT_LOOS		= 0x60000000,
	PT_HIOS		= 0x6fffffff,
	PT_LOPROC	= 0x70000000,
	PT_HIPROC	= 0x7fffffff,
} ElfProgramHeaderType;

typedef enum {
	PF_X		= 0x1,		// execute
	PF_W		= 0x2,		// write
	PF_R		= 0x4,		// read
	PF_MASKOS	= 0x0ff00000,	// unspec
	PF_MASKPROC	= 0xf0000000	// unspec
} ElfProgramHeaderFlag;

#endif

