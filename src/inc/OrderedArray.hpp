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

#ifndef __ORDERED_ARRAY_HPP__
#define __ORDERED_ARRAY_HPP__

#include <arch.hpp>
#include <debug.hpp>
#include <string>

template <typename T>
class OrderedArray {
public:
	typedef bool (*LessThanPredicate)(const T &, const T &);

	OrderedArray(T *array, u32 maxSize, LessThanPredicate lessThan): _array(array), _size(0), _maxSize(maxSize), _lessThan(lessThan) {
		ASSERT(array);
		ASSERT(maxSize);
		ASSERT(lessThan);

		memset(_array, 0, maxSize * sizeof(T));
	}

	~OrderedArray() {
		// I'm assuming we don't want to remove array, as we didn't allocate it ourselves.
	}

	const u32 size() const {
		return _size;
	}

	const T &operator[](u32 index) const {
		ASSERT(index < _size);
		return _array[index];
	}

	T &operator[](u32 index) {
		ASSERT(index < _size);
		return _array[index];
	}

	void insert(const T &item) {
		ASSERT(_size < _maxSize);
		u32 it = 0;
		while (it < _size && _lessThan(_array[it], item))
			++it;
		if (it == _size)
			_array[_size++] = item;
		else {
			T save = _array[it];
			_array[it] = item;
			while (it < _size) {
				++it;
				T swap = _array[it];
				_array[it] = save;
				save = swap;
			}
			++_size;
		}
	}

	void remove(u32 index) {
		while (index < _size) {
			_array[index] = _array[index + 1];
			++index;
		}
		--_size;
	}

protected:
	T *_array;
	u32 _size, _maxSize;
	LessThanPredicate _lessThan;
};

#endif

