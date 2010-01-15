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
// along with Akari.  If not, see <http://www.gnu.org/licenses/

#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>
#include <debug.hpp>

#include "VFSProto.hpp"
#include "main.hpp"

u32 fat_read(u32 inode, u32 offset, u32 length, char *buffer);
VFSDirent *fat_readdir(u32 inode, u32 index);

pid_t fat = 0;

typedef struct {
	pid_t driver;
	u32 inode;
} VFSReference;

VFSReference *vfs_root;

extern "C" int start() {
	syscall_puts("VFS: waiting for fat\n");
	while (!fat)
		fat = syscall_processIdByName("system.io.fs.fat");

	vfs_root = new VFSReference;
	vfs_root->driver = fat;
	vfs_root->inode = 0;

	if (!syscall_registerName("system.io.vfs"))
		syscall_panic("VFS: could not register system.io.vfs");

	syscall_puts("VFS: entering loop\n");

	char *buffer = 0; u32 buffer_len = 0;

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();
		syscall_puts("VFS: got request\n");

		u32 len = info.data_len;
		if (len > VFS_MAX_WILL_ALLOC) syscall_panic("VFS: given more data than would like to alloc");
		if (len > buffer_len) {
			if (buffer) delete [] buffer;
			buffer = new char[len];
			buffer_len = len;
		}

		syscall_readQueue(&info, buffer, 0, len);
		syscall_shiftQueue(&info);

		if (buffer[0] == VFS_OP_READ) {
			VFSOpRead op = *reinterpret_cast<VFSOpRead *>(buffer);

			if (op.length > VFS_MAX_WILL_ALLOC) syscall_panic("VFS: request asked for more than we'd like to alloc");
			if (op.length > buffer_len) {
				delete [] buffer;
				buffer = new char[op.length];
				buffer_len = op.length;
			}

			u32 bytes_read = fat_read(op.inode, op.offset, op.length, buffer);
			syscall_sendQueue(info.from, info.id, buffer, bytes_read);
		} else if (buffer[0] == VFS_OP_READDIR) {
			VFSOpReaddir op = *reinterpret_cast<VFSOpReaddir *>(buffer);

			VFSDirent *dirent = fat_readdir(op.inode, op.index);
			syscall_sendQueue(info.from, info.id, reinterpret_cast<char *>(dirent), sizeof(VFSDirent));
			delete dirent;
		} else {
			syscall_panic("VFS: confused");
		}
	}

	syscall_panic("VFS: went off the edge");
	return 1;
}

u32 fat_read(u32 inode, u32 offset, u32 length, char *buffer) {
	VFSOpRead op = {
		VFS_OP_READ,
		inode,
		offset,
		length
	};

	u32 msg_id = syscall_sendQueue(fat, 0, reinterpret_cast<char *>(&op), sizeof(VFSOpRead));

	struct queue_item_info *info = syscall_probeQueueFor(msg_id);
	if (info->data_len > length) syscall_panic("VFS: FAT read not expected number of bytes back?");

	u32 len = info->data_len;
	syscall_readQueue(info, buffer, 0, len);
	syscall_shiftQueue(info);

	return len;
}

VFSDirent *fat_readdir(u32 inode, u32 index) {
	VFSOpReaddir op = {
		VFS_OP_READDIR,
		inode,
		index
	};

	u32 msg_id = syscall_sendQueue(fat, 0, reinterpret_cast<char *>(&op), sizeof(VFSOpReaddir));

	struct queue_item_info *info = syscall_probeQueueFor(msg_id);
	if (info->data_len != sizeof(VFSDirent)) syscall_panic("VFS: FAT read not expected number of bytes back?");

	VFSDirent *dirent = new VFSDirent;
	syscall_readQueue(info, reinterpret_cast<char *>(dirent), 0, info->data_len);
	syscall_shiftQueue(info);
	return dirent;
}
