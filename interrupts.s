.code32
.section .text

.globl _isr0
.globl _isr1
.globl _isr2
.globl _isr3
.globl _isr4
.globl _isr5
.globl _isr6
.globl _isr7
.globl _isr8
.globl _isr9
.globl _isra
.globl _isrb
.globl _isrc
.globl _isrd
.globl _isre
.globl _isrf
.globl _isr10
.globl _isr11
.globl _isr12
.globl _isr13
.globl _isr14
.globl _isr15
.globl _isr16
.globl _isr17
.globl _isr18
.globl _isr19
.globl _isr1a
.globl _isr1b
.globl _isr1c
.globl _isr1d
.globl _isr1e
.globl _isr1f
.globl _isr80

.globl _irq0
.globl _irq1
.globl _irq2
.globl _irq3
.globl _irq4
.globl _irq5
.globl _irq6
.globl _irq7
.globl _irq8
.globl _irq9
.globl _irqa
.globl _irqb
.globl _irqc
.globl _irqd
.globl _irqe
.globl _irqf

_isr0:	cli
	push $0x00
	push $0x00
	jmp isr_common

_isr1:	cli
	push $0x00
	push $0x01
	jmp isr_common

_isr2:	cli
	push $0x00
	push $0x02
	jmp isr_common

_isr3:	cli
	push $0x00
	push $0x03
	jmp isr_common

_isr4:	cli
	push $0x00
	push $0x04
	jmp isr_common

_isr5:	cli
	push $0x00
	push $0x05
	jmp isr_common

_isr6:	cli
	push $0x00
	push $0x06
	jmp isr_common

_isr7:	cli
	push $0x00
	push $0x07
	jmp isr_common

_isr8:	cli
	push $0x08
	jmp isr_common

_isr9:	cli
	push $0x00
	push $0x09
	jmp isr_common

_isra:	cli
	push $0x0a
	jmp isr_common

_isrb:	cli
	push $0x0b
	jmp isr_common

_isrc:	cli
	push $0x0c
	jmp isr_common

_isrd:	cli
	push $0x0d
	jmp isr_common

_isre:	cli
	push $0x0e
	jmp isr_common

_isrf:	cli
	push $0x00
	push $0x0f
	jmp isr_common

_isr10:	cli
	push $0x00
	push $0x10
	jmp isr_common

_isr11:	cli
	push $0x00
	push $0x11
	jmp isr_common

_isr12:	cli
	push $0x00
	push $0x12
	jmp isr_common

_isr13:	cli
	push $0x00
	push $0x13
	jmp isr_common

_isr14:	cli
	push $0x00
	push $0x14
	jmp isr_common

_isr15:	cli
	push $0x00
	push $0x15
	jmp isr_common

_isr16:	cli
	push $0x00
	push $0x16
	jmp isr_common

_isr17:	cli
	push $0x00
	push $0x17
	jmp isr_common

_isr18:	cli
	push $0x00
	push $0x18
	jmp isr_common

_isr19:	cli
	push $0x00
	push $0x19
	jmp isr_common

_isr1a:	cli
	push $0x00
	push $0x1a
	jmp isr_common

_isr1b:	cli
	push $0x00
	push $0x1b
	jmp isr_common

_isr1c:	cli
	push $0x00
	push $0x1c
	jmp isr_common

_isr1d:	cli
	push $0x00
	push $0x1d
	jmp isr_common

_isr1e:	cli
	push $0x00
	push $0x1e
	jmp isr_common

_isr1f:	cli
	push $0x00
	push $0x1f
	jmp isr_common

_isr80:	cli
	push $0x00
	push $0x80
	jmp isr_common


#; timer is special case, handled manually!
_irq0:	cli
	push $0x00
	push $0x20
	jmp irq_timer_multitask

_irq1:	cli
	push $0x00
	push $0x21
	jmp irq_common

_irq2:	cli
	push $0x00
	push $0x22
	jmp irq_common

_irq3:	cli
	push $0x00
	push $0x23
	jmp irq_common

_irq4:	cli
	push $0x00
	push $0x24
	jmp irq_common

_irq5:	cli
	push $0x00
	push $0x25
	jmp irq_common

_irq6:	cli
	push $0x00
	push $0x26
	jmp irq_common

_irq7:	cli
	push $0x00
	push $0x27
	jmp irq_common

_irq8:	cli
	push $0x00
	push $0x28
	jmp irq_common

_irq9:	cli
	push $0x00
	push $0x29
	jmp irq_common

_irqa:	cli
	push $0x00
	push $0x2a
	jmp irq_common

_irqb:	cli
	push $0x00
	push $0x2b
	jmp irq_common

_irqc:	cli
	push $0x00
	push $0x2c
	jmp irq_common

_irqd:	cli
	push $0x00
	push $0x2d
	jmp irq_common

_irqe:	cli
	push $0x00
	push $0x2e
	jmp irq_common

_irqf:	cli
	push $0x00
	push $0x2f
	jmp irq_common

.type isr_common, @function
isr_common:
	pusha

	mov %ds, %ax
	push %eax

	mov %esp, %eax		#; stack from here *up*: ds, `pusha', task switch
	push %eax			#; ptr to said stack becomes parameter for _irq_handler

	mov $0x10, %ax		#; kernel data segment descriptor
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	call _isr_handler

	add $4, %esp

	pop %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	add $8, %esp		#; cleans pushed error code, and pushed ISR number
	sti	
	iret				#; pops CS, EIP, EFLAGS, SS, ESP

.type irq_common, @function
irq_common:
	pusha

	mov %ds, %ax
	push %eax

	mov %esp, %eax		#; stack from here *up*: ds, `pusha', task switch
	push %eax			#; ptr to said stack becomes parameter for _irq_handler

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	call _irq_handler

	add $4, %esp		#; clean up pointer

	pop %eax
	mov %ax, %ds		#; restore from stack, which _irq_handler may change
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	add $8, %esp
	sti					#; waits one instruction, remember?
	iret

.type irq_timer_multitask, @function
irq_timer_multitask:
	pusha

	mov %ds, %ax
	push %eax

	mov %esp, %eax
	mov $0xE0000000, %esp
	push %eax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	mov $0x20, %al
	outb %al, $0x20	#; EOI to master irq controller

	call _AkariMicrokernel	#; see use of %eax below, i.e. the return value.

	#; add $4, %esp	#; remove the pointer arg we pushed
	#; technically the above is wasted since we replace %esp below,
	#; but should we not at some point later, don't forget to remove
	#; the pointer arg with something similar!

	mov %eax, %esp		#; this will be either the utks or kernel-stack. either way.

	pop %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	add $8, %esp
	sti
	iret

