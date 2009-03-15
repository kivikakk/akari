#include <AkariMemorySubsystem.hpp>

AkariMemorySubsystem::AkariMemorySubsystem()
{ }

u8 AkariMemorySubsystem::VersionMajor() const { return 0; }
u8 AkariMemorySubsystem::VersionMinor() const { return 1; }
const char *AkariMemorySubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariMemorySubsystem::VersionProduct() const { return "Akari Memory Heap"; }

u32 AkariMemorySubsystem::GetHeapSize() const {
	return sizeof(AkariMemorySubsystem::Heap);
}

void AkariMemorySubsystem::CreateKernelHeap(void *addr) {
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
