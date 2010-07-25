// This file is part of Akari.
// Copyright 2010 Arlen Cuss
//
// Akari is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Akari is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Akari.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include <Heap.hpp>
#include <Subsystem.hpp>
#include <OrderedArray.hpp>

class Memory : public Subsystem {
public:
	Memory(ptr_t);

	u8 versionMajor() const;
	u8 versionMinor() const;
	const char *versionManufacturer() const;
	const char *versionProduct() const;

	void setPlacementMode(ptr_t);
	void setPaging(bool);

	void *alloc(u32, ptr_t *phys=0);
	void *allocAligned(u32, ptr_t *phys=0);
	bool free(void *);

// protected:
// XXX _activeDirectory is accessed from outside, as are these classes! Damn! Refactor!
	static void *PageFault(struct modeswitch_registers *);

	class PageDirectory;
	void switchPageDirectory(PageDirectory *);

	void setFrame(ptr_t);
	void clearFrame(ptr_t);
	bool testFrame(ptr_t) const;
	ptr_t freeFrame() const;
	
	class Page {
	public:
		void allocAnyFrame(bool kernel, bool writeable);
		void allocFrame(ptr_t, bool kernel, bool writeable);

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
		static PageTable *Allocate(ptr_t *);

		PageTable *clone(ptr_t *) const;

		Page pages[1024];
	} __attribute__((__packed__));

	class PageDirectory {
	public:
		static PageDirectory *Allocate();

		Page *getPage(ptr_t, bool);
		PageDirectory *clone() const;

		PageTable *tables[1024];
		ptr_t tablePhysicals[1024];
		ptr_t physicalAddr;
	} __attribute__((__packed__));

	ptr_t _upperMemory;

	ptr_t _placementAddress;
	ptr_t *_frames, _frameCount;
	Heap *_heap;
	PageDirectory *_kernelDirectory, *_activeDirectory;
};

#endif

