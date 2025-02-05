

#ifndef _ASM_X86_XEN_INTERFACE_H
#define _ASM_X86_XEN_INTERFACE_H


#ifdef __XEN__
#define __DEFINE_GUEST_HANDLE(name, type) \
    typedef struct { type *p; } __guest_handle_ ## name
#else
#define __DEFINE_GUEST_HANDLE(name, type) \
    typedef type * __guest_handle_ ## name
#endif

#define DEFINE_GUEST_HANDLE_STRUCT(name) \
	__DEFINE_GUEST_HANDLE(name, struct name)
#define DEFINE_GUEST_HANDLE(name) __DEFINE_GUEST_HANDLE(name, name)
#define GUEST_HANDLE(name)        __guest_handle_ ## name

#ifdef __XEN__
#if defined(__i386__)
#define set_xen_guest_handle(hnd, val)			\
	do {						\
		if (sizeof(hnd) == 8)			\
			*(uint64_t *)&(hnd) = 0;	\
		(hnd).p = val;				\
	} while (0)
#elif defined(__x86_64__)
#define set_xen_guest_handle(hnd, val)	do { (hnd).p = val; } while (0)
#endif
#else
#if defined(__i386__)
#define set_xen_guest_handle(hnd, val)			\
	do {						\
		if (sizeof(hnd) == 8)			\
			*(uint64_t *)&(hnd) = 0;	\
		(hnd) = val;				\
	} while (0)
#elif defined(__x86_64__)
#define set_xen_guest_handle(hnd, val)	do { (hnd) = val; } while (0)
#endif
#endif

#ifndef __ASSEMBLY__

typedef unsigned long xen_pfn_t;
#define PRI_xen_pfn "lx"
typedef unsigned long xen_ulong_t;
#define PRI_xen_ulong "lx"
typedef long xen_long_t;
#define PRI_xen_long "lx"


__DEFINE_GUEST_HANDLE(uchar, unsigned char);
__DEFINE_GUEST_HANDLE(uint,  unsigned int);
DEFINE_GUEST_HANDLE(char);
DEFINE_GUEST_HANDLE(int);
DEFINE_GUEST_HANDLE(void);
DEFINE_GUEST_HANDLE(uint64_t);
DEFINE_GUEST_HANDLE(uint32_t);
DEFINE_GUEST_HANDLE(xen_pfn_t);
DEFINE_GUEST_HANDLE(xen_ulong_t);
#endif

#ifndef HYPERVISOR_VIRT_START
#define HYPERVISOR_VIRT_START mk_unsigned_long(__HYPERVISOR_VIRT_START)
#endif

#define MACH2PHYS_VIRT_START  mk_unsigned_long(__MACH2PHYS_VIRT_START)
#define MACH2PHYS_VIRT_END    mk_unsigned_long(__MACH2PHYS_VIRT_END)
#define MACH2PHYS_NR_ENTRIES  ((MACH2PHYS_VIRT_END-MACH2PHYS_VIRT_START)>>__MACH2PHYS_SHIFT)


#define MAX_VIRT_CPUS 32



#define FIRST_RESERVED_GDT_PAGE  14
#define FIRST_RESERVED_GDT_BYTE  (FIRST_RESERVED_GDT_PAGE * 4096)
#define FIRST_RESERVED_GDT_ENTRY (FIRST_RESERVED_GDT_BYTE / 8)


#define TI_GET_DPL(_ti)		((_ti)->flags & 3)
#define TI_GET_IF(_ti)		((_ti)->flags & 4)
#define TI_SET_DPL(_ti, _dpl)	((_ti)->flags |= (_dpl))
#define TI_SET_IF(_ti, _if)	((_ti)->flags |= ((!!(_if))<<2))

#ifndef __ASSEMBLY__
struct trap_info {
    uint8_t       vector;  
    uint8_t       flags;   
    uint16_t      cs;      
    unsigned long address; 
};
DEFINE_GUEST_HANDLE_STRUCT(trap_info);

struct arch_shared_info {
	
	unsigned long max_pfn;
	
	xen_pfn_t pfn_to_mfn_frame_list_list;
	unsigned long nmi_reason;
	
	unsigned long p2m_cr3;		
	unsigned long p2m_vaddr;	
	unsigned long p2m_generation;	
#ifdef CONFIG_X86_32
	uint32_t wc_sec_hi;
#endif
};
#endif	

