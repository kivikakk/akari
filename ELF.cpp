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

	Akari->console->putString("type: ");
	switch (hdr->e_type) {
		case ET_NONE:	Akari->console->putString("none\n"); break;
		case ET_REL:	Akari->console->putString("relocatable\n"); break;
		case ET_EXEC:	Akari->console->putString("executable\n"); break;
		case ET_DYN:	Akari->console->putString("shared object\n"); break;
		case ET_CORE:	Akari->console->putString("core\n"); break;
	}

	return true;
}

#if 0
/*
	*/

/*
	puts("\nenumerating program header(s):\n");
	int i; for (i = 0; i < hdr->e_phnum; ++i) {
		Elf32_Phdr *ph = (Elf32_Phdr *)(file_buffer + hdr->e_phoff + hdr->e_phentsize * i);
		switch (ph->p_type) {
			case PT_NULL:		puts("\tnull\n"); break;
			case PT_LOAD:		puts("\tloadable\n"); break;
			case PT_DYNAMIC:	puts("\tdynamic linkage\n"); break;
			default:		puts("\tother("); puthexlong(ph->p_type); puts(")\n"); break;
		}
		puts("\t\tvaddr/paddr: "); puthexlong(ph->p_vaddr); puts("/"); puthexlong(ph->p_paddr); locatenl();
		puts("\t\tfilesz/memsz: "); puthexlong(ph->p_filesz); puts("/"); puthexlong(ph->p_memsz); locatenl();
		puts("\t\toffset/align: "); puthexlong(ph->p_offset); puts("/"); puthexlong(ph->p_align); locatenl();

	}
	*/

	// unsigned long stloc = ((Elf32_Shdr *)(file_buffer + hdr->e_shoff + hdr->e_shentsize * hdr->e_shstrndx))->sh_offset;
	
	Map<unsigned long, unsigned long> program_space;

	// puts("enumerating section header(s):\n");
	for (int i = 0; i < hdr->e_shnum; ++i) {
		Elf32_Shdr *sh = (Elf32_Shdr *)(file_buffer + hdr->e_shoff + hdr->e_shentsize * i);
		// puts("\t"); puts((char *)(file_buffer + stloc + sh->sh_name)); puts(" [");

/*
		switch (sh->sh_type) {
			case SHT_NULL:		puts("null"); break;
			case SHT_PROGBITS:	puts("progbits"); break;
			case SHT_SYMTAB:	puts("symbol table"); break;
			case SHT_STRTAB:	puts("string table"); break;
			default:		puts("other("); puthexlong(sh->sh_type); puts(")"); break;
		}
		puts("] at "); puthexlong(sh->sh_offset);
		puts(" -> "); puthexlong(sh->sh_addr);
		puts("; ");
		*/

		if (sh->sh_addr) {
		//	puts("placing in image");

			unsigned long phys = sh->sh_offset, virt = sh->sh_addr, copied = 0;
			while (copied < sh->sh_size) {
				// we copy up until the end of one frame
				unsigned long copy = 0x1000 - (virt % 0x1000);
				puts("locating "); puthexlong((virt + copied) & 0xfffff000); puts(" ... ");
				Page *page = new_task->page_directory->get_page((virt + copied) & 0xfffff000, true);

				unsigned long virt_frame;
				if (page->page_address) {
					// retrieve old virt frame
					puts("searching "); puthexlong(page->page_address * 0x1000); locatenl();
					virt_frame = program_space.get(page->page_address * 0x1000);
				} else {
					unsigned long phys_frame;
					virt_frame = (unsigned long)kmalloc_ap(0x1000, &phys_frame);
					page->direct_frame(phys_frame, false, true);	// make sure we put the phys one in here; TODO permissions
					program_space.set(phys_frame, virt_frame);
					puts("storing "); puthexlong(phys_frame); locatenl();
				}

				unsigned char *dest = (unsigned char *)(virt_frame + ((virt + copied) % 0x1000));
				memcpy(dest, (unsigned char *)(file_buffer + phys + copied), copy);

				copied += copy;
			}
		} else {
			// puts("ignored");
		}

		// locatenl();
	}

	kfree(file_buffer);

	add_task_to_scheduler(new_task, true);
}

#endif
