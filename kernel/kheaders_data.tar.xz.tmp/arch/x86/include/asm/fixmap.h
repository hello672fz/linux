

#ifndef _ASM_X86_FIXMAP_H
#define _ASM_X86_FIXMAP_H

#include <asm/kmap_size.h>


#ifndef CONFIG_DEBUG_KMAP_LOCAL_FORCE_MAP
# define FIXMAP_PMD_NUM	2
#else
# define KM_PMDS	(KM_MAX_IDX * ((CONFIG_NR_CPUS + 511) / 512))
# define FIXMAP_PMD_NUM (KM_PMDS + 2)
#endif

#define FIXMAP_PMD_TOP	507

#ifndef __ASSEMBLY__
#include <linux/kernel.h>
#include <asm/apicdef.h>
#include <asm/page.h>
#include <asm/pgtable_types.h>
#ifdef CONFIG_X86_32
#include <linux/threads.h>
#else
#include <uapi/asm/vsyscall.h>
#endif


#ifdef CONFIG_X86_32

extern unsigned long __FIXADDR_TOP;
#define FIXADDR_TOP	((unsigned long)__FIXADDR_TOP)
#else
#define FIXADDR_TOP	(round_up(VSYSCALL_ADDR + PAGE_SIZE, 1<<PMD_SHIFT) - \
			 PAGE_SIZE)
#endif


enum fixed_addresses {
#ifdef CONFIG_X86_32
	FIX_HOLE,
#else
#ifdef CONFIG_X86_VSYSCALL_EMULATION
	VSYSCALL_PAGE = (FIXADDR_TOP - VSYSCALL_ADDR) >> PAGE_SHIFT,
#endif
#endif
	FIX_DBGP_BASE,
	FIX_EARLYCON_MEM_BASE,
#ifdef CONFIG_PROVIDE_OHCI1394_DMA_INIT
	FIX_OHCI1394_BASE,
#endif
#ifdef CONFIG_X86_LOCAL_APIC
	FIX_APIC_BASE,	
#endif
#ifdef CONFIG_X86_IO_APIC
	FIX_IO_APIC_BASE_0,
	FIX_IO_APIC_BASE_END = FIX_IO_APIC_BASE_0 + MAX_IO_APICS - 1,
#endif
#ifdef CONFIG_KMAP_LOCAL
	FIX_KMAP_BEGIN,	
	FIX_KMAP_END = FIX_KMAP_BEGIN + (KM_MAX_IDX * NR_CPUS) - 1,
#ifdef CONFIG_PCI_MMCONFIG
	FIX_PCIE_MCFG,
#endif
#endif
#ifdef CONFIG_PARAVIRT_XXL
	FIX_PARAVIRT_BOOTMAP,
#endif

#ifdef CONFIG_ACPI_APEI_GHES
	
	FIX_APEI_GHES_IRQ,
	FIX_APEI_GHES_NMI,
#endif

	__end_of_permanent_fixed_addresses,

	
#define NR_FIX_BTMAPS		64
#define FIX_BTMAPS_SLOTS	8
#define TOTAL_FIX_BTMAPS	(NR_FIX_BTMAPS * FIX_BTMAPS_SLOTS)
	FIX_BTMAP_END =
	 (__end_of_permanent_fixed_addresses ^
	  (__end_of_permanent_fixed_addresses + TOTAL_FIX_BTMAPS - 1)) &
	 -PTRS_PER_PTE
	 ? __end_of_permanent_fixed_addresses + TOTAL_FIX_BTMAPS -
	   (__end_of_permanent_fixed_addresses & (TOTAL_FIX_BTMAPS - 1))
	 : __end_of_permanent_fixed_addresses,
	FIX_BTMAP_BEGIN = FIX_BTMAP_END + TOTAL_FIX_BTMAPS - 1,
#ifdef CONFIG_X86_32
	FIX_WP_TEST,
#endif
#ifdef CONFIG_INTEL_TXT
	FIX_TBOOT_BASE,
#endif
	__end_of_fixed_addresses
};


extern void reserve_top_address(unsigned long reserve);

#define FIXADDR_SIZE		(__end_of_permanent_fixed_addresses << PAGE_SHIFT)
#define FIXADDR_START		(FIXADDR_TOP - FIXADDR_SIZE)
#define FIXADDR_TOT_SIZE	(__end_of_fixed_addresses << PAGE_SHIFT)
#define FIXADDR_TOT_START	(FIXADDR_TOP - FIXADDR_TOT_SIZE)

extern int fixmaps_set;

extern pte_t *pkmap_page_table;

void __native_set_fixmap(enum fixed_addresses idx, pte_t pte);
void native_set_fixmap(unsigned  idx,
		       phys_addr_t phys, pgprot_t flags);

#ifndef CONFIG_PARAVIRT_XXL
static inline void __set_fixmap(enum fixed_addresses idx,
				phys_addr_t phys, pgprot_t flags)
{
	native_set_fixmap(idx, phys, flags);
}
#endif


#define FIXMAP_PAGE_NOCACHE PAGE_KERNEL_IO_NOCACHE


void __init *early_memremap_encrypted(resource_size_t phys_addr,
				      unsigned long size);
void __init *early_memremap_encrypted_wp(resource_size_t phys_addr,
					 unsigned long size);
void __init *early_memremap_decrypted(resource_size_t phys_addr,
				      unsigned long size);
void __init *early_memremap_decrypted_wp(resource_size_t phys_addr,
					 unsigned long size);

#include <asm-generic/fixmap.h>

#define __late_set_fixmap(idx, phys, flags) __set_fixmap(idx, phys, flags)
#define __late_clear_fixmap(idx) __set_fixmap(idx, 0, __pgprot(0))

void __early_set_fixmap(enum fixed_addresses idx,
			phys_addr_t phys, pgprot_t flags);

#endif 
#endif 
