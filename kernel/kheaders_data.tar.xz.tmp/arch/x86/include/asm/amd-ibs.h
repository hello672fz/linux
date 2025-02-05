/* SPDX-License-Identifier: GPL-2.0 */


#include <asm/msr-index.h>


#define IBS_DATA_SRC_LOC_CACHE			 2
#define IBS_DATA_SRC_DRAM			 3
#define IBS_DATA_SRC_REM_CACHE			 4
#define IBS_DATA_SRC_IO				 7


#define IBS_DATA_SRC_EXT_LOC_CACHE		 1
#define IBS_DATA_SRC_EXT_NEAR_CCX_CACHE		 2
#define IBS_DATA_SRC_EXT_DRAM			 3
#define IBS_DATA_SRC_EXT_FAR_CCX_CACHE		 5
#define IBS_DATA_SRC_EXT_PMEM			 6
#define IBS_DATA_SRC_EXT_IO			 7
#define IBS_DATA_SRC_EXT_EXT_MEM		 8
#define IBS_DATA_SRC_EXT_PEER_AGENT_MEM		12




union ibs_fetch_ctl {
	__u64 val;
	struct {
		__u64	fetch_maxcnt:16,
			fetch_cnt:16,	
			fetch_lat:16,	
			fetch_en:1,	
			fetch_val:1,	
			fetch_comp:1,	
			ic_miss:1,	
			phy_addr_valid:1,
			l1tlb_pgsz:2,	
			l1tlb_miss:1,	
			l2tlb_miss:1,	
			rand_en:1,	
			fetch_l2_miss:1,
			l3_miss_only:1,	
			fetch_oc_miss:1,
			fetch_l3_miss:1,
			reserved:2;	
	};
};


union ibs_op_ctl {
	__u64 val;
	struct {
		__u64	opmaxcnt:16,	
			l3_miss_only:1,	
			op_en:1,	
			op_val:1,	
			cnt_ctl:1,	
			opmaxcnt_ext:7,	
			reserved0:5,	
			opcurcnt:27,	
			reserved1:5;	
	};
};


union ibs_op_data {
	__u64 val;
	struct {
		__u64	comp_to_ret_ctr:16,	
			tag_to_ret_ctr:16,	
			reserved1:2,		
			op_return:1,		
			op_brn_taken:1,		
			op_brn_misp:1,		
			op_brn_ret:1,		
			op_rip_invalid:1,	
			op_brn_fuse:1,		
			op_microcode:1,		
			reserved2:23;		
	};
};


union ibs_op_data2 {
	__u64 val;
	struct {
		__u64	data_src_lo:3,	
			reserved0:1,	
			rmt_node:1,	
			cache_hit_st:1,	
			data_src_hi:2,	
			reserved1:56;	
	};
};


union ibs_op_data3 {
	__u64 val;
	struct {
		__u64	ld_op:1,			
			st_op:1,			
			dc_l1tlb_miss:1,		
			dc_l2tlb_miss:1,		
			dc_l1tlb_hit_2m:1,		
			dc_l1tlb_hit_1g:1,		
			dc_l2tlb_hit_2m:1,		
			dc_miss:1,			
			dc_mis_acc:1,			
			reserved:4,			
			dc_wc_mem_acc:1,		
			dc_uc_mem_acc:1,		
			dc_locked_op:1,			
			dc_miss_no_mab_alloc:1,		
			dc_lin_addr_valid:1,		
			dc_phy_addr_valid:1,		
			dc_l2_tlb_hit_1g:1,		
			l2_miss:1,			
			sw_pf:1,			
			op_mem_width:4,			
			op_dc_miss_open_mem_reqs:6,	
			dc_miss_lat:16,			
			tlb_refill_lat:16;		
	};
};


union ic_ibs_extd_ctl {
	__u64 val;
	struct {
		__u64	itlb_refill_lat:16,	
			reserved:48;		
	};
};



struct perf_ibs_data {
	u32		size;
	union {
		u32	data[0];	
		u32	caps;
	};
	u64		regs[MSR_AMD64_IBS_REG_COUNT_MAX];
};
