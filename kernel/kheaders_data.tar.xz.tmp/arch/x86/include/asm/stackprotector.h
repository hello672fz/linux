/* SPDX-License-Identifier: GPL-2.0 */


#ifndef _ASM_STACKPROTECTOR_H
#define _ASM_STACKPROTECTOR_H 1

#ifdef CONFIG_STACKPROTECTOR

#include <asm/tsc.h>
#include <asm/processor.h>
#include <asm/percpu.h>
#include <asm/desc.h>

#include <linux/random.h>
#include <linux/sched.h>


static __always_inline void boot_init_stack_canary(void)
{
	u64 canary;
	u64 tsc;

#ifdef CONFIG_X86_64
	BUILD_BUG_ON(offsetof(struct fixed_percpu_data, stack_canary) != 40);
#endif
	
	get_random_bytes(&canary, sizeof(canary));
	tsc = rdtsc();
	canary += tsc + (tsc << 32UL);
	canary &= CANARY_MASK;

	current->stack_canary = canary;
#ifdef CONFIG_X86_64
	this_cpu_write(fixed_percpu_data.stack_canary, canary);
#else
	this_cpu_write(__stack_chk_guard, canary);
#endif
}

static inline void cpu_init_stack_canary(int cpu, struct task_struct *idle)
{
#ifdef CONFIG_X86_64
	per_cpu(fixed_percpu_data.stack_canary, cpu) = idle->stack_canary;
#else
	per_cpu(__stack_chk_guard, cpu) = idle->stack_canary;
#endif
}

#else	



static inline void cpu_init_stack_canary(int cpu, struct task_struct *idle)
{ }

#endif	
#endif	
