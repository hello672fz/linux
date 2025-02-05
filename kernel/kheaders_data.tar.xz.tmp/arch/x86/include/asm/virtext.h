/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_X86_VIRTEX_H
#define _ASM_X86_VIRTEX_H

#include <asm/processor.h>

#include <asm/vmx.h>
#include <asm/svm.h>
#include <asm/tlbflush.h>



static inline int cpu_has_vmx(void)
{
	unsigned long ecx = cpuid_ecx(1);
	return test_bit(5, &ecx); 
}



static inline int cpu_vmxoff(void)
{
	asm_volatile_goto("1: vmxoff\n\t"
			  _ASM_EXTABLE(1b, %l[fault])
			  ::: "cc", "memory" : fault);

	cr4_clear_bits(X86_CR4_VMXE);
	return 0;

fault:
	cr4_clear_bits(X86_CR4_VMXE);
	return -EIO;
}

static inline int cpu_vmx_enabled(void)
{
	return __read_cr4() & X86_CR4_VMXE;
}


static inline void __cpu_emergency_vmxoff(void)
{
	if (cpu_vmx_enabled())
		cpu_vmxoff();
}


static inline void cpu_emergency_vmxoff(void)
{
	if (cpu_has_vmx())
		__cpu_emergency_vmxoff();
}







static inline int cpu_has_svm(const char **msg)
{
	if (boot_cpu_data.x86_vendor != X86_VENDOR_AMD &&
	    boot_cpu_data.x86_vendor != X86_VENDOR_HYGON) {
		if (msg)
			*msg = "not amd or hygon";
		return 0;
	}

	if (boot_cpu_data.extended_cpuid_level < SVM_CPUID_FUNC) {
		if (msg)
			*msg = "can't execute cpuid_8000000a";
		return 0;
	}

	if (!boot_cpu_has(X86_FEATURE_SVM)) {
		if (msg)
			*msg = "svm not available";
		return 0;
	}
	return 1;
}



static inline void cpu_svm_disable(void)
{
	uint64_t efer;

	wrmsrl(MSR_VM_HSAVE_PA, 0);
	rdmsrl(MSR_EFER, efer);
	wrmsrl(MSR_EFER, efer & ~EFER_SVME);
}


static inline void cpu_emergency_svm_disable(void)
{
	if (cpu_has_svm(NULL))
		cpu_svm_disable();
}

#endif 
