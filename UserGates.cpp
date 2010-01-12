#include <UserGates.hpp>

// Here we define the user-mode functions, matching them to the number in the _syscalls array (see Syscall).

DEFN_SYSCALL1(putc, 0, char);
DEFN_SYSCALL1(puts, 1, const char *);
DEFN_SYSCALL2(putl, 2, u32, u8);
DEFN_SYSCALL0(getProcessId, 3);
DEFN_SYSCALL0(irqWait, 4);
DEFN_SYSCALL1(irqListen, 5, u32);
DEFN_SYSCALL1(panic, 6, const char *);
DEFN_SYSCALL0(exit, 7);
DEFN_SYSCALL0(defer, 8);
DEFN_SYSCALL1(malloc, 9, u32);
DEFN_SYSCALL1(free, 10, void *);
DEFN_SYSCALL3(memcpy, 11, void *, const void *, u32);

