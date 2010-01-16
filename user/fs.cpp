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

#include <fs.hpp>
#include <stdio.hpp>
#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>

static pid_t vfs = 0;
static VFSNode *root = 0;

static VFSNode *finddir_fs(VFSNode *node, const char *component);
static VFSNode *resolve_path(const char *path);

static void init() {
	while (!vfs)
		vfs = processIdByName("system.io.vfs");

	if (!root) {
		VFSOpRoot op = { VFS_OP_ROOT };
		u32 msg = sendQueue(vfs, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpRoot));
		struct queue_item_info *info = probeQueueFor(msg);
		if (info->data_len == 0) {
			printf("fs.cpp: no root!?\n");
			panic("oh no!");
		}
		root = new VFSNode;
		readQueue(info, reinterpret_cast<u8 *>(root), 0, info->data_len);
		shiftQueue(info);
	}
}

DIR *opendir(const char *dirname) {
	init();

	DIR *dirp = new DIR;
	dirp->dir = resolve_path(dirname);

	return dirp;
}

int closedir(DIR *dirp) {
	delete dirp->dir;
	delete dirp;

	return 0;
}

static VFSNode *finddir_fs(VFSNode *node, const char *component) {
	u32 cmd_len = sizeof(VFSOpFinddir) + strlen(component) + 1;
	
	VFSOpFinddir *op = reinterpret_cast<VFSOpFinddir *>(malloc(cmd_len));
	op->cmd = VFS_OP_FINDDIR;
	op->inode = node->inode;
	strcpy(op->name, component);

	u32 msg = sendQueue(vfs, 0, reinterpret_cast<u8 *>(op), cmd_len);
	struct queue_item_info *info = probeQueueFor(msg);
	if (info->data_len == 0) {
		shiftQueue(info);
		return 0;
	}
	VFSNode *ret = new VFSNode;
	readQueue(info, reinterpret_cast<u8 *>(ret), 0, info->data_len);
	shiftQueue(info);

	return ret;
}

// Always returns a new VFSNode object, even if it's a copy
// of the root.
static VFSNode *resolve_path(const char *path) {
	VFSNode *node = new VFSNode(*root);
	static char component[128];

	while (*path)
		if (*path == '/')
			++path;
		else {
			char *component_writer = component;
			while (*path && *path != '/')
				*component_writer++ = *path++;
			*component_writer = 0;

			// resolve `component' in the context of `node'
			VFSNode *old = node;
			node = finddir_fs(old, component);
			delete old;

			if (!node)
				return 0;
		}
	
	return node;
}

