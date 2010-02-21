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

# Installs the bootloader ARGV[0] to ARGV[1]

if ARGV.length != 2
  STDERR.puts "usage: #$0 bootloader destimg"
  STDERR.puts "  tries to find where a bootloader will fit in, otherwise aborts"
  exit 1
end

bootloader = File.read ARGV[0]
dest = File.open ARGV[1], 'r+'

short_jmp, delta = dest.read(2).unpack('CC')

if short_jmp != 0xeb
  STDERR.puts "first opcode #{short_jmp.to_s 16} not 0xeb (relative short jmp) - confused"
  exit 1
end

avail = 510 - delta - 2
puts "available space #{avail}"
