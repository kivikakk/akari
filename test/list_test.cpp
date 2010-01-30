#include <cstdio>

typedef unsigned long u32;

#include "list"

typedef std::list<int> list_type;
list_type my_list;

void report() {
	printf("my_list(%u): ", (unsigned int)my_list.size());
	for (list_type::iterator it = my_list.begin(); it != my_list.end(); ++it) {
		if (it != my_list.begin())
			printf(", ");
		printf("%d", *it);
	}
	printf("\n");
}

int main() {
	my_list.push_back(4);
	my_list.push_back(3);
	my_list.push_back(2);
	my_list.push_back(1);
	report();

	my_list.insert(my_list.begin(), 1);
	report();

	my_list.insert(++++my_list.begin(), 2);
	report();

	my_list.insert(++++++++my_list.begin(), 3);
	report();

	my_list.insert(++++++++++++my_list.begin(), 4);
	report();

	my_list.insert(my_list.end(), 0);
	report();

	my_list.insert(my_list.begin(), 0);
	report();
}
