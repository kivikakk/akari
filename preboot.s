.code32
.section .text

.globl AkariPreboot
AkariPreboot:
	cmp $0x2BADB002, %eax
	jne .NoMultiboot
	jmp .HasMultiboot

.NoMultiboot:
	pushl $AkariEntryNoMultibootMessage
	hlt
	#; call _AkariPanic

.HasMultiboot:
	mov %ebx, _AkariMultiboot
	movl %esp, _AkariStack

	jmp _AkariEntry

.section .data
AkariEntryNoMultibootMessage:
	.asciz "Akari: not loaded by a MULTIBOOT compliant boot loader!\nCannot continue - dying now. Please use GRUB or a similar loader.\n"

