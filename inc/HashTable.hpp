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

#ifndef __HASH_TABLE_HPP__
#define __HASH_TABLE_HPP__

#include <Akari.hpp>
#include <Console.hpp>
#include <list>

// Note this is not in fact a hash table, just an associative array.
// We should make it a hash table in the future, though.
// TODO: we need to actually make a decent lookup mechanism for this. Currently very slow.

template <typename K, typename V>
class HashTable {
	public:
		HashTable()
		{ }

		~HashTable()
		{ }

		bool hasKey(const K &key) const {
			for (typename std::list<_InternalItem>::iterator it = items.begin(); it != items.end(); ++it) {
				if (key == it->key)
					return true;
			}
			return false;
		}

		const V &operator[](const K &key) const {
			for (const typename std::list<_InternalItem>::iterator it = items.begin(); it != items.end(); ++it) {
				if (key == it->key)
					return it->value;
			}
			return V();
		}

		V &operator[](const K &key) {
			for (typename std::list<_InternalItem>::iterator it = items.begin(); it != items.end(); ++it) {
				if (key == it->key)
					return it->value;
			}

			_InternalItem add = { key, V() };
			add.key = key;
			add.value = V();
			
			return items.push_back(add).value;
		}

	protected:
		struct _InternalItem {
			K key;
			V value;
		};

		std::list<_InternalItem> items;
};

#endif

