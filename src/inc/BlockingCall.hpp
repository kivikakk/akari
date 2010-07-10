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

#ifndef __BLOCKING_CALL_HPP__
#define __BLOCKING_CALL_HPP__

#include <arch.hpp>
#include <Symbol.hpp>

class BlockingCall {
public:
	BlockingCall();
	virtual ~BlockingCall();

	bool shallBlock() const;

	virtual u32 operator ()() = 0;
	virtual bool unblockWith(u32 data) const;
	virtual Symbol insttype() const = 0;

protected:
	void _wontBlock();
	void _willBlock();

private:
	enum {
		UNSET, WILL, WONT
	} _shallBlock;
};

#endif

