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

#ifndef __STRINGS_HPP__
#define __STRINGS_HPP__

#include <arch.hpp>

// TODO in the future: COW?
class ASCIIString {
	public:
		ASCIIString();
		ASCIIString(const ASCIIString &);
		ASCIIString(const char *&);
		~ASCIIString();
		
		ASCIIString &operator =(const ASCIIString &);
		ASCIIString &operator =(const char *&);

		bool operator ==(const ASCIIString &) const;
		bool operator ==(const char *) const;

		bool operator !() const;

		char operator[](u32 n) const;
		char &operator[](u32 n);

		bool empty() const;
		u32 length() const;
		const char *getCString() const;

	protected:
		char *_data;
		u32 _dataLength;
};

#endif

