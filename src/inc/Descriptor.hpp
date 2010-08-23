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

#ifndef __DESCRIPTOR_HPP__
#define __DESCRIPTOR_HPP__

#include <Subsystem.hpp>
#include <interrupts.hpp>

// the subsystem itself

class Descriptor : public Subsystem {
public:
	Descriptor();

	explicit Descriptor(const Descriptor &);
	Descriptor &operator =(const Descriptor &);

	u8 versionMajor() const;
	u8 versionMinor() const;
	const char *versionManufacturer() const;
	const char *versionProduct() const;

	class GDT {
	public:
		GDT(u32);

		void setGateFields(s32 num, ptr_t base, ptr_t limit, u8 access, u8 granularity);

		void setGate(s32 num, ptr_t base, ptr_t limit, u8 dpl, bool code);
		void clearGate(s32 num);
		void writeTSS(s32 num, u16 ss0, u32 esp0);

		void flush();
		void flushTSS(s32);

		void setTSSStack(ptr_t);
		void setTSSIOMap(u8 *const &);

	protected:
		struct Entry {
			u16 limit_low;
			u16 base_low;
			u8 base_middle;
			union {
				u8 access;
				__extension__ struct {
					unsigned segment_type : 4;
					unsigned descriptor_type : 1;
					unsigned privilege_level : 2;
					unsigned present : 1;
				} __attribute__((__packed__));
			};
			u8 granularity;
			u8 base_high;
		} __attribute__((__packed__));

		struct Pointer {
			u16 limit;
			u32 base;
		} __attribute__((__packed__));

		class TSSEntry {
		public:
			u32 prev_tss;					// linked list in hardware task switching
			u32 esp0, ss0;					// stack ptr+segment to load when we change to kernel mode
			// unused below.
			u32 esp1, ss1, esp2, ss2, cr3, eip;
			u32 eflags, eax, ecx, edx, ebx, esp, ebp;
			u32 esi, edi;
			// unused above.
			u32 es, cs, ss, ds, fs, gs;		// loaded into appropriate registers when we change to kernel
			u32 ldt;						// unused
			u16 trap;						// unused
			u16 iomap_base;
			u8 iomap[(65536 / 8)];		// We are currently using 8K per TSS just for iomap.. Ugh.
		} __attribute__((__packed__));

		u32 _entryCount;
		Entry *_entries;
		Pointer _pointer;
		TSSEntry _tssEntry;
	};

	class IDT {
	public:
		IDT();

		void setGate(u8, void (*)(), u16, u8);
		void installHandler(u8, isr_handler_func_t);
		void clearHandler(u8);
		void *callHandler(u8, struct modeswitch_registers *);

	protected:
		union Entry {
			__extension__ struct {
				unsigned offset_low : 16;
				unsigned selector : 16;
				unsigned _always_0 : 8;
				unsigned flags : 8;
				unsigned offset_high : 16;
			} __attribute__((__packed__));
			unsigned long ulong;
		} __attribute__((__packed__));

		struct Pointer {
			u16 limit;
			u32 ridt;
		} __attribute__((__packed__));

		isr_handler_func_t _routines[256];
		Entry _entries[256];
		Pointer _pointer;
	};

	class IRQT {
	public:
		IRQT(IDT *);

		void installHandler(u8, irq_handler_func_t);
		void clearHandler(u8);
		void *callHandler(u8, struct modeswitch_registers *);

	protected:
		irq_handler_func_t _routines[16];
		IDT *_idt;
	};

	GDT *gdt;
	IDT *idt;
	IRQT *irqt;
};

#endif

