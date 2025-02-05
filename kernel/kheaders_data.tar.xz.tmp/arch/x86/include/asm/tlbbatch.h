/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ARCH_X86_TLBBATCH_H
#define _ARCH_X86_TLBBATCH_H

#include <linux/cpumask.h>

struct arch_tlbflush_unmap_batch {
	
	struct cpumask cpumask;
};

#endif 
