/* SPDX-License-Identifier: GPL-2.0 */


#include <asm/pc-conf-reg.h>

static inline u8 getCx86(u8 reg)
{
	return pc_conf_get(reg);
}

static inline void setCx86(u8 reg, u8 data)
{
	pc_conf_set(reg, data);
}
