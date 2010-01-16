#; This file is part of Akari.
#; Copyright 2010 Arlen Cuss
#; 
#; Akari is free software: you can redistribute it and/or modify
#; it under the terms of the GNU General Public License as published by
#; the Free Software Foundation, either version 3 of the License, or
#; (at your option) any later version.
#; 
#; Akari is distributed in the hope that it will be useful,
#; but WITHOUT ANY WARRANTY; without even the implied warranty of
#; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#; GNU General Public License for more details.
#; 
#; You should have received a copy of the GNU General Public License
#; along with Akari.  If not, see <http://www.gnu.org/licenses/>.

.code32
.section .text

.globl AkariPreboot
AkariPreboot:
	cmp $0x2BADB002, %eax
	jne .NoMultiboot
	jmp .HasMultiboot

.NoMultiboot:
	pushl $AkariEntryNoMultibootMessage
	call _AkariPanic
	hlt

.HasMultiboot:
	mov %ebx, _AkariMultiboot

	jmp _AkariEntry

.section .data
AkariEntryNoMultibootMessage:
	.asciz "Akari: not loaded by a MULTIBOOT compliant boot loader! Cannot continue - dying now. Please use GRUB or a similar loader."

#; Multiboot header

.code16
.section .multiboot
.align 4

AkariMultibootHeader:
	.long 0x1BADB002
	#; 0: align on 4K
	#; 1: include mem info
	#; 3: includes modules (set if needed)
	.long 0b00000000000000000000000000000011
	.long -(0x1BADB002 + 0b11)						#; 32-bit unsigned checksum that adds to the header & options to make 0

