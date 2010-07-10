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

#include <Symbol.hpp>
#include <Akari.hpp>
#include <string>
#include <Console.hpp>

Symbol::Symbol(): _content(0)
{ }

// Every symbol has its own copy of the string.  Don't just use the pointer
// assuming it won't change - it might be from userspace for all you know.
Symbol::Symbol(const char *content): _content(strdup(content))
{ }

// Unfortunately we can't just use the other Symbol's pointer, even though we
// know they have their own copy, because we don't know when it'll die, and I
// don't want to do reference counting.
Symbol::Symbol(const Symbol &symbol): _content(strdup(symbol._content))
{ }

Symbol::~Symbol() {
	if (_content)
		delete [] _content;
}

Symbol &Symbol::operator =(const Symbol &symbol) {
	if (_content)
		delete [] _content;
	_content = strdup(symbol._content);
	return *this;
}

bool Symbol::operator !() const {
	return !_content;
}

bool Symbol::operator ==(const Symbol &r) const {
	// If we've got no content, then their having no content is equal.
	// If they've got no content (we already know we do), we're inequal.
	if (!_content) return !r._content;
	if (!r._content) return false;

	return (strcmp(_content, r._content) == 0);
}

bool Symbol::operator !=(const Symbol &r) const {
	if (!_content) return !!r._content;
	if (!r._content) return true;

	return (strcmp(_content, r._content) != 0);
}

const char *Symbol::c_str() const {
	return _content;
}
