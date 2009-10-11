#ifndef __USER_GATES_HPP__
#define __USER_GATES_HPP__

#include <arch.hpp>

#define DECL_SYSCALL0(fn) u32 syscall_##fn()
#define DECL_SYSCALL1(fn,p1) u32 syscall_##fn(p1)
#define DECL_SYSCALL2(fn,p1,p2) u32 syscall_##fn(p1,p2)
#define DECL_SYSCALL3(fn,p1,p2,p3) u32 syscall_##fn(p1,p2,p3)
#define DECL_SYSCALL4(fn,p1,p2,p3,p4) u32 syscall_##fn(p1,p2,p3,p4)
#define DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) u32 syscall_##fn(p1,p2,p3,p4,p5)
#define DECL_SYSCALL6(fn,p1,p2,p3,p4,p5,p6) u32 syscall_##fn(p1,p2,p3,p4,p5,p6)

// Here we declare the user-mode names.

DECL_SYSCALL1(putc, char);
DECL_SYSCALL1(puts, const char *);
DECL_SYSCALL2(putl, u32, u8);
DECL_SYSCALL0(getProcessId);
DECL_SYSCALL0(irqWait);
DECL_SYSCALL1(irqListen, u32);
DECL_SYSCALL1(panic, const char *);
DECL_SYSCALL1(registerName, const char *);
DECL_SYSCALL1(registerNode, const char *);
DECL_SYSCALL0(exit);
DECL_SYSCALL3(obtainNodeWriter, const char *, const char *, bool);
DECL_SYSCALL2(obtainNodeListener, const char *, const char *);
DECL_SYSCALL5(readNode, const char *, const char *, u32, char *, u32);
DECL_SYSCALL5(readNodeUnblock, const char *, const char *, u32, char *, u32);
DECL_SYSCALL5(writeNode, const char *, const char *, u32, const char *, u32);
DECL_SYSCALL0(defer);

#define SYSCALL_BOOL(x) ((bool)((x) & 0xFF))

#endif

