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

#; copies a frame from one physical address to another
.type _AkariCopyFramePhysical, @function
.globl _AkariCopyFramePhysical
_AkariCopyFramePhysical:
	push %ebx
	pushf
	cli

	mov 12(%esp), %ebx		#; src
	mov 16(%esp), %ecx		#; dst

	mov %cr0, %edx
	and $0x7FFFFFFF, %edx
	mov %edx, %cr0			#; paging disabled

	mov $1024, %edx			#; copy 1024*4=0x1000

.loop:
	movl (%ebx), %eax
	movl %eax, (%ecx)
	add $4, %ebx
	add $4, %ecx

	dec %edx
	jnz .loop

	mov %cr0, %edx
	or $0x80000000, %edx
	mov %edx, %cr0			#; paging enabled

	popf
	pop %ebx
	ret

