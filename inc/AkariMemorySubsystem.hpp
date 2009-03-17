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
		class Heap {
			public:
				Heap(u32, u32, u32, bool, bool);

			protected:
				class Header {
					Header(u32, bool);

					u32 size;
					bool is_hole;
				};
		};

		class Page {
			union {
				struct {
					bool present			: 1;
					bool readwrite			: 1;
					bool user				: 1;
					unsigned _reserved_0	: 2;
					bool accessed			: 1;
					bool dirty				: 1;
					unsigned _reserved_1	: 2;
					unsigned _available_0	: 3;
					unsigned page_address	: 20;
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
			PageTable *tables[1024];
			u32 table_physicals[1024];
			u32 physical_addr;
			// TODO: functions, and get naming convention right
		} __attribute__((__packed__));

		u32 _upperMemory;
		u32 _placementAddress;
		Heap *_heap;
		PageDirectory *_kernelDirectory, *_currentDirectory;
};

#endif

