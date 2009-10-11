#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include <Subsystem.hpp>
#include <OrderedArray.hpp>

class Memory : public Subsystem {
public:
	Memory(u32);

	u8 versionMajor() const;
	u8 versionMinor() const;
	const char *versionManufacturer() const;
	const char *versionProduct() const;

	void setPlacementMode(u32);
	void setPaging(bool);

	void *alloc(u32, u32 *phys=0);
	void *allocAligned(u32, u32 *phys=0);
	void free(void *);

// protected:
// XXX _activeDirectory is accessed from outside, as are these classes! Damn! Refactor!
	static void *PageFault(struct modeswitch_registers *);

	class PageDirectory;
	void switchPageDirectory(PageDirectory *);

	void setFrame(u32);
	void clearFrame(u32);
	bool testFrame(u32) const;
	u32 freeFrame() const;
	
	class Heap {
	public:
		Heap(u32, u32, u32, bool, bool);

		void *alloc(u32);
		void *allocAligned(u32);

	protected:
		class Entry {
		public:
			Entry(u32, u32, bool);

			u32 start, size;
			bool isHole;
		};

		static bool IndexSort(const Entry &, const Entry &);

		s32 smallestHole(u32) const;
		s32 smallestAlignedHole(u32) const;

		OrderedArray<Entry> _index;
		u32 _start, _end, _max;
		bool _supervisor, _readonly;
	};

	class Page {
	public:
		void allocAnyFrame(bool, bool);
		void allocFrame(u32, bool, bool);

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

		PageTable *clone(u32 *) const;

		Page pages[1024];
	} __attribute__((__packed__));

	class PageDirectory {
	public:
		static PageDirectory *Allocate();

		Page *getPage(u32, bool);
		PageDirectory *clone() const;

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

