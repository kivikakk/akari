#ifndef __HOLY_ARRAY_HPP__
#define __HOLY_ARRAY_HPP__

// An array which you can poke holes out of. Fun!
// When you do, that index won't be used again, but won't take up
// the space. That's good, right?

#include <arch.hpp>
#include <HashTable.hpp>

template <typename T>
class HolyArray {
	public:
		HolyArray(u32 initialSize): _lastIndex(0) {
			ASSERT(initialSize > 0);

			if (initialSize % 32 != 0) {
				initialSize += (32 - initialSize % 32);
			}

			_indexMap = new HashTable<u32, u32>();
			_usageMap = new u32[initialSize / 32];
			_items = new T*[initialSize];

			_currentSize = initialSize;
		}

		~HolyArray() {
			delete _indexMap;
			delete _usageMap;
			delete _items;
		}

		const T &operator[](u32 index) const {
			ASSERT(_indexMap->hasKey(index));
			u32 transformedIndex = (*_indexMap)[index];
			return *_items[transformedIndex];
		}

		T &operator[](u32 index) {
			ASSERT(_indexMap->hasKey(index));
			u32 transformedIndex = (*_indexMap)[index];
			return *_items[transformedIndex];
		}

		u32 insert(const T &item) {
			u32 index = _lastIndex++;
			insertAtIndex(item, index);
			return index;
		}

		void insertAtIndex(const T &item, u32 index) {
			while (true) {
				for (u32 i = 0; i < _currentSize / 32; ++i) {
					if (_usageMap[i] != 0xFFFFFFFF) {
						// There's a gap.
						for (u32 j = 0; j < 32; ++j) {
							if ((_usageMap[i] & (1 << j)) == 0) {
								_usageMap[i] |= (1 << j);
								_items[i * 32 + j] = new T(item);
								(*_indexMap)[index] = i * 32 + j;
								return;
							}
						}
					}
				}
				// Need to resize! Doubling is the way we seem to want to do this.
				_currentSize <<= 1;
				u32 *newUsageMap = new u32[_currentSize / 32];
				POSIX::memcpy(newUsageMap, _usageMap, _currentSize / 32 / 2);
				POSIX::memset((void *)((u32)newUsageMap + _currentSize / 32 / 2), 0, _currentSize / 32 / 2);
				T **newItems = new T*[_currentSize];
				POSIX::memcpy(newItems, _items, _currentSize / 2);
				POSIX::memset((void *)((u32)newItems + _currentSize / 2), 0, _currentSize / 2);

				delete [] _usageMap;
				delete [] _items;

				_usageMap = newUsageMap;
				_items = newItems;

				// Try again.
			}
		}

		void remove(u32 index) {
			u32 transformedIndex = (*_indexMap)[index];
			_usageMap[transformedIndex / 32] &= ~(1 << (transformedIndex % 32));
			delete _items[transformedIndex];
		}

	protected:
		HashTable<u32, u32> *_indexMap;
		u32 *_usageMap;
		T **_items;
		u32 _lastIndex, _currentSize;
};

#endif

