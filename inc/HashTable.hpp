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

// Note this is not in fact a hash table, just an associative array.
// We should make it a hash table in the future, though.
// TODO: we need to actually make a decent lookup mechanism for this. Currently very slow.

template <typename K, typename V>
class HashTable {
	public:
		HashTable(): head(0) {
		}

		~HashTable() {
			_InternalItem *traverse = head, *next;
			while (traverse) {
				next = traverse->next;
				delete traverse;
				traverse = next;
			}
		}

		const u32 size() const {
			_InternalItem *traverse = head;
			u32 s = 0;

			while (traverse) {
				traverse = traverse->next;
				++s;
			}

			return s;
		}

		bool hasKey(const K &key) const {
			_InternalItem *traverse = head;
			while (traverse) {
				if (traverse->key == key) {
					return true;
				}
				traverse = traverse->next;
			}
			return false;
		}

		const V &operator[](const K &key) const {
			_InternalItem *traverse = head;
			while (traverse) {
				if (traverse->key == key)
					return traverse->value;
				traverse = traverse->next;
			}
			return V();
		}

		V &operator[](const K &key) {
			_InternalItem **traverse = &head;
			while (*traverse) {
				if ((*traverse)->key == key)
					return (*traverse)->value;

				traverse = &(*traverse)->next;
			}

			_InternalItem *add = new _InternalItem();
			add->key = key;
			add->value = V();
			add->next = 0;
			*traverse = add;
			
			return add->value;
		}

	protected:
		struct _InternalItem {
			public:
				K key;
				V value;
				_InternalItem *next;
		};

		_InternalItem *head;
};

#endif

