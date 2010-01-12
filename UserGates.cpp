#include <UserGates.hpp>

#define DEFN_SYSCALL0(fn, num) \
    u32 syscall_##fn() { \
        u32 a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
        return a; \
    }

#define DEFN_SYSCALL1(fn, num, P1) \
    u32 syscall_##fn(P1 p1) { \
        u32 a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1)); \
        return a; \
    }

#define DEFN_SYSCALL2(fn, num, P1, P2) \
    u32 syscall_##fn(P1 p1, P2 p2) { \
        u32 a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
        return a; \
    }

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
    u32 syscall_##fn(P1 p1, P2 p2, P3 p3) { \
        u32 a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3)); \
        return a; \
    }

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
    u32 syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) { \
        u32 a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4)); \
        return a; \
    }

#define DEFN_SYSCALL5(fn, num, P1, P2, P3, P4, P5) \
    u32 syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { \
        u32 a; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4), "D" ((int)p5)); \
        return a; \
    }

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

DEFN_SYSCALL1(registerName, 12, const char *);

DEFN_SYSCALL1(registerStream, 13, const char *);
DEFN_SYSCALL3(obtainStreamWriter, 14, const char *, const char *, bool);
DEFN_SYSCALL2(obtainStreamListener, 15, const char *, const char *);
DEFN_SYSCALL5(readStream, 16, const char *, const char *, u32, char *, u32);
DEFN_SYSCALL5(readStreamUnblock, 17, const char *, const char *, u32, char *, u32);
DEFN_SYSCALL5(writeStream, 18, const char *, const char *, u32, const char *, u32);

DEFN_SYSCALL0(probeQueue, 19);
DEFN_SYSCALL0(probeQueueUnblock, 20);
DEFN_SYSCALL3(readQueue, 21, char *, u32, u32);
DEFN_SYSCALL0(shiftQueue, 22);
DEFN_SYSCALL4(sendQueue, 23, const char *, u32, const char *, u32);

