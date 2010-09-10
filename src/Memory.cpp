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

#include <Memory.hpp>
#include <Console.hpp>
#include <Descriptor.hpp>
#include <Akari.hpp>
#include <Debugger.hpp>
#include <debug.hpp>
#include <entry.hpp>
#include <physmem.hpp>

// Index into _frames, and the bit offset within a single entry
#define INDEX_BIT(n)	((n)/(8*4))
#define OFFSET_BIT(n)	((n)%(8*4))

#define KHEAP_START	0xC0000000
#define KHEAP_INITIAL_SIZE	0x2000000		// 32MiB
#define KHEAP_INDEX_SIZE	0x20000

Memory::Memory(ptr_t upperMemory):
_upperMemory(upperMemory), _placementAddress(0), _frames(0), _frameCount(0), _heap(0),
_kernelDirectory(0), _activeDirectory(0)
{ }

void Memory::setPlacementMode(ptr_t addr) {
	ASSERT(!_placementAddress);

	_placementAddress = addr;
}

/**
 * Initialiases paging mode.
 * TODO let us turn it off too!
 *
 * @param mode Whether to turn paging on or off.
 */

// temp debug flag only, set to true if you want the kheap to be writeable from usermode!
#define KERNEL_HEAP_PROMISC false

void Memory::setPaging(bool mode) {
	ASSERT(mode);		// TODO support turning paging off [if ever required?! who knows! :)]

	_frameCount = (0x100000 + _upperMemory * 1024) / 0x1000;
	// NOTE that INDEX_BIT/OFFSET_BIT are giving indices/offsets for arrays
	// with items of 32-bits, whereas alloc clearly is allocating a multiple
	// of 8-bits. Hence *4. This has been over A YEAR unsolved and caused
	// many bugs where _frames intersected with THE PAGE TABLES. (!!!)
	_frames = static_cast<ptr_t *>(alloc(INDEX_BIT(_frameCount) * 4 + 1));
	memset(_frames, 0, INDEX_BIT(_frameCount) * 4 + 1);

	_kernelDirectory = PageDirectory::Allocate();

	for (ptr_t i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000)
		_kernelDirectory->getPage(i, true);
	
	ptr_t base = 0;
	while (base < (_placementAddress + sizeof(Heap))) {
		_kernelDirectory->getPage(base, true)->allocFrame(base, false, KERNEL_HEAP_PROMISC);
		base += 0x1000;
	}

	for (ptr_t i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000)
		_kernelDirectory->getPage(i, true)->allocAnyFrame(false, KERNEL_HEAP_PROMISC);

	mu_descriptor->idt->installHandler(14, this->PageFault);

	switchPageDirectory(_kernelDirectory);
	_heap = new Heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 0xCFFFF000, KHEAP_INDEX_SIZE, false, !(KERNEL_HEAP_PROMISC));
	_placementAddress = 0;
}

void *Memory::alloc(u32 n, ptr_t *phys) {
	if (!_placementAddress) {
		ASSERT(_heap);
		void *addr = _heap->alloc(n);
		if (phys) {
			Page *page = _kernelDirectory->getPage(reinterpret_cast<ptr_t>(addr), false);
			ASSERT(page);
			*phys = page->pageAddress * 0x1000 + (reinterpret_cast<ptr_t>(addr) & 0xFFF);
		}
		return addr;
	}

	ASSERT(_placementAddress);

	if (phys)
		*phys = _placementAddress;

	ptr_t addr = _placementAddress;
	_placementAddress += n;
	return reinterpret_cast<void *>(addr);
}

void *Memory::allocAligned(u32 n, ptr_t *phys) {
	if (!_placementAddress) {
		ASSERT(_heap);
		void *addr = _heap->allocAligned(n);
		if (phys) {
			Page *page = _kernelDirectory->getPage(reinterpret_cast<ptr_t>(addr), false);
			ASSERT(page);
			*phys = page->pageAddress * 0x1000 + (reinterpret_cast<ptr_t>(addr) & 0xFFF);
		}
		return addr;
	}

	ASSERT(_placementAddress);

	if (_placementAddress & 0xFFF)
		_placementAddress = (_placementAddress & ~0xFFF) + 0x1000;
	
	if (phys)
		*phys = _placementAddress;

	ptr_t addr = _placementAddress;
	_placementAddress += n;
	return reinterpret_cast<void *>(addr);
}

bool Memory::free(void *p) {
	if (!_placementAddress) {
		ASSERT(_heap);
		return _heap->free(p);
	}
	
	AkariPanic("Memory: tried to free() in placement mode");
}

void *Memory::PageFault(struct modeswitch_registers *r) {
	ptr_t faultingAddress;
	__asm__ __volatile__("mov %%cr2, %0" : "=r" (faultingAddress));

	bool notPresent =	!(r->callback.err_code & 0x1);
	bool writeOp = 		r->callback.err_code & 0x2;
	bool userMode = 	r->callback.err_code & 0x4;
	bool reserved = 	r->callback.err_code & 0x8;
	bool insFetch = 	r->callback.err_code & 0x10;

	mu_console->printf("\nPage fault!\n");
	if (notPresent) mu_console->printf(" * Page wasn't present.\n");
	if (writeOp) 	mu_console->printf(" * Write operation.\n");
	if (userMode) 	mu_console->printf(" * From user-mode.\n");
	if (reserved) 	mu_console->printf(" * Clobbered reserved bits in page.\n");
	if (insFetch) 	mu_console->printf(" * On instruction fetch.\n");

	mu_console->printf("Address: 0x%x\n", faultingAddress);

	//mu_debugger->run();

	// Return 0, which tells the interrupt handler to kill this process.
	return 0;
}

