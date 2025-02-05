/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_IOBITMAP_H
#define _ASM_X86_IOBITMAP_H

#include <linux/refcount.h>
#include <asm/processor.h>

struct io_bitmap {
	u64		sequence;
	refcount_t	refcnt;
	
	unsigned int	max;
	unsigned long	bitmap[IO_BITMAP_LONGS];
};

struct task_struct;

#ifdef CONFIG_X86_IOPL_IOPERM
void io_bitmap_share(struct task_struct *tsk);
void io_bitmap_exit(struct task_struct *tsk);

static inline void native_tss_invalidate_io_bitmap(void)
{
	
	this_cpu_write(cpu_tss_rw.x86_tss.io_bitmap_base,
		       IO_BITMAP_OFFSET_INVALID);
}

void native_tss_update_io_bitmap(void);

#ifdef CONFIG_PARAVIRT_XXL
#include <asm/paravirt.h>
#else
#define tss_update_io_bitmap native_tss_update_io_bitmap
#define tss_invalidate_io_bitmap native_tss_invalidate_io_bitmap
#endif

#else
static inline void io_bitmap_share(struct task_struct *tsk) { }
static inline void io_bitmap_exit(struct task_struct *tsk) { }
static inline void tss_update_io_bitmap(void) { }
#endif

#endif
