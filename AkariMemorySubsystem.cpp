#include <AkariMemorySubsystem.hpp>
#include <Akari.hpp>
#include <POSIX.hpp>
#include <debug.hpp>

// Index into _frames, and the bit offset within a single entry
#define INDEX_BIT(n)	((n)/(8*4))
#define OFFSET_BIT(n)	((n)%(8*4))

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
}

void *AkariMemorySubsystem::Alloc(u32 n, u32 *phys) {
	if (_heap)
		AkariPanic("implement Alloc() for heaps");

	ASSERT(_placementAddress);

	if (phys)
		*phys = _placementAddress;

	u32 addr = _placementAddress;
	_placementAddress += n;
	return (void *)addr;
}

void *AkariMemorySubsystem::AllocAligned(u32 n, u32 *phys) {
	if (_heap)
		AkariPanic("implement AllocAligned() for heaps");

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
	if (_heap)
		AkariPanic("implement Free() for heaps");
	
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

AkariMemorySubsystem::PageDirectory *AkariMemorySubsystem::PageDirectory::Allocate() {
	u32 phys;

	PageDirectory *dir = (PageDirectory *)Akari->Memory->AllocAligned(sizeof(PageDirectory), &phys);
	POSIX::memset(dir, 0, sizeof(PageDirectory));
	// WARNING TODO XXX: is this calculation below correct? using dir and dir->tablePhysicals in this fashion?
	// do we not need to take any addresses?
	u32 off = (u32)dir->tablePhysicals - (u32)dir;
	Akari->Console->PutString("offset: ");
	Akari->Console->PutInt(off, 16);
	Akari->Console->PutString("\n");
	dir->physicalAddr = phys + off;						// check above

	return dir;
}

