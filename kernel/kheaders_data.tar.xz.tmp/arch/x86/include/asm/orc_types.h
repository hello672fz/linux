/* SPDX-License-Identifier: GPL-2.0-or-later */


#ifndef _ORC_TYPES_H
#define _ORC_TYPES_H

#include <linux/types.h>
#include <linux/compiler.h>


#define ORC_REG_UNDEFINED		0
#define ORC_REG_PREV_SP			1
#define ORC_REG_DX			2
#define ORC_REG_DI			3
#define ORC_REG_BP			4
#define ORC_REG_SP			5
#define ORC_REG_R10			6
#define ORC_REG_R13			7
#define ORC_REG_BP_INDIRECT		8
#define ORC_REG_SP_INDIRECT		9
#define ORC_REG_MAX			15

#ifndef __ASSEMBLY__
#include <asm/byteorder.h>


struct orc_entry {
	s16		sp_offset;
	s16		bp_offset;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned	sp_reg:4;
	unsigned	bp_reg:4;
	unsigned	type:2;
	unsigned	end:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
	unsigned	bp_reg:4;
	unsigned	sp_reg:4;
	unsigned	unused:5;
	unsigned	end:1;
	unsigned	type:2;
#endif
} __packed;

#endif 

#endif 
