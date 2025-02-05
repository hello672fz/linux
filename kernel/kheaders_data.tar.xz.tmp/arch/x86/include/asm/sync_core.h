/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SYNC_CORE_H
#define _ASM_X86_SYNC_CORE_H

#include <linux/preempt.h>
#include <asm/processor.h>
#include <asm/cpufeature.h>
#include <asm/special_insns.h>

#ifdef CONFIG_X86_32
static inline void iret_to_self(void)
{
	asm volatile (
		"pushfl\n\t"
		"pushl %%cs\n\t"
		"pushl $1f\n\t"
		"iret\n\t"
		"1:"
		: ASM_CALL_CONSTRAINT : : "memory");
}
#else
static inline void iret_to_self(void)
{
	unsigned int tmp;

	asm volatile (
		"mov %%ss, %0\n\t"
		"pushq %q0\n\t"
		"pushq %%rsp\n\t"
		"addq $8, (%%rsp)\n\t"
		"pushfq\n\t"
		"mov %%cs, %0\n\t"
		"pushq %q0\n\t"
		"pushq $1f\n\t"
		"iretq\n\t"
		"1:"
		: "=&r" (tmp), ASM_CALL_CONSTRAINT : : "cc", "memory");
}
#endif 


static inline void sync_core(void)
{
	
	if (static_cpu_has(X86_FEATURE_SERIALIZE)) {
		serialize();
		return;
	}

	
	iret_to_self();
}


static inline void sync_core_before_usermode(void)
{
	
	if (static_cpu_has(X86_FEATURE_PTI))
		return;

	
	sync_core();
}

#endif 
