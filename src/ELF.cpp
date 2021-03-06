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
#include <Akari.hpp>
#include <Console.hpp>

#undef ELF_DEBUG

ELF::ELF()
{ }

bool ELF::loadImageInto(Tasks::Task *task, const u8 *image) const {
	const Elf32_Ehdr *hdr = reinterpret_cast<const Elf32_Ehdr *>(image);

#ifdef ELF_DEBUG
	mu_console->printf("image is at 0x%x, ", reinterpret_cast<u32>(image));
	mu_console->printf("entry point apparently 0x%x\n", hdr->e_entry);
#endif

	if (hdr->e_ident[EI_MAG0] != 0x7f) return false;
	if (hdr->e_ident[EI_MAG1] != 'E') return false;
	if (hdr->e_ident[EI_MAG2] != 'L') return false;
	if (hdr->e_ident[EI_MAG3] != 'F') return false;

	// Change the EIP in the registers to our entry point.
	reinterpret_cast<struct modeswitch_registers *>(task->ks)->callback.eip = hdr->e_entry;

#ifdef ELF_DEBUG
	mu_console->printf("type: ");
	switch (hdr->e_type) {
		case ET_NONE:	mu_console->printf("none\n"); break;
		case ET_REL:	mu_console->printf("relocatable\n"); break;
		case ET_EXEC:	mu_console->printf("executable\n"); break;
		case ET_DYN:	mu_console->printf("shared object\n"); break;
		case ET_CORE:	mu_console->printf("core\n"); break;
	}
#endif
	
	
	// We're going to be placing stuff in the target program.  We allocate one aligned page,
	// then grab its page so we can point that part of our own perception of memory to wherever
	// we want.  We set it back to real heap-land after then dealloc.
	
	// XXX unforunately, as a side effect, we're reloading the page directory every time we move
	// the 'wand', which is slow.  We need a better way to do this.

	u8 *magic_wand = static_cast<u8 *>(mu_memory->allocAligned(0x1000));
	Memory::Page *wand_page = mu_memory->_activeDirectory->getPage(reinterpret_cast<u32>(magic_wand), false);
	ASSERT(wand_page);

#ifdef ELF_DEBUG
	mu_console->printf("wppA was %x, wp at %x\n", wand_page->pageAddress, (u32)wand_page);
#endif

	// We can now twiddle wand_page->pageAddress to change where 'magic_wand' looks at.
	u32 old_wand_pA = wand_page->pageAddress;

	for (int i = 0; i < hdr->e_phnum; ++i) {
		Elf32_Phdr *ph = (Elf32_Phdr *)(image + hdr->e_phoff + hdr->e_phentsize * i);

		if (ph->p_type == PT_LOAD) {
#ifdef ELF_DEBUG
			mu_console->printf("header at %x mapped to v/p %x/%x f/msz %x/%x align %x\n",
				ph->p_offset, ph->p_vaddr, ph->p_paddr, ph->p_filesz, ph->p_memsz, ph->p_align);
#endif

			unsigned long phys = ph->p_offset, virt = ph->p_vaddr, copied = 0;
			while (copied < ph->p_memsz) {
				// we copy up until the end of one frame
				unsigned long copy = 0x1000 - ((virt + copied) % 0x1000);
				if (ph->p_memsz - copied < copy)
					copy = ph->p_memsz - copied;

#ifdef ELF_DEBUG
				mu_console->printf("\tvirt 0x%x", virt + copied);
#endif

				Memory::Page *page = task->pageDir->getPage((virt + copied) & ~0xfff, true);
				ASSERT(page);

				page->allocAnyFrame(false, true);


#ifdef ELF_DEBUG
				mu_console->printf(" (wand %x %x", wand_page->pageAddress, *((u8 *)magic_wand));
#endif
				wand_page->pageAddress = page->pageAddress;
				__asm__ __volatile__("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "%eax");
#ifdef ELF_DEBUG
				mu_console->printf("->%x %x) len 0x%x phys 0x%x off 0x%x\n",
						*((u8 *)magic_wand), wand_page->pageAddress,
						copy, page->pageAddress * 0x1000, (virt + copied) % 0x1000);
#endif

				u8 *dest = magic_wand + ((virt + copied) % 0x1000);

				if (ph->p_filesz == 0) {
#ifdef ELF_DEBUG
					mu_console->printf("\nblanking %x\n", copy);
#endif
					memset(dest, 0, copy);
				} else if (ph->p_filesz < ph->p_memsz) {
					s32 file_left = static_cast<s32>(ph->p_filesz) - copied;
					if (static_cast<s32>(copy) > file_left) {
						if (file_left < 0) file_left = 0;
						if (file_left) {
							memcpy(dest, (u8 *)(image + phys + copied), file_left);
						}
						memset(dest + file_left, 0, copy - file_left);
					} else {
						memcpy(dest, (u8 *)(image + phys + copied), copy);
					}
				} else if (ph->p_filesz == ph->p_memsz) {
					memcpy(dest, (u8 *)(image + phys + copied), copy);
				} else {
					AkariPanic("ph->p_filesz is greater than ph->p_memsz!");
				}

				copied += copy;
			}

#ifdef ELF_DEBUG
			mu_console->putChar('\n');
#endif
		} else {
			// Ignore it.
		}
	}

	// Cleaning up our hole in memory.
	wand_page->pageAddress = old_wand_pA;
	__asm__ __volatile__("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "%eax");
	mu_memory->free(magic_wand);

	return true;
}

