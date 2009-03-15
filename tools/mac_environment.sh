export AS=/opt/local/bin/i386-elf-as
export LD=/opt/local/bin/i386-elf-ld
export CC=/opt/local/bin/i386-elf-gcc-4.3.2
export CXX=/opt/local/bin/i386-elf-g++-4.3.2
export STRIP=/opt/local/bin/i386-elf-strip
export MTOOLS_BIN=/opt/local/bin
alias qemu="/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu -cocoaquartz -cocoapath /Users/celtic/Documents/QEMU/Akari.qvm -cocoaname Akari -cocoalivethumbnail -m 128 -localtime -net nic -net user -smb /Users/celtic/Desktop/Q Shared Files -fda /Users/celtic/Code/Akari/disk.img -hda /Users/celtic/Code/Akari/16disk.img -cdrom /dev/cdrom -boot a"
alias debug="i386-elf-gdb obj/lobster -ex 'target remote localhost:1234'"
alias bochs="/usr/local/share/bochs.app/Contents/MacOS/bochs -f tools/bochsrc"
