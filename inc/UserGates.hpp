// This file is part of Akari.
// Copyright 2010 Arlen Cuss
//
// Akari is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Akari is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Akari.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __USER_GATES_HPP__
#define __USER_GATES_HPP__

#include <arch.hpp>

#define DECL_SYSCALL0(fn,r) extern "C" r syscall_##fn()
#define DECL_SYSCALL1(fn,r,p1) extern "C" r syscall_##fn(p1)
#define DECL_SYSCALL2(fn,r,p1,p2) extern "C" r syscall_##fn(p1,p2)
#define DECL_SYSCALL3(fn,r,p1,p2,p3) extern "C" r syscall_##fn(p1,p2,p3)
#define DECL_SYSCALL4(fn,r,p1,p2,p3,p4) extern "C" r syscall_##fn(p1,p2,p3,p4)
#define DECL_SYSCALL5(fn,r,p1,p2,p3,p4,p5) extern "C" r syscall_##fn(p1,p2,p3,p4,p5)

// In the interest of protecting the sanity of all, reinterpret_ or static_ style casts
// have not been used here, as they cause mayhem depending on what types are passed.
// (reinterpret_cast<void>(...) does not work, for instance. (void)x does.)

#define DEFN_SYSCALL0(fn, num, r) \
    extern "C" r syscall_##fn() { \
        u32 a = 0; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
        return (r)a; \
    }

#define DEFN_SYSCALL1(fn, num, r, P1) \
    extern "C" r syscall_##fn(P1 p1) { \
        u32 a = 0; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1)); \
        return (r)a; \
    }

#define DEFN_SYSCALL2(fn, num, r, P1, P2) \
    extern "C" r syscall_##fn(P1 p1, P2 p2) { \
        u32 a = 0; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
        return (r)a; \
    }

#define DEFN_SYSCALL3(fn, num, r, P1, P2, P3) \
    extern "C" r syscall_##fn(P1 p1, P2 p2, P3 p3) { \
        u32 a = 0; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3)); \
        return (r)a; \
    }

#define DEFN_SYSCALL4(fn, num, r, P1, P2, P3, P4) \
    extern "C" r syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) { \
        u32 a = 0; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4)); \
        return (r)a; \
    }

#define DEFN_SYSCALL5(fn, num, r, P1, P2, P3, P4, P5) \
    extern "C" r syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { \
        u32 a = 0; \
        asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4), "D" ((int)p5)); \
        return (r)a; \
    }

#endif

