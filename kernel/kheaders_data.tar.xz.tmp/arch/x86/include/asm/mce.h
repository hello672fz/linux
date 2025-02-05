/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_MCE_H
#define _ASM_X86_MCE_H

#include <uapi/asm/mce.h>




#define MCG_BANKCNT_MASK	0xff         
#define MCG_CTL_P		BIT_ULL(8)   
#define MCG_EXT_P		BIT_ULL(9)   
#define MCG_CMCI_P		BIT_ULL(10)  
#define MCG_EXT_CNT_MASK	0xff0000     
#define MCG_EXT_CNT_SHIFT	16
#define MCG_EXT_CNT(c)		(((c) & MCG_EXT_CNT_MASK) >> MCG_EXT_CNT_SHIFT)
#define MCG_SER_P		BIT_ULL(24)  
#define MCG_ELOG_P		BIT_ULL(26)  
#define MCG_LMCE_P		BIT_ULL(27)  


#define MCG_STATUS_RIPV		BIT_ULL(0)   
#define MCG_STATUS_EIPV		BIT_ULL(1)   
#define MCG_STATUS_MCIP		BIT_ULL(2)   
#define MCG_STATUS_LMCES	BIT_ULL(3)   


#define MCG_EXT_CTL_LMCE_EN	BIT_ULL(0) 


#define MCI_STATUS_VAL		BIT_ULL(63)  
#define MCI_STATUS_OVER		BIT_ULL(62)  
#define MCI_STATUS_UC		BIT_ULL(61)  
#define MCI_STATUS_EN		BIT_ULL(60)  
#define MCI_STATUS_MISCV	BIT_ULL(59)  
#define MCI_STATUS_ADDRV	BIT_ULL(58)  
#define MCI_STATUS_PCC		BIT_ULL(57)  
#define MCI_STATUS_S		BIT_ULL(56)  
#define MCI_STATUS_AR		BIT_ULL(55)  
#define MCI_STATUS_CEC_SHIFT	38           
#define MCI_STATUS_CEC_MASK	GENMASK_ULL(52,38)
#define MCI_STATUS_CEC(c)	(((c) & MCI_STATUS_CEC_MASK) >> MCI_STATUS_CEC_SHIFT)
#define MCI_STATUS_MSCOD(m)	(((m) >> 16) & 0xffff)


#define MCI_STATUS_TCC		BIT_ULL(55)  
#define MCI_STATUS_SYNDV	BIT_ULL(53)  
#define MCI_STATUS_DEFERRED	BIT_ULL(44)  
#define MCI_STATUS_POISON	BIT_ULL(43)  
#define MCI_STATUS_SCRUB	BIT_ULL(40)  


#define MCI_CONFIG_MCAX		0x1
#define MCI_IPID_MCATYPE	0xFFFF0000
#define MCI_IPID_HWID		0xFFF


#define MCACOD		  0xefff     


#define MCACOD_SCRUB	0x00C0	
#define MCACOD_SCRUBMSK	0xeff0	
#define MCACOD_L3WB	0x017A	
#define MCACOD_DATA	0x0134	
#define MCACOD_INSTR	0x0150	


#define MCI_MISC_ADDR_LSB(m)	((m) & 0x3f)
#define MCI_MISC_ADDR_MODE(m)	(((m) >> 6) & 7)
#define  MCI_MISC_ADDR_SEGOFF	0	
#define  MCI_MISC_ADDR_LINEAR	1	
#define  MCI_MISC_ADDR_PHYS	2	
#define  MCI_MISC_ADDR_MEM	3	
#define  MCI_MISC_ADDR_GENERIC	7	


#define MCI_CTL2_CMCI_EN		BIT_ULL(30)
#define MCI_CTL2_CMCI_THRESHOLD_MASK	0x7fffULL

#define MCJ_CTX_MASK		3
#define MCJ_CTX(flags)		((flags) & MCJ_CTX_MASK)
#define MCJ_CTX_RANDOM		0    
#define MCJ_CTX_PROCESS		0x1  
#define MCJ_CTX_IRQ		0x2  
#define MCJ_NMI_BROADCAST	0x4  
#define MCJ_EXCEPTION		0x8  
#define MCJ_IRQ_BROADCAST	0x10 

#define MCE_OVERFLOW 0		

#define MCE_LOG_MIN_LEN 32U
#define MCE_LOG_SIGNATURE	"MACHINECHECK"


