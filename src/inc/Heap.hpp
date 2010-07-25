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

#ifndef __HEAP_HPP__
#define __HEAP_HPP__

#include <arch.hpp>
#include <OrderedArray.hpp>

class Heap {
public:
	class Entry {
	public:
		Entry(ptr_t, u32, bool);

		ptr_t start; u32 size;
		bool isHole;
	};

	Heap(ptr_t start, ptr_t end, ptr_t max, u32 index_size, bool supervisor, bool readonly);

	void *alloc(u32);
	void *allocAligned(u32);
	void free(void *);

	ptr_t start() const;
	ptr_t used() const;
	const OrderedArray<Entry> &index() const;

protected:
	static bool IndexSort(const Entry &, const Entry &);

	s32 indexOfEntryStarting(ptr_t start) const;
	s32 indexOfEntryEnding(ptr_t end) const;
	s32 smallestHole(u32) const;
	s32 smallestAlignedHole(u32) const;

	OrderedArray<Entry> _index;
	ptr_t _start, _end, _max;
	ptr_t _used;
	bool _supervisor, _readonly;
};

#endif

