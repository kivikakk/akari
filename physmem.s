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

