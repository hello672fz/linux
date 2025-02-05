/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PTRACE_H
#define _ASM_X86_PTRACE_H

#include <asm/segment.h>
#include <asm/page_types.h>
#include <uapi/asm/ptrace.h>

#ifndef __ASSEMBLY__
#ifdef __i386__

struct pt_regs {
	
	unsigned long bx;
	unsigned long cx;
	unsigned long dx;
	unsigned long si;
	unsigned long di;
	unsigned long bp;
	unsigned long ax;
	unsigned short ds;
	unsigned short __dsh;
	unsigned short es;
	unsigned short __esh;
	unsigned short fs;
	unsigned short __fsh;
	
	unsigned short gs;
	unsigned short __gsh;
	
	unsigned long orig_ax;
	unsigned long ip;
	unsigned short cs;
	unsigned short __csh;
	unsigned long flags;
	unsigned long sp;
	unsigned short ss;
	unsigned short __ssh;
};

#else 

struct pt_regs {

	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long bp;
	unsigned long bx;

	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long ax;
	unsigned long cx;
	unsigned long dx;
	unsigned long si;
	unsigned long di;

	unsigned long orig_ax;

	unsigned long ip;
	unsigned long cs;
	unsigned long flags;
	unsigned long sp;
	unsigned long ss;

};

#endif 

#ifdef CONFIG_PARAVIRT
#include <asm/paravirt_types.h>
#endif

#include <asm/proto.h>

struct cpuinfo_x86;
struct task_struct;

extern unsigned long profile_pc(struct pt_regs *regs);

extern unsigned long
convert_ip_to_linear(struct task_struct *child, struct pt_regs *regs);
extern void send_sigtrap(struct pt_regs *regs, int error_code, int si_code);


static inline unsigned long regs_return_value(struct pt_regs *regs)
{
	return regs->ax;
}

static inline void regs_set_return_value(struct pt_regs *regs, unsigned long rc)
{
	regs->ax = rc;
}


static __always_inline int user_mode(struct pt_regs *regs)
{
#ifdef CONFIG_X86_32
	return ((regs->cs & SEGMENT_RPL_MASK) | (regs->flags & X86_VM_MASK)) >= USER_RPL;
#else
	return !!(regs->cs & 3);
#endif
}

static __always_inline int v8086_mode(struct pt_regs *regs)
{
#ifdef CONFIG_X86_32
	return (regs->flags & X86_VM_MASK);
#else
	return 0;	
#endif
}

static inline bool user_64bit_mode(struct pt_regs *regs)
{
#ifdef CONFIG_X86_64
#ifndef CONFIG_PARAVIRT_XXL
	
	return regs->cs == __USER_CS;
#else
	
	return regs->cs == __USER_CS || regs->cs == pv_info.extra_user_64bit_cs;
#endif
#else 
	return false;
#endif
}


static inline bool any_64bit_mode(struct pt_regs *regs)
{
#ifdef CONFIG_X86_64
	return !user_mode(regs) || user_64bit_mode(regs);
#else
	return false;
#endif
}

#ifdef CONFIG_X86_64
#define current_user_stack_pointer()	current_pt_regs()->sp
#define compat_user_stack_pointer()	current_pt_regs()->sp

static __always_inline bool ip_within_syscall_gap(struct pt_regs *regs)
{
	bool ret = (regs->ip >= (unsigned long)entry_SYSCALL_64 &&
		    regs->ip <  (unsigned long)entry_SYSCALL_64_safe_stack);

	ret = ret || (regs->ip >= (unsigned long)entry_SYSRETQ_unsafe_stack &&
		      regs->ip <  (unsigned long)entry_SYSRETQ_end);
#ifdef CONFIG_IA32_EMULATION
	ret = ret || (regs->ip >= (unsigned long)entry_SYSCALL_compat &&
		      regs->ip <  (unsigned long)entry_SYSCALL_compat_safe_stack);
	ret = ret || (regs->ip >= (unsigned long)entry_SYSRETL_compat_unsafe_stack &&
		      regs->ip <  (unsigned long)entry_SYSRETL_compat_end);
#endif

	return ret;
}
#endif

static inline unsigned long kernel_stack_pointer(struct pt_regs *regs)
{
	return regs->sp;
}

static inline unsigned long instruction_pointer(struct pt_regs *regs)
{
	return regs->ip;
}

