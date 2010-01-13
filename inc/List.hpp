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

#ifndef __LIST_HPP__
#define __LIST_HPP__

template <typename T>
class LinkedList {
	public:
		struct listNode {
			T *item;
			listNode *next;
		};

		class iterator {
			friend class LinkedList;
			public:
				bool operator ==(const iterator &r) const {
					return _data == r._data;
				}

				bool operator !=(const iterator &r) const {
					return _data != r._data;
				}

				T &operator *() {
					 return *_data->item;
				}

				const T &operator *() const {
					 return *_data->item;
				}

				T *&operator ->() {
					return _data->item;
				}

				T *const &operator ->() const {
					return _data->item;
				}

				iterator next() const {
					return iterator(_data->next);
				}

				iterator &operator ++() {
					_data = _data->next;
					return *this;
				}

			protected:
				iterator(listNode *data) {
					_data = data;
				}

				listNode *_data;
		};

		LinkedList(): _begin(0) {
		}

		iterator begin() {
			return iterator(_begin);
		}

		const iterator begin() const {
			return iterator(_begin);
		}

		iterator end() {
			return iterator(0);
		}

		const iterator end() const {
			return iterator(0);
		}

		void push_back(const T &data) {
			listNode **writeHead = &_begin;
			while (*writeHead) {
				writeHead = &(*writeHead)->next;
			}

			// XXX: is this vaguely copying-ish behaviour
			// correct?  Depending on whether T is `type`
			// or `type*`, we'll see...
			listNode *newNode = new listNode;
			newNode->item = new T(data);
			newNode->next = 0;

			*writeHead = newNode;
		}

		void shift() {
			listNode *old = _begin;
			_begin = _begin->next;
			delete old->item;
			delete old;
		}

		bool empty() const {
			return !_begin;
		}

	protected:
		listNode *_begin;
};

#endif

