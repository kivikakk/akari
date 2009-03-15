#!/usr/bin/env ruby

if ARGV.length != 1
  puts "Specify a filename."
  exit
end

class String
  def h2s
    self.split(/(..)/).reject {|q| q.length.zero?}.h2s
  end
end

class Array
  def h2s
    self.map {|n| n.to_i 16}.pack 'c*'
  end
end

File.open(ARGV.first, 'w') do |o|
  o.seek 0x1b8
  o.write %w(68 29 51 5b 00 00 00 01 01 00 83 fe 3f 00 3f 00 00 00 82 3e).h2s
  o.seek 0x1fe
  o.write %w(55 aa).h2s
  o.seek 0x7dff
  o.write %w(00).h2s
end

