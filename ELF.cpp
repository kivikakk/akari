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

#if ELF_DEBUG
	Akari->console->putString("image is at 0x");
	Akari->console->putInt(reinterpret_cast<u32>(image), 16);
	Akari->console->putString(", entry point apparently 0x");
	Akari->console->putInt(hdr->e_entry, 16);
	Akari->console->putString("\n");
#endif

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
#endif
	
	
	// We're going to be placing stuff in the target program.  We allocate one aligned page,
	// then grab its page so we can point that part of our own perception of memory to wherever
	// we want.  We set it back to real heap-land after then dealloc.
	
	// XXX unforunately, as a side effect, we're reloading the page directory every time we move
	// the 'wand', which is slow.  We need a better way to do this.

	u8 *magic_wand = static_cast<u8 *>(Akari->memory->allocAligned(0x1000));
	Memory::Page *wand_page = Akari->memory->_activeDirectory->getPage(reinterpret_cast<u32>(magic_wand), false);
	ASSERT(wand_page);

#if ELF_DEBUG
	Akari->console->putString("wppA was ");
	Akari->console->putInt(wand_page->pageAddress, 16);
	Akari->console->putString(", wp at ");
	Akari->console->putInt((u32)wand_page, 16);
	Akari->console->putString("\n");
#endif

	// We can now twiddle wand_page->pageAddress to change where 'magic_wand' looks at.
	u32 old_wand_pA = wand_page->pageAddress;

	for (int i = 0; i < hdr->e_phnum; ++i) {
		Elf32_Phdr *ph = (Elf32_Phdr *)(image + hdr->e_phoff + hdr->e_phentsize * i);

		if (ph->p_type == PT_LOAD) {
#if ELF_DEBUG
			Akari->console->putString("header at "); Akari->console->putInt(ph->p_offset, 16);
			Akari->console->putString(" mapped to v/p "); Akari->console->putInt(ph->p_vaddr, 16);
			Akari->console->putString("/"); Akari->console->putInt(ph->p_paddr, 16);
			Akari->console->putString(" f/msz "); Akari->console->putInt(ph->p_filesz, 16);
			Akari->console->putString("/"); Akari->console->putInt(ph->p_memsz, 16);
			Akari->console->putString(" align "); Akari->console->putInt(ph->p_align, 16);
			Akari->console->putChar('\n');
#endif

			unsigned long phys = ph->p_offset, virt = ph->p_vaddr, copied = 0;
			while (copied < ph->p_memsz) {
				// we copy up until the end of one frame
				unsigned long copy = 0x1000 - ((virt + copied) % 0x1000);
				if (ph->p_memsz - copied < copy)
					copy = ph->p_memsz - copied;

#if ELF_DEBUG
				Akari->console->putString("\tvirt 0x");
				Akari->console->putInt((virt + copied), 16);
#endif

				Memory::Page *page = task->pageDir->getPage((virt + copied) & 0xfffff000, true);
				ASSERT(page);

				if (!page->pageAddress)
					page->allocAnyFrame(false, true);


#if ELF_DEBUG
				Akari->console->putString(" (wand ");
				Akari->console->putInt(wand_page->pageAddress, 16);
				Akari->console->putString(" ");
				Akari->console->putInt(*((u8 *)magic_wand), 16);
#endif
				wand_page->pageAddress = page->pageAddress;
				__asm__ __volatile__("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "%eax");
#if ELF_DEBUG
				Akari->console->putString("->");
				Akari->console->putInt(*((u8 *)magic_wand), 16);
				Akari->console->putString(" ");
				Akari->console->putInt(wand_page->pageAddress, 16);
				Akari->console->putString(")");

				Akari->console->putString(" len 0x");
				Akari->console->putInt(copy, 16);
				Akari->console->putString(" phys 0x");
				Akari->console->putInt(page->pageAddress * 0x1000, 16);
				Akari->console->putString(" off 0x");
				Akari->console->putInt((virt + copied) % 0x1000, 16);
				Akari->console->putString("\n");
#endif

				u8 *dest = magic_wand + ((virt + copied) % 0x1000);

				if (ph->p_filesz == 0) {
#if ELF_DEBUG
					Akari->console->putString("\nblanking ");
					Akari->console->putInt(copy, 16);
					Akari->console->putString("\n");
#endif
					POSIX::memset(dest, 0, copy);
				} else if (ph->p_filesz < ph->p_memsz) {
					s32 file_left = static_cast<s32>(ph->p_filesz) - copied;
					if (static_cast<s32>(copy) > file_left) {
						if (file_left < 0) file_left = 0;
						if (file_left) {
							POSIX::memcpy(dest, (u8 *)(image + phys + copied), file_left);
						}
						POSIX::memset(dest + file_left, 0, copy - file_left);
					} else {
						POSIX::memcpy(dest, (u8 *)(image + phys + copied), copy);
					}
				} else if (ph->p_filesz == ph->p_memsz) {
					POSIX::memcpy(dest, (u8 *)(image + phys + copied), copy);
				} else {
					AkariPanic("ph->p_filesz is greater than ph->p_memsz!");
				}

				copied += copy;
			}

#if ELF_DEBUG
			Akari->console->putChar('\n');
#endif
		} else {
			// Ignore it.
		}
	}

	// Cleaning up our hole in memory.
	wand_page->pageAddress = old_wand_pA;
	__asm__ __volatile__("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "%eax");
	Akari->memory->free(magic_wand);

	return true;
}

