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
#include <List.hpp>
#include <debug.hpp>

#include "VFSProto.hpp"
#include "main.hpp"

pid_t pidForDriver(u32 driver);
u32 fs_read(pid_t pid, u32 inode, u32 offset, u32 length, u8 *buffer);
VFSDirent *fs_readdir(pid_t pid, u32 inode, u32 index);

LinkedList<VFSDriver> drivers;
VFSNode *vfs_root = 0;

extern "C" int start() {
	{
		// Requests to driver 0 will come back here.
		VFSDriver null_driver;
		syscall_strcpy(null_driver.name, "null driver");
		null_driver.pid = syscall_processId();
		drivers.push_back(null_driver);
	}

	if (!syscall_registerName("system.io.vfs"))
		syscall_panic("VFS: could not register system.io.vfs");

	printf("[VFS] ");

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();
		u8 *request = syscall_grabQueue(&info);
		syscall_shiftQueue(&info);

		printf("VFS: got request 0x%x\n", request[0]);

		if (request[0] == VFS_OP_READ) {
			VFSOpRead *op = reinterpret_cast<VFSOpRead *>(request);

			u8 *buffer = new u8[op->length];
			u32 bytes_read = fs_read(pidForDriver(vfs_root->driver), op->inode, op->offset, op->length, buffer);
			syscall_sendQueue(info.from, info.id, buffer, bytes_read);
			delete [] buffer;
		} else if (request[0] == VFS_OP_READDIR) {
			VFSOpReaddir *op = reinterpret_cast<VFSOpReaddir *>(request);

			VFSDirent *dirent = fs_readdir(pidForDriver(vfs_root->driver), op->inode, op->index);
			syscall_sendQueue(info.from, info.id, reinterpret_cast<u8 *>(dirent), sizeof(VFSDirent));
			delete dirent;
		} else if (request[0] == VFS_OP_REGISTER_DRIVER) {
			VFSOpRegisterDriver *op = reinterpret_cast<VFSOpRegisterDriver *>(request);

			// Figure out how many bytes are at the end of the message for the name.
			u32 name_len = info.data_len - (reinterpret_cast<u32>(&op->name) - reinterpret_cast<u32>(op));

			VFSDriver driver;

			syscall_memcpy(&driver.name, op->name, name_len);
			driver.name[name_len] = 0;

			driver.pid = info.from;

			drivers.push_back(driver);

			printf("VFS: registered '%s' driver\n", driver.name);

			VFSReplyRegisterDriver reply = { true, drivers.length() - 1 };
			syscall_sendQueue(info.from, info.id, reinterpret_cast<u8 *>(&reply), sizeof(reply));
		} else if (request[0] == VFS_OP_MOUNT_ROOT) {
			VFSOpMountRoot *op = reinterpret_cast<VFSOpMountRoot *>(request);

			if (vfs_root) syscall_panic("VFS: already have a root!");

			vfs_root = new VFSNode;
			syscall_strcpy(vfs_root->name, "root (/)");
			vfs_root->driver = op->driver;
			vfs_root->inode = op->inode;
			
			syscall_sendQueue(info.from, info.id, reinterpret_cast<const u8 *>("\1"), 1);
		} else {
			syscall_panic("VFS: confused");
		}
	}

	syscall_panic("VFS: went off the edge");
	return 1;
}

pid_t pidForDriver(u32 driver) {
	return drivers[driver].pid;
}

u32 fs_read(pid_t pid, u32 inode, u32 offset, u32 length, u8 *buffer) {
	VFSOpRead op = {
		VFS_OP_READ,
		inode,
		offset,
		length
	};

	u32 msg_id = syscall_sendQueue(pid, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpRead));

	struct queue_item_info *info = syscall_probeQueueFor(msg_id);
	if (info->data_len > length) syscall_panic("VFS: fs read not expected number of bytes back?");

	u32 len = info->data_len;
	syscall_readQueue(info, buffer, 0, len);
	syscall_shiftQueue(info);

	return len;
}

VFSDirent *fs_readdir(pid_t pid, u32 inode, u32 index) {
	VFSOpReaddir op = {
		VFS_OP_READDIR,
		inode,
		index
	};

	u32 msg_id = syscall_sendQueue(pid, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpReaddir));

	struct queue_item_info *info = syscall_probeQueueFor(msg_id);
	if (info->data_len != sizeof(VFSDirent)) syscall_panic("VFS: fs read not expected number of bytes back?");

	VFSDirent *dirent = new VFSDirent;
	syscall_readQueue(info, reinterpret_cast<u8 *>(dirent), 0, info->data_len);
	syscall_shiftQueue(info);
	return dirent;
}
