#ifndef __AKARI_MEMORY_SUBSYSTEM_HPP__
#define __AKARI_MEMORY_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

class AkariMemorySubsystem : public AkariSubsystem {
	public:
		AkariMemorySubsystem(Akari *);

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		u32 GetHeapSize() const;
		void CreateHeap(void *);

		void *Alloc(u32);
		void *AllocAligned(u32);
		void *AllocPhys(u32, u32 *);
		void *AllocPhysAligned(u32, u32 *);
	
	protected:
		class Heap {

		};

		Heap *_heap;
};

#endif

