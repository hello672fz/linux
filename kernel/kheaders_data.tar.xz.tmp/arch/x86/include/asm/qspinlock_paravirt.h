/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_QSPINLOCK_PARAVIRT_H
#define __ASM_QSPINLOCK_PARAVIRT_H

#include <asm/ibt.h>


#ifdef CONFIG_64BIT

__PV_CALLEE_SAVE_REGS_THUNK(__pv_queued_spin_unlock_slowpath, ".spinlock.text");
#define __pv_queued_spin_unlock	__pv_queued_spin_unlock
#define PV_UNLOCK		"__raw_callee_save___pv_queued_spin_unlock"
#define PV_UNLOCK_SLOWPATH	"__raw_callee_save___pv_queued_spin_unlock_slowpath"


asm    (".pushsection .spinlock.text, \"ax\";"
	".globl " PV_UNLOCK ";"
	".type " PV_UNLOCK ", @function;"
	".align 4,0x90;"
	PV_UNLOCK ": "
	ASM_ENDBR
	FRAME_BEGIN
	"push  %rdx;"
	"mov   $0x1,%eax;"
	"xor   %edx,%edx;"
	LOCK_PREFIX "cmpxchg %dl,(%rdi);"
	"cmp   $0x1,%al;"
	"jne   .slowpath;"
	"pop   %rdx;"
	FRAME_END
	ASM_RET
	".slowpath: "
	"push   %rsi;"
	"movzbl %al,%esi;"
	"call " PV_UNLOCK_SLOWPATH ";"
	"pop    %rsi;"
	"pop    %rdx;"
	FRAME_END
	ASM_RET
	".size " PV_UNLOCK ", .-" PV_UNLOCK ";"
	".popsection");

#else 

extern void __lockfunc __pv_queued_spin_unlock(struct qspinlock *lock);
__PV_CALLEE_SAVE_REGS_THUNK(__pv_queued_spin_unlock, ".spinlock.text");

#endif 
#endif
