#!/bin/bash

TARGET="grub-0.97"
if [ ! -d "$TARGET" ]; then
    mkdir "$TARGET"
fi

cd "$TARGET"
echo "Please note - if you are cross-compiling, you'll probably"
echo "need to install gcc-multilib or similar."

if [ ! -f grub-0.97.tar.gz ]; then
    wget ftp://alpha.gnu.org/gnu/grub/grub-0.97.tar.gz
fi

if [ ! -d grub-0.97 ]; then
    tar zxf grub-0.97.tar.gz
fi

cd grub-0.97

if [ ! -f configure-unpatched ]; then
    echo "Patching GRUB configure"
    cp configure configure-unpatched
    patch --verbose -u -l < ../../grub-configure.patch
fi

if [ ! -f Makefile ]; then
    CFLAGS=-m32\ -fno-stack-protector LDFLAGS=-m32 ./configure --host=i686-pc-linux-gnu --disable-ext2fs --disable-ffs --disable-iso9660 --disable-jfs --disable-minix --disable-reiserfs --disable-ufs2 --disable-vstafs --disable-xfs
else
    echo "Not running ./configure, Makefile exists already"
fi

make

echo "GRUB 0.97 has been built successfully"