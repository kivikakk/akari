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
  def initialize(io, part_id)
	@io, @part_id = io, part_id

	@mbr = _mbr_of(_raw_read(0, 0, 512))
	@fbr = _fat_boot_record_of(_read(0, 0, 0x24))
	ebr = _fat_ebr_of(_read(0, 0x24, 476))
	ebr32 = _fat_ebr32_of(_read(0, 0x24, 476))
	p @mbr
	p ebr32
	p @fbr

	@fat_style =
	  if @fbr[:total_sectors_small] > 0 and [0x28, 0x29].include?(ebr[:signature])
		raise "FAT16 not handled"
		@ebr = ebr
		:fat
	  elsif @fbr[:total_sectors_large] > 0 and [0x28, 0x29].include?(ebr32[:signature])
		@ebr = ebr32
		:fat32
	  else
		raise "Unknown FAT type"
	  end

	p (fat_sectors = @ebr[:fat_size])
	p (first_data_sector = @fbr[:reserved_sectors] + fat_sectors * @fbr[:fats])
	p (first_fat_sector = @fbr[:reserved_sectors])

	p (root_cluster = @ebr[:root_directory_cluster])

	puts "b/s"
	p (bytes_per_cluster = @fbr[:bytes_per_sector] * @fbr[:sectors_per_cluster])

	cluster_zero = _read(first_fat_sector, 0, bytes_per_cluster)
	p first_fat_sector
	p root_cluster
	p first_data_sector
	p _read(first_data_sector, 0, 512)

	p @ebr[:fat_size]
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
	  Hash[*[:oem_identifier, :bytes_per_sector, :sectors_per_cluster, :reserved_sectors, :fats,
		:directory_entries, :total_sectors_small, :media_descriptor, :sectors_per_fat, :sectors_per_track,
		:media_sides, :hidden_sectors, :total_sectors_large].zip(s.unpack('x3Z8SCSCSSCSSSLL')).flatten]
	end

	def _fat_ebr_of(s)
	  Hash[*[:drive_number, :nt_flags, :signature, :serial_number, :volume_label, :system_id].zip(
		s.unpack('CCCLA11A8')).flatten]
	end

	def _fat_ebr32_of(s)
	  p s[28...(28+26)]
	  Hash[*[:fat_size, :flags, :fat_version_minor, :fat_version_major, :root_directory_cluster,
		:fsinfo_cluster, :backup_boot_cluster].zip(s.unpack('LSCCLSS')).flatten].merge(
		  _fat_ebr_of(s[28...(28+26)]))
	end

	def _raw_read(sector, offset, length)
	  @io.seek sector * 512 + offset
	  @io.read length
	end

	def _read(sector, offset, length)
	  _raw_read(@mbr[@part_id][:begin_disk_sector] + sector, offset, length)
	end
end

fat = FAT.new(File.open(ARGV[0], 'rb'), 0)

