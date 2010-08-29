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

#include <stdio.hpp>
#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>
#include <slist>
#include <vector>
#include <debug.hpp>

#include "NetProto.hpp"
#include "main.hpp"


extern "C" int main() {
	if (!registerName("system.io.net"))
		panic("Net: could not register system.io.net");

	printf("Net: started\n");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == NET_OP_NOOP) {
			NetOpNoop *op = reinterpret_cast<NetOpNoop *>(request);

			// u8 *buffer = new u8[op->length];
			// sendQueue(info.from, info.id, buffer, bytes_read);
			// delete [] buffer;
		} else {
			panic("Net: confused");
		}
	}
}

