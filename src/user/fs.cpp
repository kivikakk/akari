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

static u32 read_fs(VFSNode *node, u32 offset, u32 length, void *buffer);
static VFSNode *finddir_fs(VFSNode *node, const char *component);
static VFSDirent *readdir_fs(VFSNode *node, u32 index);
static VFSNode *resolve_path(const char *path);

static void init() {
	while (!vfs)
		vfs = processIdByName("system.io.vfs");

	if (!root) {
		{
			VFSOpAwaitRoot op = { VFS_OP_AWAIT_ROOT };
			u32 msg_id = sendQueue(vfs, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpAwaitRoot));
			struct queue_item_info *info = probeQueueFor(msg_id);
			shiftQueue(info);
		}

		{
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
}

bool fexists(const char *filename) {
	init();

	VFSNode *node = resolve_path(filename);
	return node;
}

FILE *fopen(const char *filename, const char *mode) {
	init();

	if (strcmp(mode, "r") != 0) panic("fs.cpp: don't know how to not read!");

	VFSNode *node = resolve_path(filename);
	if (!node)
		return 0;

	FILE *stream = new FILE;
	stream->file = node;
	stream->offset = 0;

	return stream;
}

u32 fread(void *buf, u32 size, u32 n, FILE *stream) {
	if (!stream || !buf) return 0;

	u8 *writer = reinterpret_cast<u8 *>(buf);
	
	u32 w = 0;
	while (n--) {
		read_fs(stream->file, stream->offset, size, writer);

		stream->offset += size;
		writer += size;

		++w;
	}

	return w;
}

u32 flen(FILE *stream) {
	return stream->file->length;
}

int fclose(FILE *stream) {
	if (!stream || !stream->file) return -1;

	delete stream->file;
	delete stream;

	return 0;
}

DIR *opendir(const char *dirname) {
	init();

	VFSNode *node = resolve_path(dirname);
	if (!node) return 0;

	DIR *dirp = new DIR;
	dirp->dir = node;
	dirp->index = 0;

	return dirp;
}

VFSDirent *readdir(DIR *dirp) {
	static VFSDirent static_dirent;

	if (!dirp || !dirp->dir) return 0;

	VFSDirent *dirent = readdir_fs(dirp->dir, dirp->index++);
	if (!dirent)
		return 0;

	memcpy(&static_dirent, dirent, sizeof(VFSDirent));
	delete dirent;

	return &static_dirent;
}

int closedir(DIR *dirp) {
	if (!dirp || !dirp->dir) return -1;

	delete dirp->dir;
	delete dirp;

	return 0;
}

static u32 read_fs(VFSNode *node, u32 offset, u32 length, void *buffer) {
	VFSOpRead op = { VFS_OP_READ, node->inode, offset, length };

	u32 msg = sendQueue(vfs, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpRead));
	struct queue_item_info *info = probeQueueFor(msg);
	if (info->data_len == 0) {
		shiftQueue(info);
		return 0;
	}

	u32 len = info->data_len;
	readQueue(info, reinterpret_cast<u8 *>(buffer), 0, len);
	shiftQueue(info);

	return len;
}

static VFSNode *finddir_fs(VFSNode *node, const char *component) {
	u32 cmd_len = sizeof(VFSOpFinddir) + strlen(component) + 1 - 1;
	
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

static VFSDirent *readdir_fs(VFSNode *node, u32 index) {
	VFSOpReaddir op = { VFS_OP_READDIR, node->inode, index };

	u32 msg = sendQueue(vfs, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpReaddir));
	struct queue_item_info *info = probeQueueFor(msg);
	if (info->data_len == 0) {
		shiftQueue(info);
		return 0;
	}

	VFSDirent *ret = new VFSDirent;
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

