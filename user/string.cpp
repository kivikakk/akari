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

#include <string>
#include <stdio.hpp>
#include <UserCalls.hpp>

bool isspace(char c) {
	return !(c < 9 || (c > 13 && c != 32));
}

std::string::string(): _str(0), _length(0)
{ }

std::string::string(const std::string &r): _length(r._length) {
	_str = new char[_length + 1];
	memcpy(_str, r._str, _length);
	_str[_length] = 0;
}

std::string::string(const char *s) {
	_length = strlen(s);
	_str = new char[_length + 1];
	strcpy(_str, s);
}

std::string::string(const char *s, u32 n): _length(n) {
	_str = new char[_length + 1];
	memcpy(_str, s, _length);
	_str[_length] = 0;
}

std::string::~string() {
	if (_str) {
		delete [] _str;
	}
}

bool std::string::operator ==(const char *c) const {
	if (!_length) {
		if (!c || *c == 0) return true;
		// c has content, our length is 0
		return false;
	}

	// _length > 0
	if (!c || *c == 0) return false;

	return (strcmp(_str, c) == 0);
}

const char &std::string::operator[](u32 pos) const {
	return _str[pos];
}

char &std::string::operator[](u32 pos) {
	return _str[pos];
}

u32 std::string::length() const {
	return _length;
}

const char *std::string::c_str() const {
	return _str;
}

std::string std::string::substr(u32 pos, u32 n) const {
	if (pos > _length) panic("out_of_range exception (according to STL)");
	if (pos + n > _length) n = _length - pos;

	return std::string(_str + pos, n);
}

std::string std::string::trim() const {
	printf("trim starting with %x '%s'\n", _str, _str);

	u32 start_index = 0;
	while (start_index < _length && isspace(_str[start_index]))
		++start_index;
	
	if (start_index == _length)
		return std::string();

	u32 end_index = _length - 1;
	while (isspace(_str[end_index]))
		--end_index;
	
	return std::string(_str + start_index, end_index - start_index + 1);
}

std::vector<std::string> std::string::split() const {
	std::string base = trim();
	std::vector<std::string> result;

	u32 length = base.length(), offset = 0;
	while (offset < length) {
		u32 index = 0;
		while ((offset + index) < length && !isspace(base[offset + index]))
			++index;
		result.push_back(base.substr(offset, index));

		while ((offset + index) < length && isspace(base[offset + index]))
			++index;

		offset += index;
	}

	return result;
}

