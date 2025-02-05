/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _ORC_LOOKUP_H
#define _ORC_LOOKUP_H


#define LOOKUP_BLOCK_ORDER	8
#define LOOKUP_BLOCK_SIZE	(1 << LOOKUP_BLOCK_ORDER)

#ifndef LINKER_SCRIPT

extern unsigned int orc_lookup[];
extern unsigned int orc_lookup_end[];

#define LOOKUP_START_IP		(unsigned long)_stext
#define LOOKUP_STOP_IP		(unsigned long)_etext

#endif 

#endif 
