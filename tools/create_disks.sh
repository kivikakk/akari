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

# Oh god how did this get here I am not good with computer

IMAGE="32disk.img"
GRUBINST="grub-0.97"

if [ ! -f "$GRUBINST"/grub-0.97/stage1/stage1 ]; then
    echo "GRUB does not appear to be built. Please run create_grub.sh first,"
    echo "or ensure that \"$GRUBINST\" is in the current directory (as created"
    echo "by create_grub.sh)."
    exit 1
elif [ ! -f grub-configure.patch ]; then
    echo "grub-configure.patch not found in cwd."
    exit 1
elif [ ! -f grub-menu.lst ]; then
    echo "grub-menu.lst not found in cwd (to be installed in new image)."
    exit 1
fi

echo "  I am about to create a sparse 1GiB HDD image (\"$IMAGE\"),"
echo "mount it under a loopback, create an MBR partition which takes"
echo "its entirety, then create a FAT-32 filesystem on that partition."
echo "  I'll then go and install GRUB on that hard drive to boot from "
echo "the created partition."
echo "  I will use sudo and fail immediately if anything goes wrong."
echo "Should that happen, you will need to step in and clean up after me."
echo

echo -n "Continue? (yes or anything else to cancel) "
read cont
if [ "$cont" != "yes" ]; then
    exit
fi
echo

echo "Creating \"$IMAGE\" ..."
mkdosfs -C "$IMAGE" 1048576

set -e

echo "Finding free loopback ..."
loopback="`sudo losetup -f`"
sudo losetup "$loopback" "$IMAGE"

echo "Partitioning \"$loopback\" ..."
sudo sfdisk --no-reread "$loopback" <<EOF
0,,b *
;
;
;
EOF

echo "Finding another free loopback ..."
partloop="`sudo losetup -f`"
sudo losetup -o 512 "$partloop" "$loopback"

echo "Creating DOS filesystem on \"$partloop\" ..."
sudo mkdosfs "$partloop"

echo "Creating temporary mountpoint ..."
mntpoint="`mktemp -d`"

echo "Mounting \"$partloop\" on \"$mntpoint\" ..."
sudo mount "$partloop" "$mntpoint"

echo "Copying across GRUB ... "
sudo mkdir -p "$mntpoint"/boot/grub
sudo cp -r "$GRUBINST"/grub-0.97/stage1/stage1 "$GRUBINST"/grub-0.97/stage2/stage2 "$GRUBINST"/grub-0.97/stage2/fat_stage1_5 "$mntpoint"/boot/grub
sudo cp grub-menu.lst "$mntpoint"/boot/grub/menu.lst
sudo sync

sudo umount "$partloop"
sudo rmdir "$mntpoint"
sudo losetup -d "$partloop"
sudo losetup -d "$loopback"

echo "Running GRUB shell to install ..."
sudo "$GRUBINST"/grub-0.97/grub/grub --device-map=/dev/null <<EOF
device (hd0) $IMAGE
root (hd0,0)
setup (hd0)
EOF