#ifdef CONFIG_X86_32
#include <asm/xen/interface_32.h>
#else
#include <asm/xen/interface_64.h>
#endif

#include <asm/pvclock-abi.h>

#ifndef __ASSEMBLY__

struct vcpu_guest_context {
    
    struct { char x[512]; } fpu_ctxt;       
#define VGCF_I387_VALID                (1<<0)
#define VGCF_IN_KERNEL                 (1<<2)
#define _VGCF_i387_valid               0
#define VGCF_i387_valid                (1<<_VGCF_i387_valid)
#define _VGCF_in_kernel                2
#define VGCF_in_kernel                 (1<<_VGCF_in_kernel)
#define _VGCF_failsafe_disables_events 3
#define VGCF_failsafe_disables_events  (1<<_VGCF_failsafe_disables_events)
#define _VGCF_syscall_disables_events  4
#define VGCF_syscall_disables_events   (1<<_VGCF_syscall_disables_events)
#define _VGCF_online                   5
#define VGCF_online                    (1<<_VGCF_online)
    unsigned long flags;                    
    struct cpu_user_regs user_regs;         
    struct trap_info trap_ctxt[256];        
    unsigned long ldt_base, ldt_ents;       
    unsigned long gdt_frames[16], gdt_ents; 
    unsigned long kernel_ss, kernel_sp;     
    
    unsigned long ctrlreg[8];               
    unsigned long debugreg[8];              
#ifdef __i386__
    unsigned long event_callback_cs;        
    unsigned long event_callback_eip;
    unsigned long failsafe_callback_cs;     
    unsigned long failsafe_callback_eip;
#else
    unsigned long event_callback_eip;
    unsigned long failsafe_callback_eip;
    unsigned long syscall_callback_eip;
#endif
    unsigned long vm_assist;                
#ifdef __x86_64__
    
    uint64_t      fs_base;
    uint64_t      gs_base_kernel;
    uint64_t      gs_base_user;
#endif
};
DEFINE_GUEST_HANDLE_STRUCT(vcpu_guest_context);


struct xen_pmu_amd_ctxt {
	
	uint32_t counters;
	uint32_t ctrls;

	
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
	uint64_t regs[];
#elif defined(__GNUC__)
	uint64_t regs[0];
#endif
};


struct xen_pmu_cntr_pair {
	uint64_t counter;
	uint64_t control;
};

struct xen_pmu_intel_ctxt {
	
	uint32_t fixed_counters;
	uint32_t arch_counters;

	
	uint64_t global_ctrl;
	uint64_t global_ovf_ctrl;
	uint64_t global_status;
	uint64_t fixed_ctrl;
	uint64_t ds_area;
	uint64_t pebs_enable;
	uint64_t debugctl;

	
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
	uint64_t regs[];
#elif defined(__GNUC__)
	uint64_t regs[0];
#endif
};


struct xen_pmu_regs {
	uint64_t ip;
	uint64_t sp;
	uint64_t flags;
	uint16_t cs;
	uint16_t ss;
	uint8_t cpl;
	uint8_t pad[3];
};


#define PMU_CACHED	   (1<<0) 
#define PMU_SAMPLE_USER	   (1<<1) 
#define PMU_SAMPLE_REAL	   (1<<2) 
#define PMU_SAMPLE_PV	   (1<<3) 


struct xen_pmu_arch {
	union {
		
		struct xen_pmu_regs regs;
		
#define XENPMU_REGS_PAD_SZ  64
		uint8_t pad[XENPMU_REGS_PAD_SZ];
	} r;

	
	uint64_t pmu_flags;

	
	union {
		uint32_t lapic_lvtpc;
		uint64_t pad;
	} l;

	
	union {
		struct xen_pmu_amd_ctxt amd;
		struct xen_pmu_intel_ctxt intel;

		
#define XENPMU_CTXT_PAD_SZ  128
		uint8_t pad[XENPMU_CTXT_PAD_SZ];
	} c;
};

#endif	


#include <asm/emulate_prefix.h>

#define XEN_EMULATE_PREFIX __ASM_FORM(.byte __XEN_EMULATE_PREFIX ;)
#define XEN_CPUID          XEN_EMULATE_PREFIX __ASM_FORM(cpuid)

#endif 
