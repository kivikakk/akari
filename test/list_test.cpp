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

#include <cstdio>

typedef unsigned long u32;

#include "list"

typedef std::list<int> list_type;
list_type my_list;

void report() {
	printf("my_list(%u): %s ", (unsigned int)my_list.size(), my_list.empty() ? "empty" : "itemful");
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

	my_list.clear();
	report();

	my_list.push_back(1);
	my_list.push_back(2);
	report();

	my_list.pop_front();
	my_list.pop_front();
	report();

	for (int i = 10; i >= 0; i--)
		my_list.push_back(i);
	report();

	my_list.erase(my_list.begin());
	report();

	my_list.erase(++++my_list.begin());
	report();

	my_list.remove(5);
	my_list.remove(0);
	report();

	my_list.erase(my_list.begin());
	my_list.erase(my_list.begin());
	my_list.erase(my_list.begin());
	my_list.erase(my_list.begin());
	my_list.erase(my_list.begin());
	my_list.erase(my_list.begin());
	my_list.erase(my_list.begin());
	report();

	my_list.push_back(2);
	report();
	my_list.push_front(0);
	report();
	my_list.erase(my_list.rbegin());
	report();
	my_list.erase(my_list.begin());
	report();
}
