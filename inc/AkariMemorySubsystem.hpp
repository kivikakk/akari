#ifndef __AKARI_MEMORY_SUBSYSTEM_HPP__
#define __AKARI_MEMORY_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>
#include <OrderedArray.hpp>

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
	
	// protected:
	// XXX _activeDirectory is accessed from outside, as are these classes! Damn! Refactor!
		static void PageFault(struct registers);

		class PageDirectory;
		void SwitchPageDirectory(PageDirectory *);

		void SetFrame(u32);
		void ClearFrame(u32);
		bool TestFrame(u32) const;
		u32 FreeFrame() const;
		
		class Heap {
			public:
				Heap(u32, u32, u32, bool, bool);

				void *Alloc(u32);
				void *AllocAligned(u32);

			protected:
				class Entry {
					public:
						Entry(u32, u32, bool);

						u32 start, size;
						bool isHole;
				};

				static bool IndexSort(const Entry &, const Entry &);

				s32 SmallestHole(u32) const;
				s32 SmallestAlignedHole(u32) const;

				OrderedArray<Entry> _index;
				u32 _start, _end, _max;
				bool _supervisor, _readonly;
		};

		class Page {
			public:
				void AllocAnyFrame(bool, bool);
				void AllocFrame(u32, bool, bool);

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
		} __attribute__((__packed__));

		class PageTable {
			public:
				static PageTable *Allocate(u32 *);

				PageTable *Clone(u32 *) const;

				Page pages[1024];
		} __attribute__((__packed__));

		class PageDirectory {
			public:
				static PageDirectory *Allocate();

				Page *GetPage(u32, bool);
				PageDirectory *Clone() const;

				PageTable *tables[1024];
				u32 tablePhysicals[1024];
				u32 physicalAddr;
		} __attribute__((__packed__));

		u32 _upperMemory;

		u32 _placementAddress;
		u32 *_frames, _frameCount;
		Heap *_heap;
		PageDirectory *_kernelDirectory, *_activeDirectory;
};

#endif

