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
// along with Akari.  If not, see <http://www.gnu.org/licenses/>

#include <ELF.hpp>
#include <ELFInternal.hpp>
#include <debug.hpp>
#include <HashTable.hpp>

#define ELF_DEBUG false

ELF::ELF()
{ }

u8 ELF::versionMajor() const { return 0; }
u8 ELF::versionMinor() const { return 1; }
const char *ELF::versionManufacturer() const { return "Akari"; }
const char *ELF::versionProduct() const { return "Akari ELF Loader"; }

bool ELF::loadImageInto(Tasks::Task *task, const u8 *image) const {
	const Elf32_Ehdr *hdr = reinterpret_cast<const Elf32_Ehdr *>(image);

	Akari->console->putString("image is at 0x");
	Akari->console->putInt(reinterpret_cast<u32>(image), 16);
	Akari->console->putString("\n");

	if (hdr->e_ident[EI_MAG0] != 0x7f) return false;
	if (hdr->e_ident[EI_MAG1] != 'E') return false;
	if (hdr->e_ident[EI_MAG2] != 'L') return false;
	if (hdr->e_ident[EI_MAG3] != 'F') return false;

	// Change the EIP in the registers to our entry point.
	reinterpret_cast<struct modeswitch_registers *>(task->ks)->callback.eip = hdr->e_entry;

#if ELF_DEBUG
	Akari->console->putString("type: ");
	switch (hdr->e_type) {
		case ET_NONE:	Akari->console->putString("none\n"); break;
		case ET_REL:	Akari->console->putString("relocatable\n"); break;
		case ET_EXEC:	Akari->console->putString("executable\n"); break;
		case ET_DYN:	Akari->console->putString("shared object\n"); break;
		case ET_CORE:	Akari->console->putString("core\n"); break;
	}

	Akari->console->putString("\nenumerating program header(s):\n");
	int i; for (i = 0; i < hdr->e_phnum; ++i) {
		Elf32_Phdr *ph = (Elf32_Phdr *)(image + hdr->e_phoff + hdr->e_phentsize * i);
		switch (ph->p_type) {
			case PT_NULL:		Akari->console->putString("\tnull\n"); break;
			case PT_LOAD:		Akari->console->putString("\tloadable\n"); break;
			case PT_DYNAMIC:	Akari->console->putString("\tdynamic linkage\n"); break;
			default:		Akari->console->putString("\tother("); Akari->console->putInt(ph->p_type, 16); Akari->console->putString(")\n"); break;
		}
		Akari->console->putString("\t\tvaddr/paddr: "); Akari->console->putInt(ph->p_vaddr, 16); Akari->console->putString("/"); Akari->console->putInt(ph->p_paddr, 16); Akari->console->putChar('\n');
		Akari->console->putString("\t\tfilesz/memsz: "); Akari->console->putInt(ph->p_filesz, 16); Akari->console->putString("/"); Akari->console->putInt(ph->p_memsz, 16); Akari->console->putChar('\n');
		Akari->console->putString("\t\toffset/align: "); Akari->console->putInt(ph->p_offset, 16); Akari->console->putString("/"); Akari->console->putInt(ph->p_align, 16); Akari->console->putChar('\n');

	}

	unsigned long stloc = ((Elf32_Shdr *)(image + hdr->e_shoff + hdr->e_shentsize * hdr->e_shstrndx))->sh_offset;
#endif
	
	
	// We're going to be placing stuff in the target program.  We allocate one aligned page,
	// then grab its page so we can point that part of our own perception of memory to wherever
	// we want.  We set it back to real heap-land after then dealloc.
	
	u8 *magic_wand = static_cast<u8 *>(Akari->memory->allocAligned(0x1000));
	Memory::Page *wand_page = Akari->memory->_kernelDirectory->getPage(reinterpret_cast<u32>(magic_wand), false);
	ASSERT(wand_page);

	// We can now twiddle wand_page->pageAddress to change where 'magic_wand' looks at.
	u32 old_wand_pA = wand_page->pageAddress;

#if ELF_DEBUG
	Akari->console->putString("enumerating section header(s):\n");
#endif
	for (int i = 0; i < hdr->e_shnum; ++i) {
		Elf32_Shdr *sh = (Elf32_Shdr *)(image + hdr->e_shoff + hdr->e_shentsize * i);

#if ELF_DEBUG
		Akari->console->putString("\t"); Akari->console->putString((char *)(image + stloc + sh->sh_name)); Akari->console->putString(" [");

		switch (sh->sh_type) {
			case SHT_NULL:		Akari->console->putString("null"); break;
			case SHT_PROGBITS:	Akari->console->putString("progbits"); break;
			case SHT_SYMTAB:	Akari->console->putString("symbol table"); break;
			case SHT_STRTAB:	Akari->console->putString("string table"); break;
			default:		Akari->console->putString("other("); Akari->console->putInt(sh->sh_type, 16); Akari->console->putString(")"); break;
		}
		Akari->console->putString("] at "); Akari->console->putInt(sh->sh_offset, 16);
		Akari->console->putString(" -> "); Akari->console->putInt(sh->sh_addr, 16);
		Akari->console->putString("; ");
#endif

		if (sh->sh_addr) {
#if ELF_DEBUG
			Akari->console->putString("placing in image:\n\t\t");
#endif

			unsigned long phys = sh->sh_offset, virt = sh->sh_addr, copied = 0;
			while (copied < sh->sh_size) {
				// we copy up until the end of one frame
				unsigned long copy = 0x1000 - ((virt + copied) % 0x1000);

#if ELF_DEBUG
				Akari->console->putString("virt 0x");
				Akari->console->putInt((virt + copied), 16);
#endif

				Memory::Page *page = task->pageDir->getPage((virt + copied) & 0xfffff000, true);

				if (!page->pageAddress)
					page->allocAnyFrame(false, true);

				wand_page->pageAddress = page->pageAddress;

#if ELF_DEBUG
				Akari->console->putString(" len 0x");
				Akari->console->putInt(copy, 16);
				Akari->console->putString(" phys 0x");
				Akari->console->putInt(page->pageAddress * 0x1000, 16);
				Akari->console->putString(" off 0x");
				Akari->console->putInt((virt + copied) % 0x1000, 16);
				Akari->console->putString(", ");
#endif

				u8 *dest = magic_wand + ((virt + copied) % 0x1000);
				POSIX::memcpy(dest, (u8 *)(image + phys + copied), copy);

				copied += copy;
			}
		} else {
#if ELF_DEBUG
			Akari->console->putString("ignored");
#endif
		}
#if ELF_DEBUG
		Akari->console->putChar('\n');
#endif
	}

	// Cleaning up our hole in memory.
	wand_page->pageAddress = old_wand_pA;
	Akari->memory->free(magic_wand);

	return true;
}

