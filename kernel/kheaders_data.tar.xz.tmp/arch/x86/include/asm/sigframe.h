/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SIGFRAME_H
#define _ASM_X86_SIGFRAME_H

#include <uapi/asm/sigcontext.h>
#include <asm/siginfo.h>
#include <asm/ucontext.h>
#include <linux/compat.h>

#ifdef CONFIG_X86_32
#define sigframe_ia32		sigframe
#define rt_sigframe_ia32	rt_sigframe
#define ucontext_ia32		ucontext
#else 

#ifdef CONFIG_IA32_EMULATION
#include <asm/ia32.h>
#endif 

#endif 

#if defined(CONFIG_X86_32) || defined(CONFIG_IA32_EMULATION)
struct sigframe_ia32 {
	u32 pretcode;
	int sig;
	struct sigcontext_32 sc;
	
	struct _fpstate_32 fpstate_unused;
	unsigned int extramask[1];
	char retcode[8];
	
};

struct rt_sigframe_ia32 {
	u32 pretcode;
	int sig;
	u32 pinfo;
	u32 puc;
#ifdef CONFIG_IA32_EMULATION
	compat_siginfo_t info;
#else 
	struct siginfo info;
#endif 
	struct ucontext_ia32 uc;
	char retcode[8];
	
};
#endif 

#ifdef CONFIG_X86_64

struct rt_sigframe {
	char __user *pretcode;
	struct ucontext uc;
	struct siginfo info;
	
};

#ifdef CONFIG_X86_X32_ABI

struct ucontext_x32 {
	unsigned int	  uc_flags;
	unsigned int 	  uc_link;
	compat_stack_t	  uc_stack;
	unsigned int	  uc__pad0;     
	struct sigcontext uc_mcontext;  
	compat_sigset_t	  uc_sigmask;	
};

struct rt_sigframe_x32 {
	u64 pretcode;
	struct ucontext_x32 uc;
	compat_siginfo_t info;
	
};

#endif 

#endif 

void __init init_sigframe_size(void);

#endif 
