#ifndef __AKARI_MEMORY_SUBSYSTEM_HPP__
#define __AKARI_MEMORY_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

class AkariMemorySubsystem : public AkariSubsystem {
	public:
		AkariMemorySubsystem();

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		void SetPlacementMode(u32);

		void *Alloc(u32, u32 *phys=0);
		void *AllocAligned(u32, u32 *phys=0);
		void Free(void *);
	
	protected:
		class Heap {
			Heap(u32, u32, u32, bool, bool);

			class Header {
				Header(u32, bool);

				u32 size;
				bool is_hole;
			};
		};

		Heap *_heap;
		u32 _placementAddress;
};

#endif

