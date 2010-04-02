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
export AR=/opt/local/bin/i386-elf-ar
export AS=/opt/local/bin/i386-elf-as
export LD=/opt/local/bin/i386-elf-ld
export CC=/opt/local/bin/i386-elf-gcc-4.3.2
export CXX=/opt/local/bin/i386-elf-g++-4.3.2
export STRIP=/opt/local/bin/i386-elf-strip
export MTOOLS_BIN=/opt/local/bin
alias qemu="/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu -cocoaquartz -cocoapath /Users/celtic/Documents/QEMU/Akari.qvm -cocoaname Akari -cocoalivethumbnail -m 128 -localtime -net nic -net user -smb /Users/celtic/Desktop/Q Shared Files -hda /Users/celtic/Code/akimg.img -cdrom /dev/cdrom -boot c"
alias debug="i386-elf-gdb Akari -ex 'target remote localhost:1234'"
alias bochs="/usr/local/share/bochs.app/Contents/MacOS/bochs -f tools/bochsrc"
