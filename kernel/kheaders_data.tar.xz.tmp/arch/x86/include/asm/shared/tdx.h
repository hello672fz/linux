/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SHARED_TDX_H
#define _ASM_X86_SHARED_TDX_H

#include <linux/bits.h>
#include <linux/types.h>

#define TDX_HYPERCALL_STANDARD  0

#define TDX_HCALL_HAS_OUTPUT	BIT(0)
#define TDX_HCALL_ISSUE_STI	BIT(1)

#define TDX_CPUID_LEAF_ID	0x21
#define TDX_IDENT		"IntelTDX    "

#ifndef __ASSEMBLY__


struct tdx_hypercall_args {
	u64 r10;
	u64 r11;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;
};


u64 __tdx_hypercall(struct tdx_hypercall_args *args, unsigned long flags);


void __tdx_hypercall_failed(void);

#endif 
#endif 
