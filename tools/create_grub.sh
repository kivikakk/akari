#!/bin/bash
# This file is part of Akari.
# Copyright 2010 Arlen Cuss
# 
# Akari is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Akari is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Akari.  If not, see <http://www.gnu.org/licenses/>.

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
    patch --verbose -p0 -u -l < ../../grub-configure.patch
fi

if [ ! -f stage1/Makefile.in-unpatched ]; then
    echo "Patching stage1/Makefile.in"
    cp stage1/Makefile.in stage1/Makefile.in-unpatched
    patch --verbose -p0 -u -l < ../../grub-stage1-Makefile.in.patch
fi

if [ ! -f stage2/Makefile.in-unpatched ]; then
    echo "Patching stage2/Makefile.in"
    cp stage2/Makefile.in stage2/Makefile.in-unpatched
    patch --verbose -p0 -u -l < ../../grub-stage2-Makefile.in.patch
fi

if [ ! -f Makefile ]; then
    CFLAGS=-m32\ -fno-stack-protector\ -Wl,--build-id=none LDFLAGS=-m32 ./configure --host=i686-pc-linux-gnu --disable-ext2fs --disable-ffs --disable-iso9660 --disable-jfs --disable-minix --disable-reiserfs --disable-ufs2 --disable-vstafs --disable-xfs
else
    echo "Not running ./configure, Makefile exists already"
fi

make

echo "GRUB 0.97 has been built successfully"
