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

#include <vfs.hpp>
#include <memory.hpp>
#include <string.hpp>
#include <screen.hpp>

fs_node_t *fs_root = 0;

void installvfs(void)
{
	fs_root = (fs_node_t *)kmalloc(sizeof(fs_node_t));
	strcpy(fs_root->name, "root");
	fs_root->flags = FS_DIRECTORY;
	fs_root->inode = fs_root->length = fs_root->impl = 0;
	fs_root->read = 0; fs_root->write = 0;
	fs_root->open = 0; fs_root->close = 0;
	fs_root->readdir = 0; fs_root->finddir = 0;
	fs_root->ptr = 0;
}

void apply_fs(unsigned long root_inode, readdir_fs_t readdir, finddir_fs_t finddir) {
	fs_root->inode = root_inode;
	fs_root->readdir = readdir;
	fs_root->finddir = finddir;
}

fs_node_t *resolve_fs_node(const char *path) {
	fs_node_t *node = fs_root;
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
			node = finddir_fs(node, component);
			if (!node)
				return 0;
		}
	
	return node;
}

unsigned long read_fs(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer)
{
	if (node->read)
		return node->read(node, offset, size, buffer);
	return 0;
}

unsigned long write_fs(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer)
{
	if (node->write)
		return node->write(node, offset, size, buffer);
	return 0;
}

void open_fs(fs_node_t *node, unsigned char read, unsigned char write)
{
	/* TODO XXX: open for read/write? */
	if (node->open)
		node->open(node);
}

void close_fs(fs_node_t *node)
{
	if (node->close)
		node->close(node);
}

dirent_t *readdir_fs(fs_node_t *node, unsigned long index)
{
	if (node->readdir && (node->flags & FS_NON_MOUNT_MASK) == FS_DIRECTORY)
		return node->readdir(node, index);
	return 0;
}

fs_node_t *finddir_fs(fs_node_t *node, const char *name)
{
	if (node->finddir && (node->flags & FS_NON_MOUNT_MASK) == FS_DIRECTORY)
		return node->finddir(node, name);
	return 0;
}
