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

#include <stdio.hpp>
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
VFSNode *fs_finddir(pid_t pid, u32 inode, const char *name);

LinkedList<VFSDriver> drivers;
VFSNode *vfs_root = 0;

extern "C" int start() {
	{
		// Requests to driver 0 will come back here.
		VFSDriver null_driver;
		strcpy(null_driver.name, "null driver");
		null_driver.pid = processId();
		drivers.push_back(null_driver);
	}

	if (!registerName("system.io.vfs"))
		panic("VFS: could not register system.io.vfs");

	printf("[VFS] ");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		printf("VFS: got request 0x%x\n", request[0]);

		if (request[0] == VFS_OP_READ) {
			VFSOpRead *op = reinterpret_cast<VFSOpRead *>(request);

			u8 *buffer = new u8[op->length];
			u32 bytes_read = fs_read(pidForDriver(vfs_root->driver), op->inode, op->offset, op->length, buffer);
			sendQueue(info.from, info.id, buffer, bytes_read);
			delete [] buffer;
		} else if (request[0] == VFS_OP_READDIR) {
			VFSOpReaddir *op = reinterpret_cast<VFSOpReaddir *>(request);

			VFSDirent *dirent = fs_readdir(pidForDriver(vfs_root->driver), op->inode, op->index);
			if (!dirent) {
				sendQueue(info.from, info.id, 0, 0);
			} else {
				sendQueue(info.from, info.id, reinterpret_cast<u8 *>(dirent), sizeof(VFSDirent));
				delete dirent;
			}
		} else if (request[0] == VFS_OP_FINDDIR) {
			VFSOpFinddir *op = reinterpret_cast<VFSOpFinddir *>(request);

			VFSNode *node = fs_finddir(pidForDriver(vfs_root->driver), op->inode, op->name);
			if (!node) {
				sendQueue(info.from, info.id, 0, 0);
			} else {
				sendQueue(info.from, info.id, reinterpret_cast<u8 *>(node), sizeof(VFSNode));
				delete node;
			}
		} else if (request[0] == VFS_OP_REGISTER_DRIVER) {
			VFSOpRegisterDriver *op = reinterpret_cast<VFSOpRegisterDriver *>(request);

			VFSDriver driver;
			strcpy(driver.name, op->name);
			driver.pid = info.from;

			drivers.push_back(driver);

			printf("VFS: registered '%s' driver\n", driver.name);

			VFSReplyRegisterDriver reply = { true, drivers.length() - 1 };
			sendQueue(info.from, info.id, reinterpret_cast<u8 *>(&reply), sizeof(reply));
		} else if (request[0] == VFS_OP_MOUNT_ROOT) {
			VFSOpMountRoot *op = reinterpret_cast<VFSOpMountRoot *>(request);

			if (vfs_root) panic("VFS: already have a root!");

			vfs_root = new VFSNode;
			strcpy(vfs_root->name, "root (/)");
			vfs_root->driver = op->driver;
			vfs_root->inode = op->inode;
			
			sendQueue(info.from, info.id, reinterpret_cast<const u8 *>("\1"), 1);
		} else {
			panic("VFS: confused");
		}
	}

	panic("VFS: went off the edge");
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

	u32 msg_id = sendQueue(pid, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpRead));

	struct queue_item_info *info = probeQueueFor(msg_id);
	if (info->data_len > length) panic("VFS: fs read not expected number of bytes back?");

	u32 len = info->data_len;
	readQueue(info, buffer, 0, len);
	shiftQueue(info);

	return len;
}

VFSDirent *fs_readdir(pid_t pid, u32 inode, u32 index) {
	VFSOpReaddir op = {
		VFS_OP_READDIR,
		inode,
		index
	};

	u32 msg_id = sendQueue(pid, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpReaddir));

	struct queue_item_info *info = probeQueueFor(msg_id);
	if (info->data_len == 0) {
		shiftQueue(info);
		return 0;
	}
	if (info->data_len != sizeof(VFSDirent)) panic("VFS: fs read not expected number of bytes back?");

	VFSDirent *dirent = new VFSDirent;
	readQueue(info, reinterpret_cast<u8 *>(dirent), 0, info->data_len);
	shiftQueue(info);
	return dirent;
}

VFSNode *fs_finddir(pid_t pid, u32 inode, const char *name) {
	u32 cmd_len = sizeof(VFSOpFinddir) + strlen(name) + 1;
	VFSOpFinddir *op = reinterpret_cast<VFSOpFinddir *>(malloc(cmd_len));
	op->cmd = VFS_OP_FINDDIR;
	op->inode = inode;
	strcpy(op->name, name);

	u32 msg_id = sendQueue(pid, 0, reinterpret_cast<u8 *>(op), cmd_len);
	delete op;

	struct queue_item_info *info = probeQueueFor(msg_id);
	if (info->data_len == 0) {
		shiftQueue(info);
		return 0;
	}
	if (info->data_len != sizeof(VFSNode)) panic("VFS: fs read not expected number of bytes back?");

	VFSNode *node = new VFSNode;
	readQueue(info, reinterpret_cast<u8 *>(node), 0, info->data_len);
	shiftQueue(info);
	return node;
}