#define MSR_AMD64_SMCA_MC0_CTL		0xc0002000
#define MSR_AMD64_SMCA_MC0_STATUS	0xc0002001
#define MSR_AMD64_SMCA_MC0_ADDR		0xc0002002
#define MSR_AMD64_SMCA_MC0_MISC0	0xc0002003
#define MSR_AMD64_SMCA_MC0_CONFIG	0xc0002004
#define MSR_AMD64_SMCA_MC0_IPID		0xc0002005
#define MSR_AMD64_SMCA_MC0_SYND		0xc0002006
#define MSR_AMD64_SMCA_MC0_DESTAT	0xc0002008
#define MSR_AMD64_SMCA_MC0_DEADDR	0xc0002009
#define MSR_AMD64_SMCA_MC0_MISC1	0xc000200a
#define MSR_AMD64_SMCA_MCx_CTL(x)	(MSR_AMD64_SMCA_MC0_CTL + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_STATUS(x)	(MSR_AMD64_SMCA_MC0_STATUS + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_ADDR(x)	(MSR_AMD64_SMCA_MC0_ADDR + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_MISC(x)	(MSR_AMD64_SMCA_MC0_MISC0 + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_CONFIG(x)	(MSR_AMD64_SMCA_MC0_CONFIG + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_IPID(x)	(MSR_AMD64_SMCA_MC0_IPID + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_SYND(x)	(MSR_AMD64_SMCA_MC0_SYND + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_DESTAT(x)	(MSR_AMD64_SMCA_MC0_DESTAT + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_DEADDR(x)	(MSR_AMD64_SMCA_MC0_DEADDR + 0x10*(x))
#define MSR_AMD64_SMCA_MCx_MISCy(x, y)	((MSR_AMD64_SMCA_MC0_MISC1 + y) + (0x10*(x)))

#define XEC(x, mask)			(((x) >> 16) & mask)


#define	MCE_HANDLED_CEC		BIT_ULL(0)
#define	MCE_HANDLED_UC		BIT_ULL(1)
#define	MCE_HANDLED_EXTLOG	BIT_ULL(2)
#define	MCE_HANDLED_NFIT	BIT_ULL(3)
#define	MCE_HANDLED_EDAC	BIT_ULL(4)
#define	MCE_HANDLED_MCELOG	BIT_ULL(5)


#define MCE_IN_KERNEL_RECOV	BIT_ULL(6)


#define MCE_IN_KERNEL_COPYIN	BIT_ULL(7)


struct mce_log_buffer {
	char signature[12]; 
	unsigned len;	    
	unsigned next;
	unsigned flags;
	unsigned recordlen;	
	struct mce entry[];
};


enum mce_notifier_prios {
	MCE_PRIO_LOWEST,
	MCE_PRIO_MCELOG,
	MCE_PRIO_EDAC,
	MCE_PRIO_NFIT,
	MCE_PRIO_EXTLOG,
	MCE_PRIO_UC,
	MCE_PRIO_EARLY,
	MCE_PRIO_CEC,
	MCE_PRIO_HIGHEST = MCE_PRIO_CEC
};

struct notifier_block;
extern void mce_register_decode_chain(struct notifier_block *nb);
extern void mce_unregister_decode_chain(struct notifier_block *nb);

#include <linux/percpu.h>
#include <linux/atomic.h>

extern int mce_p5_enabled;

#ifdef CONFIG_ARCH_HAS_COPY_MC
extern void enable_copy_mc_fragile(void);
unsigned long __must_check copy_mc_fragile(void *dst, const void *src, unsigned cnt);
#else
static inline void enable_copy_mc_fragile(void)
{
}
#endif

struct cper_ia_proc_ctx;

#ifdef CONFIG_X86_MCE
int mcheck_init(void);
void mcheck_cpu_init(struct cpuinfo_x86 *c);
void mcheck_cpu_clear(struct cpuinfo_x86 *c);
int apei_smca_report_x86_error(struct cper_ia_proc_ctx *ctx_info,
			       u64 lapic_id);
#else
static inline int mcheck_init(void) { return 0; }
static inline void mcheck_cpu_init(struct cpuinfo_x86 *c) {}
static inline void mcheck_cpu_clear(struct cpuinfo_x86 *c) {}
static inline int apei_smca_report_x86_error(struct cper_ia_proc_ctx *ctx_info,
					     u64 lapic_id) { return -EINVAL; }
