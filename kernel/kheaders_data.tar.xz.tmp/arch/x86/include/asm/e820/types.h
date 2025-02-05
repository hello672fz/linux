/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_E820_TYPES_H
#define _ASM_E820_TYPES_H

#include <uapi/asm/bootparam.h>


enum e820_type {
	E820_TYPE_RAM		= 1,
	E820_TYPE_RESERVED	= 2,
	E820_TYPE_ACPI		= 3,
	E820_TYPE_NVS		= 4,
	E820_TYPE_UNUSABLE	= 5,
	E820_TYPE_PMEM		= 7,

	
	E820_TYPE_PRAM		= 12,

	
	E820_TYPE_SOFT_RESERVED	= 0xefffffff,

	
	E820_TYPE_RESERVED_KERN	= 128,
};


struct e820_entry {
	u64			addr;
	u64			size;
	enum e820_type		type;
} __attribute__((packed));



#include <linux/numa.h>

#define E820_MAX_ENTRIES	(E820_MAX_ENTRIES_ZEROPAGE + 3*MAX_NUMNODES)


struct e820_table {
	__u32 nr_entries;
	struct e820_entry entries[E820_MAX_ENTRIES];
};


#define ISA_START_ADDRESS	0x000a0000
#define ISA_END_ADDRESS		0x00100000

#define BIOS_BEGIN		0x000a0000
#define BIOS_END		0x00100000

#define HIGH_MEMORY		0x00100000

#define BIOS_ROM_BASE		0xffe00000
#define BIOS_ROM_END		0xffffffff

#endif 
