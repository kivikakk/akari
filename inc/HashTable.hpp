#ifndef __HASH_TABLE_HPP__
#define __HASH_TABLE_HPP__

// TODO: we need to actually make a decent lookup mechanism for this. Currently very slow.

template <typename K, typename V>
class HashTable {
	public:
		HashTable(): head(0) {
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

		const V &operator[](const K &index) const {
			_InternalItem *traverse = head;
			while (traverse) {
				if (traverse->key == index)
					return traverse->value;
				traverse = traverse->next;
			}
			return K();
		}

		V &operator[](const K &index) {
			_InternalItem **traverse = &head;
			while (*traverse) {
				if ((*traverse)->key == index)
					return (*traverse)->value;

				traverse = &(*traverse)->next;
			}

			_InternalItem *add = new _InternalItem();
			add->key = index;
			add->value = K();
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

