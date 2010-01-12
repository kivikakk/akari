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
#include <POSIX.hpp>
#include <Akari.hpp>
#include <Console.hpp>

Symbol::Symbol(): _content(0)
{ }

Symbol::Symbol(const char *content): _content(content)
{ }

bool Symbol::operator !() const {
	return !_content;
}

bool Symbol::operator ==(const Symbol &r) const {
	// If we've got no content, then their having no content is equal.
	// If they've got no content (we already know we do), we're inequal.
	if (!_content) return !r._content;
	if (!r._content) return false;

	return (POSIX::strcmp(_content, r._content) == 0);
}

bool Symbol::operator !=(const Symbol &r) const {
	if (!_content) return !!r._content;
	if (!r._content) return true;

	return (POSIX::strcmp(_content, r._content) != 0);
}

