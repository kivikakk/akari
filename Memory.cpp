#include <Memory.hpp>
#include <Console.hpp>
#include <Descriptor.hpp>
#include <Akari.hpp>
#include <POSIX.hpp>
#include <debug.hpp>
#include <entry.hpp>
#include <physmem.hpp>

// Index into _frames, and the bit offset within a single entry
#define INDEX_BIT(n)	((n)/(8*4))
#define OFFSET_BIT(n)	((n)%(8*4))

// where in virtual memory will the kernel heap lie?
#define KHEAP_START	0xC0000000
// how big will it start at? (5MiB)
#define KHEAP_INITIAL_SIZE	0x500000
// how many elements can exist within the heap?
#define HEAP_INDEX_SIZE		0x20000
// minimum size of a heap
#define HEAP_MIN_SIZE		0x500000

Memory::Memory(u32 upperMemory):
_upperMemory(upperMemory), _placementAddress(0), _frames(0), _frameCount(0), _heap(0),
_kernelDirectory(0), _activeDirectory(0)
{ }

u8 Memory::versionMajor() const { return 0; }
u8 Memory::versionMinor() const { return 1; }
const char *Memory::versionManufacturer() const { return "Akari"; }
const char *Memory::versionProduct() const { return "Akari Memory Heap"; }

void Memory::setPlacementMode(u32 addr) {
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
#define KERNEL_HEAP_PROMISC true

void Memory::setPaging(bool mode) {
	ASSERT(mode);		// TODO support turning paging off [if ever required?! who knows! :)]

	_frameCount = (0x100000 + _upperMemory * 1024) / 0x1000;
	_frames = (u32 *)alloc(INDEX_BIT(_frameCount) + 1);
	POSIX::memset(_frames, 0, INDEX_BIT(_frameCount) + 1);

	_kernelDirectory = PageDirectory::Allocate();

	for (u32 i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000)
		_kernelDirectory->getPage(i, true);
	
	u32 base = 0;
	while (base < (_placementAddress + sizeof(Heap))) {
		_kernelDirectory->getPage(base, true)->allocFrame(base, false, KERNEL_HEAP_PROMISC);
		base += 0x1000;
	}

	for (u32 i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000)
		_kernelDirectory->getPage(i, true)->allocAnyFrame(false, KERNEL_HEAP_PROMISC);

	Akari->descriptor->idt->installHandler(14, this->PageFault);

	switchPageDirectory(_kernelDirectory);
	_heap = new Heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 0xCFFFF000, false, !(KERNEL_HEAP_PROMISC));
	_placementAddress = 0;
	
	// _activeDirectory = _kernelDirectory->Clone();
	// SwitchPageDirectory(_activeDirectory);
}

void *Memory::alloc(u32 n, u32 *phys) {
	if (!_placementAddress) {
		ASSERT(_heap);
		void *addr = _heap->alloc(n);
		if (phys) {
			Page *page = _kernelDirectory->getPage((u32)addr, false);
			ASSERT(page);
			*phys = page->pageAddress * 0x1000 + ((u32)addr & 0xFFF);
		}
		return addr;
	}

	ASSERT(_placementAddress);

	if (phys)
		*phys = _placementAddress;

	u32 addr = _placementAddress;
	_placementAddress += n;
	return (void *)addr;
}

void *Memory::allocAligned(u32 n, u32 *phys) {
	if (!_placementAddress) {
		ASSERT(_heap);
		void *addr = _heap->allocAligned(n);
		if (phys) {
			Page *page = _kernelDirectory->getPage((u32)addr, false);
			ASSERT(page);
			*phys = page->pageAddress * 0x1000 + ((u32)addr & 0xFFF);
		}
		return addr;
	}

	ASSERT(_placementAddress);

	if (_placementAddress & 0xFFF)
		_placementAddress = (_placementAddress & ~0xFFF) + 0x1000;
	
	if (phys)
		*phys = _placementAddress;

	u32 addr = _placementAddress;
	_placementAddress += n;
	return (void *)addr;
}

void Memory::free(void *p) {
	if (!_placementAddress) {
		ASSERT(_heap);
		// TODO: AkariPanic("implement Free() for heaps");
		return;
	}
	
	AkariPanic("Memory: tried to Free() in placement mode");
}

