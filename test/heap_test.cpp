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

#include "test_helper.hpp"

#include <cstdio>

#include "Heap.hpp"
#include "../Heap.cpp"

void report(const Heap *h) {
	const OrderedArray<Heap::Entry> &e = h->index();

	printf("Heap with %ld entries\n", e.size());
	for (u32 i = 0; i < e.size(); ++i) {
		printf("  %ld:\t%llx\t+ %lx\t(%s)\n",
				i, e[i].start - h->start(), e[i].size, e[i].isHole ? "hole" : "data");
	}
}

int main() {
	char *area = new char[0x100fff];
	ptr_t aligned = (ptr_t)area;
	if (aligned & 0xfff)
		aligned = (aligned & ~0xfff) + 0x1000;

	ptr_t end = aligned + 0x100000;

	Heap *h = new Heap(aligned, end, end, 128, false, true);

	ptr_t ptr = (ptr_t) h->alloc(0x700);
	printf("offset from start: %llx\n", ptr - aligned);

	ptr_t ptr2 = (ptr_t) h->alloc(0x700);
	printf("offset from start: %llx\n", ptr2 - aligned);

	ptr_t ptr3 = (ptr_t) h->allocAligned(0x2);
	printf("offset from start: %llx\n", ptr3 - aligned);

	ptr_t ptr4 = (ptr_t) h->alloc(0x100);
	printf("offset from start: %llx\n", ptr4 - aligned);

	ptr_t ptr5 = (ptr_t) h->alloc(0x100);
	printf("offset from start: %llx\n", ptr5 - aligned);

	ptr_t ptr6 = (ptr_t) h->alloc(0x100);
	printf("offset from start: %llx\n", ptr6 - aligned);

	h->free((void *)ptr3);

	ptr_t ptr7 = (ptr_t) h->alloc(0x3);
	printf("offset from start: %llx\n", ptr7 - aligned);

	ptr_t ptr8 = (ptr_t) h->alloc(0x2);
	printf("offset from start: %llx\n", ptr8 - aligned);

	ptr_t ptr9 = (ptr_t) h->alloc(0x2);
	printf("offset from start: %llx\n", ptr9 - aligned);

	ptr_t ptr10 = (ptr_t) h->alloc(0x1);
	printf("offset from start: %llx\n", ptr10 - aligned);

	report(h);

	h->free((void *)ptr7);
	h->free((void *)ptr9);
	h->free((void *)ptr10);

	report(h);

	ptr_t ptr11 = (ptr_t) h->alloc(0x6);
	printf("offset from start: %llx\n", ptr11 - aligned);

	report(h);

	return 0;
}
