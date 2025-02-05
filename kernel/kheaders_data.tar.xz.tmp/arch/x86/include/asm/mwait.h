/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_MWAIT_H
#define _ASM_X86_MWAIT_H

#include <linux/sched.h>
#include <linux/sched/idle.h>

#include <asm/cpufeature.h>
#include <asm/nospec-branch.h>

#define MWAIT_SUBSTATE_MASK		0xf
#define MWAIT_CSTATE_MASK		0xf
#define MWAIT_SUBSTATE_SIZE		4
#define MWAIT_HINT2CSTATE(hint)		(((hint) >> MWAIT_SUBSTATE_SIZE) & MWAIT_CSTATE_MASK)
#define MWAIT_HINT2SUBSTATE(hint)	((hint) & MWAIT_CSTATE_MASK)
#define MWAIT_C1_SUBSTATE_MASK  0xf0

#define CPUID_MWAIT_LEAF		5
#define CPUID5_ECX_EXTENSIONS_SUPPORTED 0x1
#define CPUID5_ECX_INTERRUPT_BREAK	0x2

#define MWAIT_ECX_INTERRUPT_BREAK	0x1
#define MWAITX_ECX_TIMER_ENABLE		BIT(1)
#define MWAITX_MAX_WAIT_CYCLES		UINT_MAX
#define MWAITX_DISABLE_CSTATES		0xf0
#define TPAUSE_C01_STATE		1
#define TPAUSE_C02_STATE		0

static inline void __monitor(const void *eax, unsigned long ecx,
			     unsigned long edx)
{
	
	asm volatile(".byte 0x0f, 0x01, 0xc8;"
		     :: "a" (eax), "c" (ecx), "d"(edx));
}

static inline void __monitorx(const void *eax, unsigned long ecx,
			      unsigned long edx)
{
	
	asm volatile(".byte 0x0f, 0x01, 0xfa;"
		     :: "a" (eax), "c" (ecx), "d"(edx));
}

static inline void __mwait(unsigned long eax, unsigned long ecx)
{
	mds_idle_clear_cpu_buffers();

	
	asm volatile(".byte 0x0f, 0x01, 0xc9;"
		     :: "a" (eax), "c" (ecx));
}


static inline void __mwaitx(unsigned long eax, unsigned long ebx,
			    unsigned long ecx)
{
	

	
	asm volatile(".byte 0x0f, 0x01, 0xfb;"
		     :: "a" (eax), "b" (ebx), "c" (ecx));
}

static inline void __sti_mwait(unsigned long eax, unsigned long ecx)
{
	mds_idle_clear_cpu_buffers();
	
	asm volatile("sti; .byte 0x0f, 0x01, 0xc9;"
		     :: "a" (eax), "c" (ecx));
}


static inline void mwait_idle_with_hints(unsigned long eax, unsigned long ecx)
{
	if (static_cpu_has_bug(X86_BUG_MONITOR) || !current_set_polling_and_test()) {
		if (static_cpu_has_bug(X86_BUG_CLFLUSH_MONITOR)) {
			mb();
			clflush((void *)&current_thread_info()->flags);
			mb();
		}

		__monitor((void *)&current_thread_info()->flags, 0, 0);
		if (!need_resched())
			__mwait(eax, ecx);
	}
	current_clr_polling();
}


static inline void __tpause(u32 ecx, u32 edx, u32 eax)
{
	
	#ifdef CONFIG_AS_TPAUSE
	asm volatile("tpause %%ecx\n"
		     :
		     : "c"(ecx), "d"(edx), "a"(eax));
	#else
	asm volatile(".byte 0x66, 0x0f, 0xae, 0xf1\t\n"
		     :
		     : "c"(ecx), "d"(edx), "a"(eax));
	#endif
}

#endif 
