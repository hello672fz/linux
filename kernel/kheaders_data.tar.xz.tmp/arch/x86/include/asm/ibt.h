/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_IBT_H
#define _ASM_X86_IBT_H

#include <linux/types.h>


#if defined(CONFIG_X86_KERNEL_IBT) && !defined(__DISABLE_EXPORTS)

#define HAS_KERNEL_IBT	1

#ifndef __ASSEMBLY__

#ifdef CONFIG_X86_64
#define ASM_ENDBR	"endbr64\n\t"
#else
#define ASM_ENDBR	"endbr32\n\t"
#endif

#define __noendbr	__attribute__((nocf_check))


#define IBT_NOSEAL(fname)				\
	".pushsection .discard.ibt_endbr_noseal\n\t"	\
	_ASM_PTR fname "\n\t"				\
	".popsection\n\t"

static inline __attribute_const__ u32 gen_endbr(void)
{
	u32 endbr;

	
	asm ( "mov $~0xfa1e0ff3, %[endbr]\n\t"
	      "not %[endbr]\n\t"
	       : [endbr] "=&r" (endbr) );

	return endbr;
}

static inline __attribute_const__ u32 gen_endbr_poison(void)
{
	
	return 0x001f0f66; 
}

static inline bool is_endbr(u32 val)
{
	if (val == gen_endbr_poison())
		return true;

	val &= ~0x01000000U; 
	return val == gen_endbr();
}

extern __noendbr u64 ibt_save(void);
extern __noendbr void ibt_restore(u64 save);

#else 

#ifdef CONFIG_X86_64
#define ENDBR	endbr64
#else
#define ENDBR	endbr32
#endif

#endif 

#else 

#define HAS_KERNEL_IBT	0

#ifndef __ASSEMBLY__

#define ASM_ENDBR
#define IBT_NOSEAL(name)

#define __noendbr

static inline bool is_endbr(u32 val) { return false; }

static inline u64 ibt_save(void) { return 0; }
static inline void ibt_restore(u64 save) { }

#else 

#define ENDBR

#endif 

#endif 

#define ENDBR_INSN_SIZE		(4*HAS_KERNEL_IBT)

#endif 
