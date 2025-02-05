/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_BIOS_EBDA_H
#define _ASM_X86_BIOS_EBDA_H

#include <asm/io.h>


static inline unsigned int get_bios_ebda(void)
{
	
	unsigned int address = *(unsigned short *)phys_to_virt(0x40E);
	address <<= 4;
	return address;	
}

void reserve_bios_regions(void);

#ifdef CONFIG_X86_CHECK_BIOS_CORRUPTION

void check_for_bios_corruption(void);
void start_periodic_check_for_corruption(void);
#else
static inline void check_for_bios_corruption(void)
{
}

static inline void start_periodic_check_for_corruption(void)
{
}
#endif

#endif 
