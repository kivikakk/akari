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
export AS=as
export LD=ld
export CC=gcc
export CXX=g++
export STRIP=strip
export MTOOLS_BIN=/usr/bin
alias qemu="qemu -m 128 -localtime -net nic -net user -hda /home/celtic/Code/akimg.img -cdrom /dev/cdrom -boot c"
alias debug="gdb Akari -ex 'target remote localhost:1234'"