void *Memory::PageFault(struct modeswitch_registers *r) {
	u32 faultingAddress;
	__asm__ __volatile__("mov %%cr2, %0" : "=r" (faultingAddress));

	bool notPresent =	!(r->callback.err_code & 0x1);
	bool writeOp = 		r->callback.err_code & 0x2;
	bool userMode = 	r->callback.err_code & 0x4;
	bool reserved = 	r->callback.err_code & 0x8;
	bool insFetch = 	r->callback.err_code & 0x10;

	Akari->console->putString("\nPage fault!\n");
	if (notPresent) Akari->console->putString(" * Page wasn't present.\n");
	if (writeOp) 	Akari->console->putString(" * Write operation.\n");
	if (userMode) 	Akari->console->putString(" * From user-mode.\n");
	if (reserved) 	Akari->console->putString(" * Clobbered reserved bits in page.\n");
	if (insFetch) 	Akari->console->putString(" * On instruction fetch.\n");

	Akari->console->putString("Address: 0x");
	Akari->console->putInt(faultingAddress, 16);
	Akari->console->putChar('\n');

	while (1)
		__asm__ __volatile__("hlt");

	return 0;
}

void Memory::switchPageDirectory(PageDirectory *dir) {
	u32 phys = dir->physicalAddr;
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
}

void Memory::setFrame(u32 addr) {
	u32 frame = addr / 0x1000;
	u32 idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	_frames[idx] |= (1 << off);
}

void Memory::clearFrame(u32 addr) {
	u32 frame = addr / 0x1000;
	u32 idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	_frames[idx] &= ~(1 << off);
}

bool Memory::testFrame(u32 addr) const {
	u32 frame = addr / 0x1000;
	u32 idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	return (_frames[idx] & (1 << off));
}

u32 Memory::freeFrame() const {
	for (u32 i = 0; i < INDEX_BIT(_frameCount); ++i)
		if (_frames[i] != 0xFFFFFFFF) {
			for (u32 j = 0; j < 32; ++j)
				if (!(_frames[i] & (1 << j)))
					return (i * 32 + j) * 0x1000;
			AkariPanic("FreeFrame thought it found a free frame, but didn't find one?");
		}
	
	AkariPanic("no frames free! Time to panic. :)");
}

// Heap

Memory::Heap::Heap(u32 start, u32 end, u32 max, bool supervisor, bool readonly):
_index((Entry *)start, HEAP_INDEX_SIZE, IndexSort),
_start(start), _end(end), _max(max), _supervisor(supervisor), _readonly(readonly)
{
	ASSERT(start % 0x1000 == 0);
	ASSERT(end % 0x1000 == 0);
	ASSERT(max % 0x1000 == 0);

	start += sizeof(Entry) * HEAP_INDEX_SIZE;
	
	if (start & 0xFFFF)
		start = (start & 0xFFFFF000) + 0x1000;

	ASSERT(_index.size() == 0);
	_index.insert(Entry(start, end - start, true));
	ASSERT(_index.size() == 1);
	ASSERT(_index[0].start == start);
	ASSERT(_index[0].size == end - start);
	ASSERT(_index[0].isHole);
}

void *Memory::Heap::alloc(u32 n) {
	s32 it = smallestHole(n);
	ASSERT(it >= 0);		// TODO: resize instead

	Entry hole = _index[it];
	_index.remove(it);

	u32 dataStart = hole.start;
	if (n < hole.size) {
		// the hole is now forward by `n', and less by `n'
		hole.size -= n;
		hole.start += n;
		_index.insert(hole);
	} else {
		// nothing: it was exactly the right size
	}

	Entry data(dataStart, n, false);
	_index.insert(data);
	return (void *)dataStart;
}

void *Memory::Heap::allocAligned(u32 n) {
	s32 it = smallestAlignedHole(n);
	ASSERT(it >= 0);		// TODO: resize instead

	Entry hole = _index[it];
	_index.remove(it);

	u32 dataStart = hole.start;

	if (dataStart & 0xFFF) {
		// this isn't aligned! 
		u32 oldSize = hole.size;

		dataStart = (dataStart & 0xFFFFF000) + 0x1000;
		hole.size = 0x1000 - (hole.start & 0xFFF);
		_index.insert(hole);

		hole.start = dataStart;
		hole.size = oldSize - hole.size;
	}

	// by here, `hole' is an un-inserted hole, up to the end of
	// the hole we originally had, but starting where our data
	// wants to start.

	if (n < hole.size) {
		hole.size -= n;
		hole.start += n;
		_index.insert(hole);
	} else {
		// right size (as in Alloc())
	}

	Entry data(dataStart, n, false);
	_index.insert(data);
	return (void *)dataStart;
}

