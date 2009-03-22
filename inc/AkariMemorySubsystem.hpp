#ifndef __AKARI_MEMORY_SUBSYSTEM_HPP__
#define __AKARI_MEMORY_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

class AkariMemorySubsystem : public AkariSubsystem {
	public:
		AkariMemorySubsystem(u32);

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		void SetPlacementMode(u32);
		void SetPaging(bool);

		void *Alloc(u32, u32 *phys=0);
		void *AllocAligned(u32, u32 *phys=0);
		void Free(void *);
	
	protected:
		void SetFrame(u32);
		void ClearFrame(u32);
		bool TestFrame(u32) const;
		
		class Heap {
			public:
				Heap(u32, u32, u32, bool, bool);

			protected:
				class Header {
					Header(u32, bool);

					u32 size;
					bool isHole;
				};
		};

		class Page {
			union {
				struct {
					bool present			: 1;
					bool readwrite			: 1;
					bool user				: 1;
					unsigned _reserved0		: 2;
					bool accessed			: 1;
					bool dirty				: 1;
					unsigned _reserved1		: 2;
					unsigned _available0	: 3;
					unsigned pageAddress	: 20;
				} __attribute__((__packed__));
				u32 ulong;
			} __attribute__((__packed__));

			// TODO: functions
		} __attribute__((__packed__));

		class PageTable {
			Page pages[1024];
			// TODO: functions
		} __attribute__((__packed__));

		class PageDirectory {
			public:
				static PageDirectory *Allocate();

				PageTable *tables[1024];
				u32 tablePhysicals[1024];
				u32 physicalAddr;
			// TODO: functions
		} __attribute__((__packed__));

		u32 _upperMemory;

		u32 _placementAddress;
		u32 *_frames, _frameCount;
		Heap *_heap;
		PageDirectory *_kernelDirectory, *_currentDirectory;
};

#endif

