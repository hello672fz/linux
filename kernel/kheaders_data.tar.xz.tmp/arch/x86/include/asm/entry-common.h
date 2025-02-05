/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_X86_ENTRY_COMMON_H
#define _ASM_X86_ENTRY_COMMON_H

#include <linux/randomize_kstack.h>
#include <linux/user-return-notifier.h>

#include <asm/nospec-branch.h>
#include <asm/io_bitmap.h>
#include <asm/fpu/api.h>


static __always_inline void arch_enter_from_user_mode(struct pt_regs *regs)
{
	if (IS_ENABLED(CONFIG_DEBUG_ENTRY)) {
		
		unsigned long flags = native_save_fl();
		unsigned long mask = X86_EFLAGS_DF | X86_EFLAGS_NT;

		
		if (boot_cpu_has(X86_FEATURE_SMAP) ||
		    (IS_ENABLED(CONFIG_64BIT) && boot_cpu_has(X86_FEATURE_XENPV)))
			mask |= X86_EFLAGS_AC;

		WARN_ON_ONCE(flags & mask);

		
		WARN_ON_ONCE(!user_mode(regs));

		
		WARN_ON_ONCE(!on_thread_stack());
		WARN_ON_ONCE(regs != task_pt_regs(current));
	}
}
#define arch_enter_from_user_mode arch_enter_from_user_mode

static inline void arch_exit_to_user_mode_prepare(struct pt_regs *regs,
						  unsigned long ti_work)
{
	if (ti_work & _TIF_USER_RETURN_NOTIFY)
		fire_user_return_notifiers();

	if (unlikely(ti_work & _TIF_IO_BITMAP))
		tss_update_io_bitmap();

	fpregs_assert_state_consistent();
	if (unlikely(ti_work & _TIF_NEED_FPU_LOAD))
		switch_fpu_return();

#ifdef CONFIG_COMPAT
	
	current_thread_info()->status &= ~(TS_COMPAT | TS_I386_REGS_POKED);
#endif

	
	choose_random_kstack_offset(rdtsc() & 0xFF);
}
#define arch_exit_to_user_mode_prepare arch_exit_to_user_mode_prepare

static __always_inline void arch_exit_to_user_mode(void)
{
	mds_user_clear_cpu_buffers();
}
#define arch_exit_to_user_mode arch_exit_to_user_mode

#endif
