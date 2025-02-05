/* SPDX-License-Identifier: GPL-2.0 */



#ifndef _ASM_X86_JAILHOUSE_PARA_H
#define _ASM_X86_JAILHOUSE_PARA_H

#include <linux/types.h>

#ifdef CONFIG_JAILHOUSE_GUEST
bool jailhouse_paravirt(void);
#else
static inline bool jailhouse_paravirt(void)
{
	return false;
}
#endif

#endif 
