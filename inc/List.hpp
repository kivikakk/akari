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

				iterator next() const {
					return iterator(_data->next);
				}

				iterator operator ++() const {
					return next();
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

			listNode *newNode = new listNode;
			newNode->item = new T(data);
			newNode->next = 0;

			*writeHead = newNode;
		}

	protected:
		listNode *_begin;
};

#endif