#endif

void mce_setup(struct mce *m);
void mce_log(struct mce *m);
DECLARE_PER_CPU(struct device *, mce_device);


#define MAX_NR_BANKS 64

#ifdef CONFIG_X86_MCE_INTEL
void mce_intel_feature_init(struct cpuinfo_x86 *c);
void mce_intel_feature_clear(struct cpuinfo_x86 *c);
void cmci_clear(void);
void cmci_reenable(void);
void cmci_rediscover(void);
void cmci_recheck(void);
#else
static inline void mce_intel_feature_init(struct cpuinfo_x86 *c) { }
static inline void mce_intel_feature_clear(struct cpuinfo_x86 *c) { }
static inline void cmci_clear(void) {}
static inline void cmci_reenable(void) {}
static inline void cmci_rediscover(void) {}
static inline void cmci_recheck(void) {}
#endif

int mce_available(struct cpuinfo_x86 *c);
bool mce_is_memory_error(struct mce *m);
bool mce_is_correctable(struct mce *m);
int mce_usable_address(struct mce *m);

DECLARE_PER_CPU(unsigned, mce_exception_count);
DECLARE_PER_CPU(unsigned, mce_poll_count);

typedef DECLARE_BITMAP(mce_banks_t, MAX_NR_BANKS);
DECLARE_PER_CPU(mce_banks_t, mce_poll_banks);

enum mcp_flags {
	MCP_TIMESTAMP	= BIT(0),	
	MCP_UC		= BIT(1),	
	MCP_DONTLOG	= BIT(2),	
	MCP_QUEUE_LOG	= BIT(3),	
};
bool machine_check_poll(enum mcp_flags flags, mce_banks_t *b);

int mce_notify_irq(void);

DECLARE_PER_CPU(struct mce, injectm);


extern void mce_disable_bank(int bank);


void do_machine_check(struct pt_regs *pt_regs);


extern void (*mce_threshold_vector)(void);


extern void (*deferred_error_int_vector)(void);



struct cper_sec_mem_err;
extern void apei_mce_report_mem_error(int corrected,
				      struct cper_sec_mem_err *mem_err);


#ifdef CONFIG_X86_MCE_AMD


enum smca_bank_types {
	SMCA_LS = 0,	
	SMCA_LS_V2,
	SMCA_IF,	
	SMCA_L2_CACHE,	
	SMCA_DE,	
	SMCA_RESERVED,	
	SMCA_EX,	
	SMCA_FP,	
	SMCA_L3_CACHE,	
	SMCA_CS,	
	SMCA_CS_V2,
	SMCA_PIE,	
	SMCA_UMC,	
	SMCA_UMC_V2,
	SMCA_PB,	
	SMCA_PSP,	
	SMCA_PSP_V2,
	SMCA_SMU,	
	SMCA_SMU_V2,
	SMCA_MP5,	
	SMCA_MPDMA,	
	SMCA_NBIO,	
	SMCA_PCIE,	
	SMCA_PCIE_V2,
	SMCA_XGMI_PCS,	
	SMCA_NBIF,	
	SMCA_SHUB,	
	SMCA_SATA,	
	SMCA_USB,	
	SMCA_GMI_PCS,	
	SMCA_XGMI_PHY,	
	SMCA_WAFL_PHY,	
	SMCA_GMI_PHY,	
	N_SMCA_BANK_TYPES
};

extern const char *smca_get_long_name(enum smca_bank_types t);
extern bool amd_mce_is_memory_error(struct mce *m);

extern int mce_threshold_create_device(unsigned int cpu);
extern int mce_threshold_remove_device(unsigned int cpu);

void mce_amd_feature_init(struct cpuinfo_x86 *c);
enum smca_bank_types smca_get_bank_type(unsigned int cpu, unsigned int bank);
#else

static inline int mce_threshold_create_device(unsigned int cpu)		{ return 0; };
static inline int mce_threshold_remove_device(unsigned int cpu)		{ return 0; };
static inline bool amd_mce_is_memory_error(struct mce *m)		{ return false; };
static inline void mce_amd_feature_init(struct cpuinfo_x86 *c)		{ }
#endif

static inline void mce_hygon_feature_init(struct cpuinfo_x86 *c)	{ return mce_amd_feature_init(c); }
#endif 
