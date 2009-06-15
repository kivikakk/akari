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
			public:
				bool operator ==(const iterator &r) const {
					return _data == r._data;
				}

				T &operator *() {
					 return *_data->item;
				}

				iterator next() const {
					return iterator(_data->next);
				}

			protected:
				iterator(listNode *data) {
					_data = data;
				}

				listNode *_data;
		};

		LinkedList(): _start(0) {
		}

		iterator start() {
			return iterator(_start);
		}

		iterator end() {
			return iterator(0);
		}

		void push_back(const T &data) {
			listNode **writeHead = &_start;
			while (*writeHead) {
				writeHead = &(*writeHead)->next;
			}

			listNode *newNode = new listNode;
			newNode->item = new T(data);
			newNode->next = 0;

			*writeHead = newNode;
		}

	protected:
		listNode *_start;
};

#endif

