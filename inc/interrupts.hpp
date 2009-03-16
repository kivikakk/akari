#ifndef __INTERRUPTS_HPP__
#define __INTERRUPTS_HPP__

#include <arch.hpp>

/* This needs to go somewhere better, the structure needs to be verified, and its purpose verified */
struct registers {
	u32 ds;
	u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
	u32 int_no, err_code;
	u32 eip, cs, eflags, useresp, ss;
};

/* This needs to go somewhere better, the structure needs to be verified, and its purpose verified */
typedef struct irq_regs *(*isr_handler_func)(struct registers);
typedef struct irq_regs *(*irq_handler_func)(struct registers); // ??

extern "C" void isr_handler(struct registers);
extern "C" void irq_handler(struct registers);

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

#endif

