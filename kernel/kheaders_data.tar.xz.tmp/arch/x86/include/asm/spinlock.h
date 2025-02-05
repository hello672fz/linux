/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SPINLOCK_H
#define _ASM_X86_SPINLOCK_H

#include <linux/jump_label.h>
#include <linux/atomic.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <linux/compiler.h>
#include <asm/paravirt.h>
#include <asm/bitops.h>




#define SPIN_THRESHOLD	(1 << 15)

#include <asm/qspinlock.h>



#include <asm/qrwlock.h>

#endif 
