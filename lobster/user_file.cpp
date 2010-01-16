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

#include <user_file.hpp>
#include <stream.hpp>
#include <process.hpp>
#include <screen.hpp>
#include <memory.hpp>
#include <panic.hpp>

int user_open(const char *path, int oflag) {
	// first thing's first - resolve the path to a node
	fs_node_t *path_node = resolve_fs_node(path);
	if (!path_node)	// oh dear
		return -1;		// TODO proper error codes - Exceptions?! - across kern/user boundaries? hmm.

	int i;
	for (i = 0; i < 256; ++i) {
		stream_t *stream = current_task->streams[i];
		if (!stream) {
			stream = current_task->streams[i] = (stream_t *)kmalloc(sizeof(stream_t));
			stream->type = stream_t::PHYSICAL;
			stream->physical.offset = 0;
			stream->physical.node = path_node;
			return i;
		}
	}
	return -1;
}

int user_read(int fildes, void *buf, unsigned long nbyte) {
	stream_t *stream = current_task->streams[fildes];
	if (!stream)
		return -1;		// TODO
	
	switch (stream->type) {
		case stream_t::PHYSICAL: {
			unsigned long returned = read_fs(stream->physical.node, stream->physical.offset, nbyte, (unsigned char *)buf);
			stream->physical.offset += returned;
			return returned;
		}
		default:
			panic("user_read not implemented for this stream type\n");
	}
}

int user_close(int fildes) {
	stream_t *stream = current_task->streams[fildes];
	if (!stream)
		return -1;		// TODO
	
	switch (stream->type) {
		case stream_t::PHYSICAL: kfree(stream->physical.node); break;
		default: break;
	}

	kfree(stream);
	current_task->streams[fildes] = 0;
	return 0;
}

