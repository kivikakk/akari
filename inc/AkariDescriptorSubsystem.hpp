#ifndef __AKARI_DESCRIPTOR_SUBSYSTEM_HPP__
#define __AKARI_DESCRIPTOR_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>
#include <interrupts.hpp>

// the subsystem itself

class AkariDescriptorSubsystem : public AkariSubsystem {
	public:
		AkariDescriptorSubsystem();

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

	//protected:
	// XXX - _idt, _irqt is used publicly... whoops

		class GDT {
			public:
				GDT(u32);

				void SetGate(s32, u32, u32, u8, u8);
				void WriteTSS(s32, u16, u32);
				void Flush();
				void FlushTSS(s32);

			protected:
				struct Entry {
					u16 limit_low;
					u16 base_low;
					u8 base_middle;
					union {
						u8 access;
						struct {
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
						u32 prev_tss;	// linked list in hardware task switching
						u32 esp0, ss0;	// stack ptr+segment to load when we change to kernel mode
						// unused below.
						u32 esp1, ss1, esp2, ss2, cr3, eip;
						u32 eflags, eax, ecx, edx, ebx, esp, ebp;
						u32 esi, edi;
						// unused above.
						u32 es, cs, ss, ds, fs, gs;		// loaded into appropriate registers when we change to kernel
						u32 ldt;					// unused
						u16 trap, iomap_base;		// unused
				} __attribute__((__packed__));

				u32 _entryCount;
				Entry *_entries;
				Pointer _pointer;
				TSSEntry _tssEntry;
		};

		class IDT {
			public:
				IDT();

				void SetGate(u8, void (*)(), u16, u8);
				void InstallHandler(u8, isr_handler_func_t);
				void ClearHandler(u8);
				bool CallHandler(u8, struct registers);

			protected:
				union Entry {
					struct {
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

				void InstallHandler(u8, irq_handler_func_t);
				void ClearHandler(u8);
				void CallHandler(u8, struct registers *);

			protected:
				irq_handler_func_t _routines[16];
				IDT *_idt;
		};

		GDT *_gdt;
		IDT *_idt;
		IRQT *_irqt;
};

#endif

