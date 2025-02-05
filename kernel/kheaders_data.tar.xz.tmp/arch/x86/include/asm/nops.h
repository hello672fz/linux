/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_NOPS_H
#define _ASM_X86_NOPS_H

#include <asm/asm.h>



#ifndef CONFIG_64BIT


#define BYTES_NOP1	0x90
#define BYTES_NOP2	0x89,0xf6
#define BYTES_NOP3	0x8d,0x76,0x00
#define BYTES_NOP4	0x8d,0x74,0x26,0x00
#define BYTES_NOP5	0x3e,BYTES_NOP4
#define BYTES_NOP6	0x8d,0xb6,0x00,0x00,0x00,0x00
#define BYTES_NOP7	0x8d,0xb4,0x26,0x00,0x00,0x00,0x00
#define BYTES_NOP8	0x3e,BYTES_NOP7

#else


#define BYTES_NOP1	0x90
#define BYTES_NOP2	0x66,BYTES_NOP1
#define BYTES_NOP3	0x0f,0x1f,0x00
#define BYTES_NOP4	0x0f,0x1f,0x40,0x00
#define BYTES_NOP5	0x0f,0x1f,0x44,0x00,0x00
#define BYTES_NOP6	0x66,BYTES_NOP5
#define BYTES_NOP7	0x0f,0x1f,0x80,0x00,0x00,0x00,0x00
#define BYTES_NOP8	0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00

#endif 

#define ASM_NOP1 _ASM_BYTES(BYTES_NOP1)
#define ASM_NOP2 _ASM_BYTES(BYTES_NOP2)
#define ASM_NOP3 _ASM_BYTES(BYTES_NOP3)
#define ASM_NOP4 _ASM_BYTES(BYTES_NOP4)
#define ASM_NOP5 _ASM_BYTES(BYTES_NOP5)
#define ASM_NOP6 _ASM_BYTES(BYTES_NOP6)
#define ASM_NOP7 _ASM_BYTES(BYTES_NOP7)
#define ASM_NOP8 _ASM_BYTES(BYTES_NOP8)

#define ASM_NOP_MAX 8

#ifndef __ASSEMBLY__
extern const unsigned char * const x86_nops[];
#endif

#endif 
