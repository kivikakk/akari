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

#include <BlockingCall.hpp>
#include <debug.hpp>

BlockingCall::BlockingCall(): _shallBlock(UNSET) { }
BlockingCall::~BlockingCall() { }

bool BlockingCall::shallBlock() const {
	if (_shallBlock == UNSET)
		AkariPanic("_shallBlock UNSET");
	return _shallBlock == WILL;
}

bool BlockingCall::unblockWith(u32 data) const {
	AkariPanic("unblockWith not implemented");
	return false;
}

void BlockingCall::_wontBlock() {
	_shallBlock = WONT;
}

void BlockingCall::_willBlock() {
	_shallBlock = WILL;
}

