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
	// XXX - _irqt is used publicly... whoops

		class GDT {
			public:
				GDT(u32);

				void SetGate(s32, u32, u32, u8, u8);
				void Flush();

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

				u32 _entryCount;
				Entry *_entries;
				Pointer _pointer;
		};

		class IDT {
			public:
				IDT();

				void SetGate(u8, void (*)(), u16, u8);

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

				Entry _entries[256];
				Pointer _pointer;
		};

		class IRQT {
			public:
				IRQT(IDT *);

				void InstallHandler(u8, irq_handler_func_t);
				void ClearHandler(u8);
				void CallHandler(u8, struct registers);

			protected:
				irq_handler_func_t _routines[16];
				IDT *_idt;
		};

		GDT *_gdt;
		IDT *_idt;
		IRQT *_irqt;
};

#endif

