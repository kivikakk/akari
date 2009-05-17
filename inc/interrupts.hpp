#ifndef __INTERRUPTS_HPP__
#define __INTERRUPTS_HPP__

#include <arch.hpp>

struct callback_registers {
	u32 ds;
	u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
	u32 int_no, err_code;
	u32 eip, cs, eflags;
} __attribute__((__packed__));

struct modeswitch_registers {
	struct callback_registers callback;
	u32 useresp, ss;		// 8 bytes
} __attribute__((__packed__));

/* This needs to go somewhere better, the structure needs to be verified, and its purpose verified */
typedef void (*isr_handler_func_t)(struct modeswitch_registers *);
typedef void *(*irq_handler_func_t)(struct modeswitch_registers *);

extern "C" void isr_handler(struct modeswitch_registers *);
extern "C" void *irq_handler(struct modeswitch_registers *);

extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isra();
extern "C" void isrb();
extern "C" void isrc();
extern "C" void isrd();
extern "C" void isre();
extern "C" void isrf();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr1a();
extern "C" void isr1b();
extern "C" void isr1c();
extern "C" void isr1d();
extern "C" void isr1e();
extern "C" void isr1f();
extern "C" void isr80();

extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq3();
extern "C" void irq4();
extern "C" void irq5();
extern "C" void irq6();
extern "C" void irq7();
extern "C" void irq8();
extern "C" void irq9();
extern "C" void irqa();
extern "C" void irqb();
extern "C" void irqc();
extern "C" void irqd();
extern "C" void irqe();
extern "C" void irqf();

#endif

