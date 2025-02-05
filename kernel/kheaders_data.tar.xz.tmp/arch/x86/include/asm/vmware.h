/* SPDX-License-Identifier: GPL-2.0 or MIT */
#ifndef _ASM_X86_VMWARE_H
#define _ASM_X86_VMWARE_H

#include <asm/cpufeatures.h>
#include <asm/alternative.h>
#include <linux/stringify.h>




#define VMWARE_HYPERVISOR_PORT    0x5658
#define VMWARE_HYPERVISOR_PORT_HB 0x5659


#define VMWARE_HYPERVISOR_HB   BIT(0)
#define VMWARE_HYPERVISOR_OUT  BIT(1)


#define VMWARE_HYPERCALL						\
	ALTERNATIVE_2("movw $" __stringify(VMWARE_HYPERVISOR_PORT) ", %%dx; " \
		      "inl (%%dx), %%eax",				\
		      "vmcall", X86_FEATURE_VMCALL,			\
		      "vmmcall", X86_FEATURE_VMW_VMMCALL)


#define VMWARE_HYPERCALL_HB_OUT						\
	ALTERNATIVE_2("movw $" __stringify(VMWARE_HYPERVISOR_PORT_HB) ", %%dx; " \
		      "rep outsb",					\
		      "vmcall", X86_FEATURE_VMCALL,			\
		      "vmmcall", X86_FEATURE_VMW_VMMCALL)


#define VMWARE_HYPERCALL_HB_IN						\
	ALTERNATIVE_2("movw $" __stringify(VMWARE_HYPERVISOR_PORT_HB) ", %%dx; " \
		      "rep insb",					\
		      "vmcall", X86_FEATURE_VMCALL,			\
		      "vmmcall", X86_FEATURE_VMW_VMMCALL)
#endif
