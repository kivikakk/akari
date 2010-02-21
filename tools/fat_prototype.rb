#!/usr/bin/env ruby
#
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

class FAT
  def initialize(image_io)
	@io = image_io
	@mbr = _mbr_of(_raw_read(0, 0, 512))
	br = _fat_boot_record_of(_read(0, 0, 0, 0x24))
	p br
  end


  protected
	def _mbr_of(s)
	  partitions = []
	  0.upto(3) do |p_id|
		part = Hash[*[:bootable, :begin_head, :begin_cylinderhi_sector_mix, :begin_cylinderlo,
		  :system_id, :end_head, :end_cylinderhi_sector_mix, :end_cylinderlo, :begin_disk_sector,
		  :sector_count].zip(s[(446 + 16 * p_id)...(446 + 16 * (p_id + 1))].unpack('CCCCCCCCCLL')).flatten]
		part[:begin_cylinderhi] = part[:begin_cylinderhi_sector_mix] & 0x3
		part[:begin_sector] = part[:begin_cylinderhi_sector_mix] >> 2
		part[:end_cylinderhi] = part[:end_cylinderhi_sector_mix] & 0x3
		part[:end_sector] = part[:end_cylinderhi_sector_mix] >> 2
		partitions << part
	  end
	  raise "invalid mbr" unless s[510...512].unpack('S')[0] == 0xaa55
	  partitions
	end

	def _fat_boot_record_of(s)
	  comp = [:oem_identifier, :bytes_per_sector, :sectors_per_cluster, :reserved_sectors, :fat_count,
		:dirent_count, :sector_count_small, :media_descriptor, :sectors_per_fat, :sectors_per_track,
		:media_sides, :hidden_sectors, :total_sectors_large].zip(s.unpack('x3Z8SCSCSSCSSSL'))
	  Hash[*comp.flatten]
	end

	def _raw_read(sector, offset, length)
	  @io.seek sector * 512 + offset
	  @io.read length
	end

	def _read(partition_id, sector, offset, length)
	  _raw_read(@mbr[partition_id][:begin_disk_sector] + sector, offset, length)
	end
end

fat = FAT.new(File.open(ARGV[0], 'rb'))

