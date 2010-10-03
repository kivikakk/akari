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

#ifndef PCI_PROTO_HPP
#define PCI_PROTO_HPP

#define PCI_OP_DEVICE_CONFIG 	0x1
#define PCI_OP_DMA_UP 			0x2
#define PCI_OP_AWAIT_DRIVERS_UP	0x3

typedef struct {
	u8 cmd;
	u16 device, vendor;
} PCIOpDeviceConfig;

typedef struct {
	u8 cmd;
} PCIOpDMAUp;

typedef struct {
	u8 cmd;
} PCIOpAwaitDriversUp;

typedef struct {
	u16 vendor_id, device_id;
	u16 command, status;
	u8 revision_id, prog_if, subclass, class_code;
	u8 cache_line_size, latency_timer, header_type, bist;
	u32 bar0, bar1, bar2, bar3, bar4, bar5;
	u32 cardbus_cis_ptr;
	u16 subsystem_vendor_id, subsystem_id;
	u32 expansion_rom_base_addr;
	u8 capabilities_ptr;
	unsigned reserved_0 : 24;
	u32 reserved_1;
	u8 interrupt_line, interrupt_pin, min_grant, max_latency;
} __attribute__((__packed__)) pci_device_regular;


#endif
