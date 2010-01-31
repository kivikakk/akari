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
#include <UserCalls.hpp>

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
	if (pos > _length) {
		// TODO: panic("out_of_range exception (according to STL)");
		return std::string();
	}
	if (pos + n > _length) n = _length - pos;

	return std::string(_str + pos, n);
}

std::string std::string::trim() const {
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

void *memset(void *mem, u8 c, u32 n) {
	u8 *m = static_cast<u8 *>(mem);
	while (n--)
		*m++ = c;
	return mem;
}

void *memcpy(void *dest, const void *src, u32 n) {
	const u8 *s = static_cast<const u8 *>(src);
	u8 *d = static_cast<u8 *>(dest);
	while (n--)
		*d++ = *s++;
	return dest;
}

u32 strlen(const char *s) {
	u32 n = 0;
	while (*s++)
		++n;
	return n;
}

s32 strcmp(const char *s1, const char *s2) {
	while (*s1 && *s2) {
		if (*s1 < *s2) return -1;
		if (*s1 > *s2) return 1;
		++s1, ++s2;
	}
	// One or both may be NUL.
	if (*s1 < *s2) return -1;
	if (*s1 > *s2) return 1;
	return 0;
}

s32 strcmpn(const char *s1, const char *s2, int n) {
	while (*s1 && *s2 && n > 0) {
		if (*s1 < *s2) return -1;
		if (*s1 > *s2) return 1;
		++s1, ++s2;
		--n;
	}
	if (n == 0) return 0;
	// One or both may be NUL.
	if (*s1 < *s2) return -1;
	if (*s1 > *s2) return 1;
	return 0;
}

s32 strpos(const char *haystack, const char *needle) {
	s32 i = 0;
	s32 hl = (s32)strlen(haystack),
		nl = (s32)strlen(needle);
	s32 d = hl - nl;
	while (i <= d) {
		if (strcmpn(haystack, needle, nl) == 0)
			return i;
		++i, ++haystack;
	}
	return -1;
}

bool isspace(char c) {
	return !(c < 9 || (c > 13 && c != 32));
}

char tolower(char c) {
	if (c < 'A' || c > 'Z') return c;
	return c + ('a' - 'A');
}

char toupper(char c) {
	if (c < 'a' || c > 'z') return c;
	return c - ('a' - 'A');
}

s32 stricmp(const char *s1, const char *s2) {
	while (*s1 && *s2) {
		if (toupper(*s1) < toupper(*s2)) return -1;
		if (toupper(*s1) > toupper(*s2)) return 1;
		++s1, ++s2;
	}
	// One or both may be NUL.
	if (toupper(*s1) < toupper(*s2)) return -1;
	if (toupper(*s1) > toupper(*s2)) return 1;
	return 0;
}

char *strcpy(char *dest, const char *src) {
	char *orig = dest;
	while (*src)
		*dest++ = *src++;
	*dest = 0;
	return orig;
}

char *strdup(const char *src) {
	char *result = new char[strlen(src) + 1];
	strcpy(result, src);
	return result;
}
