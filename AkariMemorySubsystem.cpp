#include <AkariMemorySubsystem.hpp>

AkariMemorySubsystem::AkariMemorySubsystem(Akari *kernel): AkariSubsystem(kernel), _heap(0) {
}

u8 AkariMemorySubsystem::VersionMajor() const { return 0; }
u8 AkariMemorySubsystem::VersionMinor() const { return 1; }

u32 AkariMemorySubsystem::GetHeapSize() const {
	return sizeof(AkariMemorySubsystem::Heap);
}

void AkariMemorySubsystem::CreateHeap(void *addr) {
	_heap = new (addr) Heap();
}

void *AkariMemorySubsystem::Alloc(u32 n) {
}

void *AkariMemorySubsystem::AllocAligned(u32 n) {
}

void *AkariMemorySubsystem::AllocPhys(u32 n, u32 *phys) {
}

void *AkariMemorySubsystem::AllocPhysAligned(u32 n, u32 *phys) {
}
