

#ifndef _ASM_X86_XEN_HYPERVISOR_H
#define _ASM_X86_XEN_HYPERVISOR_H

extern struct shared_info *HYPERVISOR_shared_info;
extern struct start_info *xen_start_info;

#include <asm/processor.h>

static inline uint32_t xen_cpuid_base(void)
{
	return hypervisor_cpuid_base("XenVMMXenVMM", 2);
}

struct pci_dev;

#ifdef CONFIG_XEN_PV_DOM0
bool xen_initdom_restore_msi(struct pci_dev *dev);
#else
static inline bool xen_initdom_restore_msi(struct pci_dev *dev) { return true; }
#endif

#ifdef CONFIG_HOTPLUG_CPU
void xen_arch_register_cpu(int num);
void xen_arch_unregister_cpu(int num);
#endif

#ifdef CONFIG_PVH
void __init xen_pvh_init(struct boot_params *boot_params);
void __init mem_map_via_hcall(struct boot_params *boot_params_p);
#endif

#endif 
