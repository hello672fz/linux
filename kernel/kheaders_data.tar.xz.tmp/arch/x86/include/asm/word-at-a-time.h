/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_WORD_AT_A_TIME_H
#define _ASM_WORD_AT_A_TIME_H

#include <linux/kernel.h>


struct word_at_a_time {
	const unsigned long one_bits, high_bits;
};

#define WORD_AT_A_TIME_CONSTANTS { REPEAT_BYTE(0x01), REPEAT_BYTE(0x80) }

#ifdef CONFIG_64BIT


static inline long count_masked_bytes(unsigned long mask)
{
	return mask*0x0001020304050608ul >> 56;
}

#else	


static inline long count_masked_bytes(long mask)
{
	
	long a = (0x0ff0001+mask) >> 23;
	
	return a & mask;
}

#endif


static inline unsigned long has_zero(unsigned long a, unsigned long *bits, const struct word_at_a_time *c)
{
	unsigned long mask = ((a - c->one_bits) & ~a) & c->high_bits;
	*bits = mask;
	return mask;
}

static inline unsigned long prep_zero_mask(unsigned long a, unsigned long bits, const struct word_at_a_time *c)
{
	return bits;
}

static inline unsigned long create_zero_mask(unsigned long bits)
{
	bits = (bits - 1) & ~bits;
	return bits >> 7;
}


#define zero_bytemask(mask) (mask)

static inline unsigned long find_zero(unsigned long mask)
{
	return count_masked_bytes(mask);
}


static inline unsigned long load_unaligned_zeropad(const void *addr)
{
	unsigned long ret;

	asm volatile(
		"1:	mov %[mem], %[ret]\n"
		"2:\n"
		_ASM_EXTABLE_TYPE(1b, 2b, EX_TYPE_ZEROPAD)
		: [ret] "=r" (ret)
		: [mem] "m" (*(unsigned long *)addr));

	return ret;
}

#endif 
