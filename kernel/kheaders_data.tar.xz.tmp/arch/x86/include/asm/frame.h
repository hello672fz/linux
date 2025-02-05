/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_FRAME_H
#define _ASM_X86_FRAME_H

#include <asm/asm.h>



#ifdef CONFIG_FRAME_POINTER

#ifdef __ASSEMBLY__

.macro FRAME_BEGIN
	push %_ASM_BP
	_ASM_MOV %_ASM_SP, %_ASM_BP
.endm

.macro FRAME_END
	pop %_ASM_BP
.endm

#ifdef CONFIG_X86_64

.macro ENCODE_FRAME_POINTER ptregs_offset=0
	leaq 1+\ptregs_offset(%rsp), %rbp
.endm
#else 

.macro ENCODE_FRAME_POINTER
	mov %esp, %ebp
	andl $0x7fffffff, %ebp
.endm
#endif 

#else 

#define FRAME_BEGIN				\
	"push %" _ASM_BP "\n"			\
	_ASM_MOV "%" _ASM_SP ", %" _ASM_BP "\n"

#define FRAME_END "pop %" _ASM_BP "\n"

#ifdef CONFIG_X86_64

#define ENCODE_FRAME_POINTER			\
	"lea 1(%rsp), %rbp\n\t"

static inline unsigned long encode_frame_pointer(struct pt_regs *regs)
{
	return (unsigned long)regs + 1;
}

#else 

#define ENCODE_FRAME_POINTER			\
	"movl %esp, %ebp\n\t"			\
	"andl $0x7fffffff, %ebp\n\t"

static inline unsigned long encode_frame_pointer(struct pt_regs *regs)
{
	return (unsigned long)regs & 0x7fffffff;
}

#endif 

#endif 

#define FRAME_OFFSET __ASM_SEL(4, 8)

#else 

#ifdef __ASSEMBLY__

.macro ENCODE_FRAME_POINTER ptregs_offset=0
.endm

#else 

#define ENCODE_FRAME_POINTER

static inline unsigned long encode_frame_pointer(struct pt_regs *regs)
{
	return 0;
}

#endif

#define FRAME_BEGIN
#define FRAME_END
#define FRAME_OFFSET 0

#endif 

#endif 
