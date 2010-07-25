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

#include <Akari.hpp>
#include <Console.hpp>
#include <Heap.hpp>
#include <string>

Heap::Heap(ptr_t start, ptr_t end, ptr_t max, u32 index_size, bool supervisor, bool readonly):
_index(reinterpret_cast<Entry *>(start), index_size, IndexSort),
_start(start), _end(end), _max(max), _used(0), _supervisor(supervisor), _readonly(readonly)
{
	ASSERT(start % 0x1000 == 0);
	ASSERT(end % 0x1000 == 0);
	ASSERT(max % 0x1000 == 0);

	start += sizeof(Entry) * index_size;
	
	if (start & 0xFFFF)
		start = (start & ~0xFFF) + 0x1000;

	_used += (start - _start);
	_index.insert(Entry(start, end - start, true));
	ASSERT(start < end);
}

void *Heap::alloc(u32 n) {
	s32 it = smallestHole(n);
	ASSERT(it >= 0);		// TODO: resize instead

	Entry hole = _index[it];
	_index.remove(it);

	ptr_t dataStart = hole.start;
	if (n < hole.size) {
		// the hole is now forward by `n', and less by `n'
		hole.size -= n;
		hole.start += n;
		_index.insert(hole);
	} else {
		// nothing: it was exactly the right size
	}

	_used += n;

	Entry data(dataStart, n, false);
	_index.insert(data);
	return reinterpret_cast<void *>(dataStart);
}

void *Heap::allocAligned(u32 n) {
	s32 it = smallestAlignedHole(n);
	ASSERT(it >= 0);		// TODO: resize instead

	Entry hole = _index[it];
	_index.remove(it);

	ptr_t dataStart = hole.start;

	if (dataStart & 0xFFF) {
		// this isn't aligned! 
		u32 oldSize = hole.size;

		dataStart = (dataStart & ~0xFFF) + 0x1000;
		hole.size = 0x1000 - (hole.start & 0xFFF);
		_index.insert(hole);

		hole.start = dataStart;
		hole.size = oldSize - hole.size;
	}

	// by here, `hole' is an un-inserted hole, up to the end of
	// the hole we originally had, but starting where our data
	// wants to start.

	if (n < hole.size) {
		hole.size -= n;
		hole.start += n;
		_index.insert(hole);
	} else {
		// right size (as in Alloc())
	}

	_used += n;

	Entry data(dataStart, n, false);
	_index.insert(data);
	return reinterpret_cast<void *>(dataStart);
}

s32 Heap::indexOfEntryStarting(ptr_t start) const {
	for (u32 i = 0; i < _index.size(); ++i) {
		if (_index[i].start == start)
			return i;
	}

	return -1;
}

s32 Heap::indexOfEntryEnding(ptr_t end) const {
	for (u32 i = 0; i < _index.size(); ++i) {
		if (_index[i].start + _index[i].size == end)
			return i;
	}

	return -1;
}

void Heap::free(void *p) {
	s32 i = indexOfEntryStarting((ptr_t)p);
	if (i == -1) return;

	_index[i].isHole = true;
	
	s32 b = indexOfEntryEnding((ptr_t)p);

	if (b != -1 && _index[b].isHole) {
		ptr_t new_size = _index[b].size + _index[i].size,
			  new_end = new_size + _index[b].start;

		_index[b].size = new_size;
		_index.remove(i);
		i = indexOfEntryEnding(new_end);
	}

	s32 e = indexOfEntryStarting(_index[i].start + _index[i].size);
	if (e != -1 && _index[e].isHole) {
		_index[i].size += _index[e].size;
		_index.remove(e);
	}
}

ptr_t Heap::start() const {
	return _start;
}

ptr_t Heap::used() const {
	return _used;
}

const OrderedArray<Heap::Entry> &Heap::index() const {
	return _index;
}

Heap::Entry::Entry(ptr_t start, u32 size, bool isHole):
start(start), size(size), isHole(isHole)
{ }

bool Heap::IndexSort(const Entry &a, const Entry &b) {
	return a.size < b.size;
}

s32 Heap::smallestHole(u32 n) const {
	u32 it = 0;
	ASSERT(_index.size() > 0);

	while (it < _index.size()) {
		const Entry &entry = _index[it];
		if (!entry.isHole) {
			++it; continue;
		}
		if (entry.size >= n) {
			return it;
		}
		++it;
	}
	Akari->console->printf("looking for %x bytes, couldn't find\n", n);
	AkariPanic("no smallest hole in SmallestHole!");
	return -1;
}

s32 Heap::smallestAlignedHole(u32 n) const {
	u32 it = 0;
	while (it < _index.size()) {
		const Entry &entry = _index[it];
		if (!entry.isHole) {
			++it; continue;
		}

		// calculate how far we have to travel from the start to get to a page-align
		s32 off = 0;
		if (entry.start & 0xFFF)
			off = 0x1000 - (entry.start & 0xFFF);

		// if the size of this hole is large enough for our needs, considering we have
		// `off' bytes of waste, then it's good
		if ((static_cast<s32>(entry.size) - off) >= static_cast<s32>(n))
			return it;

		++it;
	}
	Akari->console->printf("looking for %x bytes, couldn't find\n", n);
	AkariPanic("no smallest hole in SmallestAlignedHole!");
	return -1;
}

