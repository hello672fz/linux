/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_X86_SUSPEND_64_H
#define _ASM_X86_SUSPEND_64_H

#include <asm/desc.h>
#include <asm/fpu/api.h>


struct saved_context {
	struct pt_regs regs;

	
	u16 ds, es, fs, gs;

	
	unsigned long kernelmode_gs_base, usermode_gs_base, fs_base;

	unsigned long cr0, cr2, cr3, cr4;
	u64 misc_enable;
	struct saved_msrs saved_msrs;
	unsigned long efer;
	u16 gdt_pad; 
	struct desc_ptr gdt_desc;
	u16 idt_pad;
	struct desc_ptr idt;
	u16 ldt;
	u16 tss;
	unsigned long tr;
	unsigned long safety;
	unsigned long return_address;
	bool misc_enable_saved;
} __attribute__((packed));

#define loaddebug(thread,register) \
	set_debugreg((thread)->debugreg##register, register)


extern char core_restore_code[];
extern char restore_registers[];

#endif 
