/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_FTRACE_H
#define _ASM_X86_FTRACE_H

#ifdef CONFIG_FUNCTION_TRACER
#ifndef CC_USING_FENTRY
# error Compiler does not support fentry?
#endif
# define MCOUNT_ADDR		((unsigned long)(__fentry__))
#define MCOUNT_INSN_SIZE	5 


#ifdef CONFIG_HAVE_FENTRY
# include <asm/ibt.h>

# define FTRACE_MCOUNT_MAX_OFFSET	ENDBR_INSN_SIZE
#endif

#ifdef CONFIG_DYNAMIC_FTRACE
#define ARCH_SUPPORTS_FTRACE_OPS 1
#endif

#define HAVE_FUNCTION_GRAPH_RET_ADDR_PTR

#ifndef __ASSEMBLY__
extern void __fentry__(void);

static inline unsigned long ftrace_call_adjust(unsigned long addr)
{
	
	return addr;
}


static inline void arch_ftrace_set_direct_caller(struct pt_regs *regs, unsigned long addr)
{
	
	regs->orig_ax = addr;
}

#ifdef CONFIG_HAVE_DYNAMIC_FTRACE_WITH_ARGS
struct ftrace_regs {
	struct pt_regs		regs;
};

static __always_inline struct pt_regs *
arch_ftrace_get_regs(struct ftrace_regs *fregs)
{
	
	if (!fregs->regs.cs)
		return NULL;
	return &fregs->regs;
}

#define ftrace_instruction_pointer_set(fregs, _ip)	\
	do { (fregs)->regs.ip = (_ip); } while (0)

struct ftrace_ops;
#define ftrace_graph_func ftrace_graph_func
void ftrace_graph_func(unsigned long ip, unsigned long parent_ip,
		       struct ftrace_ops *op, struct ftrace_regs *fregs);
#else
#define FTRACE_GRAPH_TRAMP_ADDR FTRACE_GRAPH_ADDR
#endif

#ifdef CONFIG_DYNAMIC_FTRACE

struct dyn_arch_ftrace {
	
};

#endif 
#endif 
#endif 


#ifndef __ASSEMBLY__

#if defined(CONFIG_FUNCTION_TRACER) && defined(CONFIG_DYNAMIC_FTRACE)
extern void set_ftrace_ops_ro(void);
#else
static inline void set_ftrace_ops_ro(void) { }
#endif

#define ARCH_HAS_SYSCALL_MATCH_SYM_NAME
static inline bool arch_syscall_match_sym_name(const char *sym, const char *name)
{
	
	return !strcmp(sym + 3, name + 3) ||
		(!strncmp(sym, "__x64_", 6) && !strcmp(sym + 9, name + 3)) ||
		(!strncmp(sym, "__ia32_", 7) && !strcmp(sym + 10, name + 3)) ||
		(!strncmp(sym, "__do_sys", 8) && !strcmp(sym + 8, name + 3));
}

#ifndef COMPILE_OFFSETS

#if defined(CONFIG_FTRACE_SYSCALLS) && defined(CONFIG_IA32_EMULATION)
#include <linux/compat.h>


#define ARCH_TRACE_IGNORE_COMPAT_SYSCALLS 1
static inline bool arch_trace_is_compat_syscall(struct pt_regs *regs)
{
	return in_32bit_syscall();
}
#endif 
#endif 
#endif 

#endif 
