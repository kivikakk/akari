export AS=as
export LD=ld
export CC=gcc
export CXX=g++
export STRIP=strip
export MTOOLS_BIN=/usr/bin
alias qemu="qemu -m 128 -localtime -net nic -net user -hda /home/celtic/Code/akari/32disk.img -cdrom /dev/cdrom -boot c"
alias debug="gdb Akari -ex 'target remote localhost:1234'"