static inline void instruction_pointer_set(struct pt_regs *regs,
		unsigned long val)
{
	regs->ip = val;
}

static inline unsigned long frame_pointer(struct pt_regs *regs)
{
	return regs->bp;
}

static inline unsigned long user_stack_pointer(struct pt_regs *regs)
{
	return regs->sp;
}

static inline void user_stack_pointer_set(struct pt_regs *regs,
		unsigned long val)
{
	regs->sp = val;
}

static __always_inline bool regs_irqs_disabled(struct pt_regs *regs)
{
	return !(regs->flags & X86_EFLAGS_IF);
}


extern int regs_query_register_offset(const char *name);
extern const char *regs_query_register_name(unsigned int offset);
#define MAX_REG_OFFSET (offsetof(struct pt_regs, ss))


static inline unsigned long regs_get_register(struct pt_regs *regs,
					      unsigned int offset)
{
	if (unlikely(offset > MAX_REG_OFFSET))
		return 0;
#ifdef CONFIG_X86_32
	
	if (offset == offsetof(struct pt_regs, cs) ||
	    offset == offsetof(struct pt_regs, ss) ||
	    offset == offsetof(struct pt_regs, ds) ||
	    offset == offsetof(struct pt_regs, es) ||
	    offset == offsetof(struct pt_regs, fs) ||
	    offset == offsetof(struct pt_regs, gs)) {
		return *(u16 *)((unsigned long)regs + offset);

	}
#endif
	return *(unsigned long *)((unsigned long)regs + offset);
}


static inline int regs_within_kernel_stack(struct pt_regs *regs,
					   unsigned long addr)
{
	return ((addr & ~(THREAD_SIZE - 1)) == (regs->sp & ~(THREAD_SIZE - 1)));
}


static inline unsigned long *regs_get_kernel_stack_nth_addr(struct pt_regs *regs, unsigned int n)
{
	unsigned long *addr = (unsigned long *)regs->sp;

	addr += n;
	if (regs_within_kernel_stack(regs, (unsigned long)addr))
		return addr;
	else
		return NULL;
}


extern long copy_from_kernel_nofault(void *dst, const void *src, size_t size);


static inline unsigned long regs_get_kernel_stack_nth(struct pt_regs *regs,
						      unsigned int n)
{
	unsigned long *addr;
	unsigned long val;
	long ret;

	addr = regs_get_kernel_stack_nth_addr(regs, n);
	if (addr) {
		ret = copy_from_kernel_nofault(&val, addr, sizeof(val));
		if (!ret)
			return val;
	}
	return 0;
}


static inline unsigned long regs_get_kernel_argument(struct pt_regs *regs,
						     unsigned int n)
{
	static const unsigned int argument_offs[] = {
#ifdef __i386__
		offsetof(struct pt_regs, ax),
		offsetof(struct pt_regs, dx),
		offsetof(struct pt_regs, cx),
#define NR_REG_ARGUMENTS 3
#else
		offsetof(struct pt_regs, di),
		offsetof(struct pt_regs, si),
		offsetof(struct pt_regs, dx),
		offsetof(struct pt_regs, cx),
		offsetof(struct pt_regs, r8),
		offsetof(struct pt_regs, r9),
#define NR_REG_ARGUMENTS 6
#endif
	};

	if (n >= NR_REG_ARGUMENTS) {
		n -= NR_REG_ARGUMENTS - 1;
		return regs_get_kernel_stack_nth(regs, n);
	} else
		return regs_get_register(regs, argument_offs[n]);
}

#define arch_has_single_step()	(1)
#ifdef CONFIG_X86_DEBUGCTLMSR
#define arch_has_block_step()	(1)
#else
#define arch_has_block_step()	(boot_cpu_data.x86 >= 6)
#endif

#define ARCH_HAS_USER_SINGLE_STEP_REPORT

struct user_desc;
extern int do_get_thread_area(struct task_struct *p, int idx,
			      struct user_desc __user *info);
extern int do_set_thread_area(struct task_struct *p, int idx,
			      struct user_desc __user *info, int can_allocate);

#ifdef CONFIG_X86_64
# define do_set_thread_area_64(p, s, t)	do_arch_prctl_64(p, s, t)
#else
# define do_set_thread_area_64(p, s, t)	(0)
#endif

#endif 
#endif 
