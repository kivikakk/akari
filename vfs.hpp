#ifndef __VFS_HPP__
#define __VFS_HPP__

#include <cpp.hpp>

cextern void installvfs(void);

#define FS_FILE		0x01
#define FS_DIRECTORY	0x02
#define FS_CHARDEVICE	0x03
#define FS_BLOCKDEVICE	0x04
#define FS_PIPE		0x05
#define FS_SYMLINK	0x06
#define FS_NON_MOUNT_MASK	0x07
#define FS_MOUNTPOINT	0x08

/* `dirent' is POSIX */
cextern typedef struct dirent
{
	char name[128];
	unsigned long ino;
} dirent_t;

cextern struct fs_node;

cextern typedef unsigned long (*read_fs_t)(struct fs_node *, unsigned long, unsigned long, unsigned char *);
cextern typedef unsigned long (*write_fs_t)(struct fs_node *, unsigned long, unsigned long, unsigned char *);
cextern typedef void (*open_fs_t)(struct fs_node *);
cextern typedef void (*close_fs_t)(struct fs_node *);
cextern typedef dirent_t *(*readdir_fs_t)(struct fs_node *, unsigned long);
cextern typedef struct fs_node *(*finddir_fs_t)(struct fs_node *, const char *);

cextern typedef struct fs_node
{
	char name[128];
	unsigned long flags;
	unsigned long inode, length;
	unsigned long impl;
	read_fs_t read;
	write_fs_t write;
	open_fs_t open;
	close_fs_t close;
	readdir_fs_t readdir;
	finddir_fs_t finddir;
	struct fs_node *ptr;
} fs_node_t;

asmextern fs_node_t *fs_root;

cextern void apply_fs(unsigned long root_inode, readdir_fs_t readdir, finddir_fs_t finddir);
cextern fs_node_t *resolve_fs_node(const char *path);

cextern unsigned long read_fs(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer);
cextern unsigned long write_fs(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer);
cextern void open_fs(fs_node_t *node, unsigned char read, unsigned char write);
cextern void close_fs(fs_node_t *node);
cextern dirent_t *readdir_fs(fs_node_t *node, unsigned long index);
cextern fs_node_t *finddir_fs(fs_node_t *node, const char *name);


#endif