Memory::Heap::Entry::Entry(u32 start, u32 size, bool isHole):
start(start), size(size), isHole(isHole)
{ }

bool Memory::Heap::IndexSort(const Entry &a, const Entry &b) {
	return a.size < b.size;
}

s32 Memory::Heap::smallestHole(u32 n) const {
	u32 it = 0;
	ASSERT(_index.size() > 0);

	while (it < _index.size()) {
		const Entry &entry = _index[it];
		if (!entry.isHole) {
			++it; continue;
		}
		if (entry.size >= n) {
			return it;
		}
		++it;
	}
	AkariPanic("no smallest hole in SmallestHole!");
	return -1;
}

s32 Memory::Heap::smallestAlignedHole(u32 n) const {
	u32 it = 0;
	while (it < _index.size()) {
		const Entry &entry = _index[it];
		if (!entry.isHole) {
			++it; continue;
		}

		// calculate how far we have to travel from the start to get to a page-align
		s32 off = 0;
		if (entry.start & 0xFFF)
			off = 0x1000 - (entry.start & 0xFFF);

		// if the size of this hole is large enough for our needs, considering we have
		// `off' bytes of waste, then it's good
		if (((s32)entry.size - off) >= (s32)n)
			return it;

		++it;
	}
	AkariPanic("no smallest hole in SmallestAlignedHole!");
	return -1;
}

// Page

void Memory::Page::allocAnyFrame(bool kernel, bool writeable) {
	ASSERT(!pageAddress);

	u32 addr = Akari->memory->freeFrame();
	Akari->memory->setFrame(addr);
	present = true;
	readwrite = writeable;
	user = !kernel;
	pageAddress = addr / 0x1000;
}

void Memory::Page::allocFrame(u32 addr, bool kernel, bool writeable) {
	ASSERT(!pageAddress);

	Akari->memory->setFrame(addr);
	present = true;
	readwrite = writeable;
	user = !kernel;
	pageAddress = addr / 0x1000;
}

// PageTable

Memory::PageTable *Memory::PageTable::Allocate(u32 *phys) {
	PageTable *table = (PageTable *)Akari->memory->allocAligned(sizeof(PageTable), phys);
	POSIX::memset(table, 0, sizeof(PageTable));
	return table;
}

Memory::PageTable *Memory::PageTable::clone(u32 *phys) const {
	// TODO: review this
	PageTable *t = PageTable::Allocate(phys);

	for (u32 i = 0; i < 1024; ++i) {
		if (!pages[i].pageAddress)
			continue;
		t->pages[i].allocAnyFrame(false, false);

		if (pages[i].present)	t->pages[i].present = true;
		if (pages[i].readwrite)	t->pages[i].readwrite = true;
		if (pages[i].user)		t->pages[i].user = true;
		if (pages[i].accessed)	t->pages[i].accessed = true;
		if (pages[i].dirty)		t->pages[i].dirty = true;

		// physically copy data over
		ASSERT(t->pages[i].pageAddress);
		AkariCopyFramePhysical(pages[i].pageAddress * 0x1000, t->pages[i].pageAddress * 0x1000);
	}

	return t;
}

// PageDirectory

Memory::PageDirectory *Memory::PageDirectory::Allocate() {
	u32 phys;

	PageDirectory *dir = (PageDirectory *)Akari->memory->allocAligned(sizeof(PageDirectory), &phys);
	POSIX::memset(dir, 0, sizeof(PageDirectory));
	u32 off = (u32)dir->tablePhysicals - (u32)dir;
	dir->physicalAddr = phys + off;						// check above

	return dir;
}

Memory::Page *Memory::PageDirectory::getPage(u32 addr, bool make) {
	addr /= 0x1000;

	u32 idx = addr / 1024, entry = addr % 1024;
	if (this->tables[idx])
		return &tables[idx]->pages[entry];
	else if (make) {
		u32 phys;
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
		if (Akari->memory->_kernelDirectory->tables[i] == tables[i]) {
			// kernel has this table, so just link it
			d->tables[i] = tables[i];
			d->tablePhysicals[i] = tablePhysicals[i];
		} else {
			// copy it
			u32 phys;
			d->tables[i] = tables[i]->clone(&phys);
			d->tablePhysicals[i] = phys | 0x7;		// present, r/w, user
		}
	}

	return d;
}