void Memory::switchPageDirectory(PageDirectory *dir) {
	ptr_t phys = dir->physicalAddr;
	_activeDirectory = dir;

	__asm__ __volatile__("mov %0, %%cr3" : : "r" (phys));
	__asm__ __volatile__("\
		mov %%cr0, %%eax; \
		or $0x80000000, %%eax; \
		mov %%eax, %%cr0" : : : "%eax");
	__asm__ __volatile__("\
		ljmp $8, $.ljd; \
	.ljd:");

	// XXX do we always necessarily want to use $8 with ljmp here?
	// depends on the gdt we're using.
}

void Memory::setFrame(ptr_t addr) {
	ptr_t frame = addr / 0x1000;
	ptr_t idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	_frames[idx] |= (1 << off);
}

void Memory::clearFrame(ptr_t addr) {
	ptr_t frame = addr / 0x1000;
	ptr_t idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	_frames[idx] &= ~(1 << off);
}

bool Memory::testFrame(ptr_t addr) const {
	ptr_t frame = addr / 0x1000;
	ptr_t idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	return (_frames[idx] & (1 << off));
}

ptr_t Memory::freeFrame() const {
	for (ptr_t i = 0; i < INDEX_BIT(_frameCount); ++i)
		if (_frames[i] != 0xFFFFFFFF) {
			for (ptr_t j = 0; j < 32; ++j)
				if (!(_frames[i] & (1 << j)))
					return (i * 32 + j) * 0x1000;
			AkariPanic("freeFrame thought it found a free frame, but didn't find one?");
		}
	
	AkariPanic("no frames free! Time to panic. :)");
}

// Page

void Memory::Page::allocAnyFrame(bool kernel, bool writeable) {
	ASSERT(!pageAddress);

	ptr_t addr = mu_memory->freeFrame();
	mu_memory->setFrame(addr);
	present = true;
	readwrite = writeable;
	user = !kernel;
	pageAddress = addr / 0x1000;
}

void Memory::Page::allocFrame(ptr_t addr, bool kernel, bool writeable) {
	ASSERT(!pageAddress);

	mu_memory->setFrame(addr);
	present = true;
	readwrite = writeable;
	user = !kernel;
	pageAddress = addr / 0x1000;
}

// PageTable

Memory::PageTable *Memory::PageTable::Allocate(ptr_t *phys) {
	PageTable *table = static_cast<PageTable *>(mu_memory->allocAligned(sizeof(PageTable), phys));
	memset(table, 0, sizeof(PageTable));
	return table;
}

Memory::PageTable *Memory::PageTable::clone(ptr_t *phys) const {
	// TODO: review this
	AkariPanic("PageTable clone not checked");
	PageTable *t = PageTable::Allocate(phys);

	for (u32 i = 0; i < 1024; ++i) {
		if (!pages[i].pageAddress)
			continue;
		t->pages[i].allocAnyFrame(false, false);

		t->pages[i].present = pages[i].present;
		t->pages[i].readwrite = pages[i].readwrite;
		t->pages[i].user = pages[i].user;
		t->pages[i].accessed = pages[i].accessed;
		t->pages[i].dirty = pages[i].dirty;

		// physically copy data over
		ASSERT(t->pages[i].pageAddress);
		AkariCopyFramePhysical(pages[i].pageAddress * 0x1000, t->pages[i].pageAddress * 0x1000);
	}

	return t;
}

// PageDirectory

Memory::PageDirectory *Memory::PageDirectory::Allocate() {
	ptr_t phys;

	PageDirectory *dir = static_cast<PageDirectory *>(mu_memory->allocAligned(sizeof(PageDirectory), &phys));
	memset(dir, 0, sizeof(PageDirectory));
	ptr_t off = reinterpret_cast<ptr_t>(dir->tablePhysicals) - reinterpret_cast<ptr_t>(dir);
	dir->physicalAddr = phys + off;						// check above

	return dir;
}

Memory::Page *Memory::PageDirectory::getPage(ptr_t addr, bool make) {
	addr /= 0x1000;

	ptr_t idx = addr / 1024, entry = addr % 1024;
	if (this->tables[idx])
		return &tables[idx]->pages[entry];
	else if (make) {
		ptr_t phys;
		tables[idx] = PageTable::Allocate(&phys);
		tablePhysicals[idx] = phys | 0x7;	// present, r/w, user
		return &tables[idx]->pages[entry];
	}

	return 0;
}

Memory::PageDirectory *Memory::PageDirectory::clone() const {
	// TODO: review this code
	PageDirectory *d = PageDirectory::Allocate();

	for (u32 i = 0; i < 1024; ++i) {
		if (!tables[i])
			continue;
		if (mu_memory->_kernelDirectory->tables[i] == tables[i]) {
			// kernel has this table, so just link it
			d->tables[i] = tables[i];
			d->tablePhysicals[i] = (tablePhysicals[i] & ~0x7) | 0x5;	// present, user
		} else {
			// copy it
			ptr_t phys;
			d->tables[i] = tables[i]->clone(&phys);
			d->tablePhysicals[i] = phys | 0x7;		// present, r/w, user
		}
	}

	return d;
}

