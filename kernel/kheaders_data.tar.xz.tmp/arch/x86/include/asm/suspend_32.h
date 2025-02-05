/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_X86_SUSPEND_32_H
#define _ASM_X86_SUSPEND_32_H

#include <asm/desc.h>
#include <asm/fpu/api.h>


struct saved_context {
	
	u16 gs;
	unsigned long cr0, cr2, cr3, cr4;
	u64 misc_enable;
	struct saved_msrs saved_msrs;
	struct desc_ptr gdt_desc;
	struct desc_ptr idt;
	u16 ldt;
	u16 tss;
	unsigned long tr;
	unsigned long safety;
	unsigned long return_address;
	bool misc_enable_saved;
} __attribute__((packed));


extern char core_restore_code[];
extern char restore_registers[];

#endif 
