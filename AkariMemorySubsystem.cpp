#include <AkariMemorySubsystem.hpp>
#include <debug.hpp>

AkariMemorySubsystem::AkariMemorySubsystem(u32 upperMemory): _upperMemory(upperMemory), _placementAddress(0), _heap(0)
{ }

u8 AkariMemorySubsystem::VersionMajor() const { return 0; }
u8 AkariMemorySubsystem::VersionMinor() const { return 1; }
const char *AkariMemorySubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariMemorySubsystem::VersionProduct() const { return "Akari Memory Heap"; }

void AkariMemorySubsystem::SetPlacementMode(u32 addr) {
	ASSERT(!_placementAddress);

	_placementAddress = addr;
}

void AkariMemorySubsystem::SetPaging(bool mode) {
	ASSERT(mode);		// TODO support turning paging off [if ever required?!]
	// RESUME from here; see paging.cpp from lobstertime (installpaging())
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

