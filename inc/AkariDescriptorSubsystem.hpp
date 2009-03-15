#ifndef __AKARI_DESCRIPTOR_SUBSYSTEM_HPP__
#define __AKARI_DESCRIPTOR_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

// GDT structures

extern "C" struct GDTEntry {
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

extern "C" struct GDTPtr {
	u16 limit;
	u32 base;
} __attribute__((__packed__));

// the subsystem itself

class AkariDescriptorSubsystem : public AkariSubsystem {
	public:
		AkariDescriptorSubsystem();

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

	protected:
};

#endif

