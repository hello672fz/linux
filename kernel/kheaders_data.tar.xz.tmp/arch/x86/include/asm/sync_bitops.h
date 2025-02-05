/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SYNC_BITOPS_H
#define _ASM_X86_SYNC_BITOPS_H





#include <asm/rmwcc.h>

#define ADDR (*(volatile long *)addr)


static inline void sync_set_bit(long nr, volatile unsigned long *addr)
{
	asm volatile("lock; " __ASM_SIZE(bts) " %1,%0"
		     : "+m" (ADDR)
		     : "Ir" (nr)
		     : "memory");
}


static inline void sync_clear_bit(long nr, volatile unsigned long *addr)
{
	asm volatile("lock; " __ASM_SIZE(btr) " %1,%0"
		     : "+m" (ADDR)
		     : "Ir" (nr)
		     : "memory");
}


static inline void sync_change_bit(long nr, volatile unsigned long *addr)
{
	asm volatile("lock; " __ASM_SIZE(btc) " %1,%0"
		     : "+m" (ADDR)
		     : "Ir" (nr)
		     : "memory");
}


static inline bool sync_test_and_set_bit(long nr, volatile unsigned long *addr)
{
	return GEN_BINARY_RMWcc("lock; " __ASM_SIZE(bts), *addr, c, "Ir", nr);
}


static inline int sync_test_and_clear_bit(long nr, volatile unsigned long *addr)
{
	return GEN_BINARY_RMWcc("lock; " __ASM_SIZE(btr), *addr, c, "Ir", nr);
}


static inline int sync_test_and_change_bit(long nr, volatile unsigned long *addr)
{
	return GEN_BINARY_RMWcc("lock; " __ASM_SIZE(btc), *addr, c, "Ir", nr);
}

#define sync_test_bit(nr, addr) test_bit(nr, addr)

#undef ADDR

#endif 
