#include <AkariMemorySubsystem.hpp>
#include <Akari.hpp>
#include <POSIX.hpp>
#include <debug.hpp>
#include <entry.hpp>

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

AkariMemorySubsystem::AkariMemorySubsystem(u32 upperMemory):
_upperMemory(upperMemory), _placementAddress(0), _frames(0), _frameCount(0), _heap(0)
{ }

u8 AkariMemorySubsystem::VersionMajor() const { return 0; }
u8 AkariMemorySubsystem::VersionMinor() const { return 1; }
const char *AkariMemorySubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariMemorySubsystem::VersionProduct() const { return "Akari Memory Heap"; }

void AkariMemorySubsystem::SetPlacementMode(u32 addr) {
	ASSERT(!_placementAddress);

	_placementAddress = addr;
}

/**
 * Initialiases paging mode.
 * TODO let us turn it off too!
 *
 * @param mode Whether to turn paging on or off.
 */
void AkariMemorySubsystem::SetPaging(bool mode) {
	ASSERT(mode);		// TODO support turning paging off [if ever required?! who knows! :)]

	_frameCount = (0x100000 + _upperMemory * 1024) / 0x1000;
	_frames = (u32 *)Alloc(INDEX_BIT(_frameCount) + 1);
	POSIX::memset(_frames, 0, INDEX_BIT(_frameCount) + 1);

	_kernelDirectory = PageDirectory::Allocate();
	_heap = new Heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 0xCFFFF000, false, true);

	for (u32 i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000)
		_kernelDirectory->GetPage(i, true);
	
	u32 base = 0;
	while (base < _placementAddress) {
		_kernelDirectory->GetPage(base, true)->AllocFrame(base, false, false);
		base += 0x1000;
	}

	Akari->Console->PutString("Wrote up to page ");
	Akari->Console->PutInt(base, 16);
	Akari->Console->PutChar('\n');

	for (u32 i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000)
		_kernelDirectory->GetPage(i, true)->AllocFrame(FreeFrame(), false, false);

	_placementAddress = 0;
}

void *AkariMemorySubsystem::Alloc(u32 n, u32 *phys) {
	if (!_placementAddress) {
		ASSERT(_heap);
		void *addr = _heap->Alloc(n);
		if (phys) {
			Page *page = _kernelDirectory->GetPage((u32)addr, false);
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

void *AkariMemorySubsystem::AllocAligned(u32 n, u32 *phys) {
	if (!_placementAddress) {
		ASSERT(_heap);
		void *addr = _heap->AllocAligned(n);
		if (phys) {
			Page *page = _kernelDirectory->GetPage((u32)addr, false);
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

void AkariMemorySubsystem::Free(void *p) {
	if (!_placementAddress) {
		ASSERT(_heap);
		AkariPanic("implement Free() for heaps");
	}
	
	AkariPanic("AkariMemorySubsystem: tried to Free() in placement mode");
}

void AkariMemorySubsystem::SetFrame(u32 addr) {
	u32 frame = addr / 0x1000;
	u32 idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	_frames[idx] |= (1 << off);
}

void AkariMemorySubsystem::ClearFrame(u32 addr) {
	u32 frame = addr / 0x1000;
	u32 idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	_frames[idx] &= ~(1 << off);
}

bool AkariMemorySubsystem::TestFrame(u32 addr) const {
	u32 frame = addr / 0x1000;
	u32 idx = INDEX_BIT(frame), off = OFFSET_BIT(frame);
	ASSERT(idx < INDEX_BIT(_frameCount));
	return (_frames[idx] & (1 << off));
}

u32 AkariMemorySubsystem::FreeFrame() const {
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

AkariMemorySubsystem::Heap::Heap(u32 start, u32 end, u32 max, bool supervisor, bool readonly):
_index((Entry *)start, HEAP_INDEX_SIZE, IndexSort),
_start(start), _end(end), _max(max), _supervisor(supervisor), _readonly(readonly)
{
	ASSERT(start % 0x1000 == 0);
	ASSERT(end % 0x1000 == 0);
	ASSERT(max % 0x1000 == 0);

	start += sizeof(Entry) * HEAP_INDEX_SIZE;
	
	if (start & 0xFFFF)
		start = (start & 0xFFFFF000) + 0x1000;

	_index.insert(Entry(start, end - start, true));
}

void *AkariMemorySubsystem::Heap::Alloc(u32 n) {
	s32 it = SmallestHole(n);
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

void *AkariMemorySubsystem::Heap::AllocAligned(u32 n) {
	s32 it = SmallestAlignedHole(n);
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

AkariMemorySubsystem::Heap::Entry::Entry(u32 start, u32 size, bool isHole):
start(start), size(size), isHole(isHole)
{ }

bool AkariMemorySubsystem::Heap::IndexSort(const Entry &a, const Entry &b) {
	return a.size < b.size;
}

s32 AkariMemorySubsystem::Heap::SmallestHole(u32 n) const {
	u32 it = 0;
	while (it < _index._size) {
		const Entry &entry = _index[it];
		if (!entry.isHole) {
			++it; continue;
		}
		if (entry.size >= n)
			return it;
		++it;
	}
	AkariPanic("no smallest hole!");
	return -1;
}

s32 AkariMemorySubsystem::Heap::SmallestAlignedHole(u32 n) const {
	u32 it = 0;
	while (it < _index._size) {
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
	AkariPanic("no smallest hole!");
	return -1;
}

// Page

void AkariMemorySubsystem::Page::AllocFrame(u32 addr, bool kernel, bool writeable) {
	ASSERT(!pageAddress);

	Akari->Memory->SetFrame(addr);
	present = true;
	readwrite = writeable;
	user = !kernel;
	pageAddress = addr / 0x1000;
}

// PageTable

AkariMemorySubsystem::PageTable *AkariMemorySubsystem::PageTable::Allocate(u32 *phys) {
	PageTable *table = (PageTable *)Akari->Memory->AllocAligned(sizeof(PageTable), phys);
	POSIX::memset(table, 0, sizeof(PageTable));
	Akari->Console->PutString("PT(");
	Akari->Console->PutInt((u32)table, 16);
	Akari->Console->PutString(") ");
	return table;
}

// PageDirectory

AkariMemorySubsystem::PageDirectory *AkariMemorySubsystem::PageDirectory::Allocate() {
	u32 phys;

	PageDirectory *dir = (PageDirectory *)Akari->Memory->AllocAligned(sizeof(PageDirectory), &phys);
	POSIX::memset(dir, 0, sizeof(PageDirectory));
	u32 off = (u32)dir->tablePhysicals - (u32)dir;
	dir->physicalAddr = phys + off;						// check above

	return dir;
}

AkariMemorySubsystem::Page *AkariMemorySubsystem::PageDirectory::GetPage(u32 addr, bool make) {
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
