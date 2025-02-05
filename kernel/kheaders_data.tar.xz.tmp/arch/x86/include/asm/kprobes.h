/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _ASM_X86_KPROBES_H
#define _ASM_X86_KPROBES_H


#include <asm-generic/kprobes.h>

#ifdef CONFIG_KPROBES
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/percpu.h>
#include <asm/text-patching.h>
#include <asm/insn.h>

#define  __ARCH_WANT_KPROBES_INSN_SLOT

struct pt_regs;
struct kprobe;

typedef u8 kprobe_opcode_t;

#define MAX_STACK_SIZE 64
#define CUR_STACK_SIZE(ADDR) \
	(current_top_of_stack() - (unsigned long)(ADDR))
#define MIN_STACK_SIZE(ADDR)				\
	(MAX_STACK_SIZE < CUR_STACK_SIZE(ADDR) ?	\
	 MAX_STACK_SIZE : CUR_STACK_SIZE(ADDR))

#define flush_insn_slot(p)	do { } while (0)


extern __visible kprobe_opcode_t optprobe_template_entry[];
extern __visible kprobe_opcode_t optprobe_template_clac[];
extern __visible kprobe_opcode_t optprobe_template_val[];
extern __visible kprobe_opcode_t optprobe_template_call[];
extern __visible kprobe_opcode_t optprobe_template_end[];
#define MAX_OPTIMIZED_LENGTH (MAX_INSN_SIZE + DISP32_SIZE)
#define MAX_OPTINSN_SIZE 				\
	(((unsigned long)optprobe_template_end -	\
	  (unsigned long)optprobe_template_entry) +	\
	 MAX_OPTIMIZED_LENGTH + JMP32_INSN_SIZE)

extern const int kretprobe_blacklist_size;

void arch_remove_kprobe(struct kprobe *p);


struct arch_specific_insn {
	
	kprobe_opcode_t *insn;
	
	unsigned boostable:1;
	unsigned char size;	
	union {
		unsigned char opcode;
		struct {
			unsigned char type;
		} jcc;
		struct {
			unsigned char type;
			unsigned char asize;
		} loop;
		struct {
			unsigned char reg;
		} indirect;
	};
	s32 rel32;	
	void (*emulate_op)(struct kprobe *p, struct pt_regs *regs);
	
	int tp_len;
};

struct arch_optimized_insn {
	
	kprobe_opcode_t copied_insn[DISP32_SIZE];
	
	kprobe_opcode_t *insn;
	
	size_t size;
};


static inline int arch_prepared_optinsn(struct arch_optimized_insn *optinsn)
{
	return optinsn->size;
}

struct prev_kprobe {
	struct kprobe *kp;
	unsigned long status;
	unsigned long old_flags;
	unsigned long saved_flags;
};


struct kprobe_ctlblk {
	unsigned long kprobe_status;
	unsigned long kprobe_old_flags;
	unsigned long kprobe_saved_flags;
	struct prev_kprobe prev_kprobe;
};

extern int kprobe_fault_handler(struct pt_regs *regs, int trapnr);
extern int kprobe_exceptions_notify(struct notifier_block *self,
				    unsigned long val, void *data);
extern int kprobe_int3_handler(struct pt_regs *regs);

#else

static inline int kprobe_debug_handler(struct pt_regs *regs) { return 0; }

#endif 
#endif 
