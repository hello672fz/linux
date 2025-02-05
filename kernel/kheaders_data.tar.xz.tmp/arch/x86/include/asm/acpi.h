/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _ASM_X86_ACPI_H
#define _ASM_X86_ACPI_H


#include <acpi/pdc_intel.h>

#include <asm/numa.h>
#include <asm/fixmap.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/mpspec.h>
#include <asm/x86_init.h>

#ifdef CONFIG_ACPI_APEI
# include <asm/pgtable_types.h>
#endif

#ifdef CONFIG_ACPI
extern int acpi_lapic;
extern int acpi_ioapic;
extern int acpi_noirq;
extern int acpi_strict;
extern int acpi_disabled;
extern int acpi_pci_disabled;
extern int acpi_skip_timer_override;
extern int acpi_use_timer_override;
extern int acpi_fix_pin2_polarity;
extern int acpi_disable_cmcff;

extern u8 acpi_sci_flags;
extern u32 acpi_sci_override_gsi;
void acpi_pic_sci_set_trigger(unsigned int, u16);

struct device;

extern int (*__acpi_register_gsi)(struct device *dev, u32 gsi,
				  int trigger, int polarity);
extern void (*__acpi_unregister_gsi)(u32 gsi);

static inline void disable_acpi(void)
{
	acpi_disabled = 1;
	acpi_pci_disabled = 1;
	acpi_noirq = 1;
}

extern int acpi_gsi_to_irq(u32 gsi, unsigned int *irq);

static inline void acpi_noirq_set(void) { acpi_noirq = 1; }
static inline void acpi_disable_pci(void)
{
	acpi_pci_disabled = 1;
	acpi_noirq_set();
}


extern int (*acpi_suspend_lowlevel)(void);


unsigned long acpi_get_wakeup_address(void);


static inline unsigned int acpi_processor_cstate_check(unsigned int max_cstate)
{
	
	if (boot_cpu_data.x86 == 0x0F &&
	    boot_cpu_data.x86_vendor == X86_VENDOR_AMD &&
	    boot_cpu_data.x86_model <= 0x05 &&
	    boot_cpu_data.x86_stepping < 0x0A)
		return 1;
	else if (boot_cpu_has(X86_BUG_AMD_APIC_C1E))
		return 1;
	else
		return max_cstate;
}

static inline bool arch_has_acpi_pdc(void)
{
	struct cpuinfo_x86 *c = &cpu_data(0);
	return (c->x86_vendor == X86_VENDOR_INTEL ||
		c->x86_vendor == X86_VENDOR_CENTAUR);
}

static inline void arch_acpi_set_pdc_bits(u32 *buf)
{
	struct cpuinfo_x86 *c = &cpu_data(0);

	buf[2] |= ACPI_PDC_C_CAPABILITY_SMP;

	if (cpu_has(c, X86_FEATURE_EST))
		buf[2] |= ACPI_PDC_EST_CAPABILITY_SWSMP;

	if (cpu_has(c, X86_FEATURE_ACPI))
		buf[2] |= ACPI_PDC_T_FFH;

	
	if (!cpu_has(c, X86_FEATURE_MWAIT))
		buf[2] &= ~(ACPI_PDC_C_C2C3_FFH);
}

static inline bool acpi_has_cpu_in_madt(void)
{
	return !!acpi_lapic;
}

#define ACPI_HAVE_ARCH_SET_ROOT_POINTER
static inline void acpi_arch_set_root_pointer(u64 addr)
{
	x86_init.acpi.set_root_pointer(addr);
}

#define ACPI_HAVE_ARCH_GET_ROOT_POINTER
static inline u64 acpi_arch_get_root_pointer(void)
{
	return x86_init.acpi.get_root_pointer();
}

void acpi_generic_reduced_hw_init(void);

void x86_default_set_root_pointer(u64 addr);
u64 x86_default_get_root_pointer(void);

#else 

#define acpi_lapic 0
#define acpi_ioapic 0
#define acpi_disable_cmcff 0
static inline void acpi_noirq_set(void) { }
static inline void acpi_disable_pci(void) { }
static inline void disable_acpi(void) { }

static inline void acpi_generic_reduced_hw_init(void) { }

static inline void x86_default_set_root_pointer(u64 addr) { }

static inline u64 x86_default_get_root_pointer(void)
{
	return 0;
}

#endif 

#define ARCH_HAS_POWER_INIT	1

#ifdef CONFIG_ACPI_NUMA
extern int x86_acpi_numa_init(void);
#endif 

struct cper_ia_proc_ctx;

#ifdef CONFIG_ACPI_APEI
static inline pgprot_t arch_apei_get_mem_attribute(phys_addr_t addr)
{
	
	return PAGE_KERNEL_NOENC;
}

int arch_apei_report_x86_error(struct cper_ia_proc_ctx *ctx_info,
			       u64 lapic_id);
#else
static inline int arch_apei_report_x86_error(struct cper_ia_proc_ctx *ctx_info,
					     u64 lapic_id)
{
	return -EINVAL;
}
#endif

#define ACPI_TABLE_UPGRADE_MAX_PHYS (max_low_pfn_mapped << PAGE_SHIFT)

#endif 
