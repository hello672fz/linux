/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ARCH_X86_REALMODE_H
#define _ARCH_X86_REALMODE_H


#define TH_FLAGS_SME_ACTIVE_BIT		0
#define TH_FLAGS_SME_ACTIVE		BIT(TH_FLAGS_SME_ACTIVE_BIT)

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <asm/io.h>


struct real_mode_header {
	u32	text_start;
	u32	ro_end;
	
	u32	trampoline_start;
	u32	trampoline_header;
#ifdef CONFIG_AMD_MEM_ENCRYPT
	u32	sev_es_trampoline_start;
#endif
#ifdef CONFIG_X86_64
	u32	trampoline_start64;
	u32	trampoline_pgd;
#endif
	
#ifdef CONFIG_ACPI_SLEEP
	u32	wakeup_start;
	u32	wakeup_header;
#endif
	
	u32	machine_real_restart_asm;
#ifdef CONFIG_X86_64
	u32	machine_real_restart_seg;
#endif
};


struct trampoline_header {
#ifdef CONFIG_X86_32
	u32 start;
	u16 gdt_pad;
	u16 gdt_limit;
	u32 gdt_base;
#else
	u64 start;
	u64 efer;
	u32 cr4;
	u32 flags;
#endif
};

extern struct real_mode_header *real_mode_header;
extern unsigned char real_mode_blob_end[];

extern unsigned long initial_code;
extern unsigned long initial_gs;
extern unsigned long initial_stack;
#ifdef CONFIG_AMD_MEM_ENCRYPT
extern unsigned long initial_vc_handler;
#endif

extern unsigned char real_mode_blob[];
extern unsigned char real_mode_relocs[];

#ifdef CONFIG_X86_32
extern unsigned char startup_32_smp[];
extern unsigned char boot_gdt[];
#else
extern unsigned char secondary_startup_64[];
extern unsigned char secondary_startup_64_no_verify[];
#endif

static inline size_t real_mode_size_needed(void)
{
	if (real_mode_header)
		return 0;	

	return ALIGN(real_mode_blob_end - real_mode_blob, PAGE_SIZE);
}

static inline void set_real_mode_mem(phys_addr_t mem)
{
	real_mode_header = (struct real_mode_header *) __va(mem);
}

void reserve_real_mode(void);
void load_trampoline_pgtable(void);

#endif 

#endif 
