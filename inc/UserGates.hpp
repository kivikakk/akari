#ifndef __USER_GATES_HPP__
#define __USER_GATES_HPP__

#include <arch.hpp>

#define DECL_SYSCALL0(fn) u32 syscall_##fn()
#define DECL_SYSCALL1(fn,p1) u32 syscall_##fn(p1)
#define DECL_SYSCALL2(fn,p1,p2) u32 syscall_##fn(p1,p2)
#define DECL_SYSCALL3(fn,p1,p2,p3) u32 syscall_##fn(p1,p2,p3)
#define DECL_SYSCALL4(fn,p1,p2,p3,p4) u32 syscall_##fn(p1,p2,p3,p4)
#define DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) u32 syscall_##fn(p1,p2,p3,p4,p5)

// Here we declare the user-mode names.

DECL_SYSCALL1(puts, const char *);
DECL_SYSCALL2(putl, u32, u8);
DECL_SYSCALL0(getProcessId);

#endif

