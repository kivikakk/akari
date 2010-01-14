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

#ifndef VFS_HPP
#define VFS_HPP

#define VFS_OP_READ 	0x0
#define VFS_OP_WRITE	0x1
#define VFS_OP_OPEN		0x2
#define VFS_OP_CLOSE	0x3
#define VFS_OP_READDIR	0x4
#define VFS_OP_FINDDIR	0x5

#define VFS_FILE     0x01
#define VFS_DIRECTORY    0x02
#define VFS_CHARDEVICE   0x03
#define VFS_BLOCKDEVICE  0x04
#define VFS_PIPE     0x05
#define VFS_SYMLINK  0x06
#define VFS_NON_MOUNT_MASK   0x07
#define VFS_MOUNTPOINT   0x08


typedef struct {
	char name[128];
	u32 inode;
} VFSDirent;

typedef struct {
	char name[128];
	u32 flags, inode, length, impl;
} VFSNode;

#endif

