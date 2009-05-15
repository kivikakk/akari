#include <UserGates.hpp>

#define DEFN_SYSCALL0(fn, num) \
    int syscall_##fn() { \
        int a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
        return a; \
    }

#define DEFN_SYSCALL1(fn, num, P1) \
    int syscall_##fn(P1 p1) { \
        int a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1)); \
        return a; \
    }

#define DEFN_SYSCALL2(fn, num, P1, P2) \
    int syscall_##fn(P1 p1, P2 p2) { \
        int a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
        return a; \
    }

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
    int syscall_##fn(P1 p1, P2 p2, P3 p3) { \
        int a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3)); \
        return a; \
    }

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
    int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) { \
        int a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4)); \
        return a; \
    }

#define DEFN_SYSCALL5(fn, num, P1, P2, P3, P4, P5) \
    int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { \
        int a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4), "D" ((int)p5)); \
        return a; \
    }

// Here we define the user-mode functions, matching them to the number in the _syscalls array (see AkariSyscallSubsystem).

DEFN_SYSCALL1(puts, 0, const char *);

