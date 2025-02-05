/* SPDX-License-Identifier: GPL-2.0 */


#ifndef _ASM_X86_HIGHMEM_H
#define _ASM_X86_HIGHMEM_H

#ifdef __KERNEL__

#include <linux/interrupt.h>
#include <linux/threads.h>
#include <asm/tlbflush.h>
#include <asm/paravirt.h>
#include <asm/fixmap.h>
#include <asm/pgtable_areas.h>


extern unsigned long highstart_pfn, highend_pfn;



#define LAST_PKMAP_MASK (LAST_PKMAP-1)
#define PKMAP_NR(virt)  ((virt-PKMAP_BASE) >> PAGE_SHIFT)
#define PKMAP_ADDR(nr)  (PKMAP_BASE + ((nr) << PAGE_SHIFT))

#define flush_cache_kmaps()	do { } while (0)

#define	arch_kmap_local_post_map(vaddr, pteval)		\
	arch_flush_lazy_mmu_mode()

#define	arch_kmap_local_post_unmap(vaddr)		\
	do {						\
		flush_tlb_one_kernel((vaddr));		\
		arch_flush_lazy_mmu_mode();		\
	} while (0)

extern void add_highpages_with_active_regions(int nid, unsigned long start_pfn,
					unsigned long end_pfn);

#endif 

#endif 
