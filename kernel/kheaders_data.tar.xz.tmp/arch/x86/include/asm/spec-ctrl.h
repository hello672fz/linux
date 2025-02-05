/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SPECCTRL_H_
#define _ASM_X86_SPECCTRL_H_

#include <linux/thread_info.h>
#include <asm/nospec-branch.h>


extern void x86_virt_spec_ctrl(u64 guest_virt_spec_ctrl, bool guest);


static inline
void x86_spec_ctrl_set_guest(u64 guest_virt_spec_ctrl)
{
	x86_virt_spec_ctrl(guest_virt_spec_ctrl, true);
}


static inline
void x86_spec_ctrl_restore_host(u64 guest_virt_spec_ctrl)
{
	x86_virt_spec_ctrl(guest_virt_spec_ctrl, false);
}


extern u64 x86_amd_ls_cfg_base;
extern u64 x86_amd_ls_cfg_ssbd_mask;

static inline u64 ssbd_tif_to_spec_ctrl(u64 tifn)
{
	BUILD_BUG_ON(TIF_SSBD < SPEC_CTRL_SSBD_SHIFT);
	return (tifn & _TIF_SSBD) >> (TIF_SSBD - SPEC_CTRL_SSBD_SHIFT);
}

static inline u64 stibp_tif_to_spec_ctrl(u64 tifn)
{
	BUILD_BUG_ON(TIF_SPEC_IB < SPEC_CTRL_STIBP_SHIFT);
	return (tifn & _TIF_SPEC_IB) >> (TIF_SPEC_IB - SPEC_CTRL_STIBP_SHIFT);
}

static inline unsigned long ssbd_spec_ctrl_to_tif(u64 spec_ctrl)
{
	BUILD_BUG_ON(TIF_SSBD < SPEC_CTRL_SSBD_SHIFT);
	return (spec_ctrl & SPEC_CTRL_SSBD) << (TIF_SSBD - SPEC_CTRL_SSBD_SHIFT);
}

static inline unsigned long stibp_spec_ctrl_to_tif(u64 spec_ctrl)
{
	BUILD_BUG_ON(TIF_SPEC_IB < SPEC_CTRL_STIBP_SHIFT);
	return (spec_ctrl & SPEC_CTRL_STIBP) << (TIF_SPEC_IB - SPEC_CTRL_STIBP_SHIFT);
}

static inline u64 ssbd_tif_to_amd_ls_cfg(u64 tifn)
{
	return (tifn & _TIF_SSBD) ? x86_amd_ls_cfg_ssbd_mask : 0ULL;
}

#ifdef CONFIG_SMP
extern void speculative_store_bypass_ht_init(void);
#else
static inline void speculative_store_bypass_ht_init(void) { }
#endif

extern void speculation_ctrl_update(unsigned long tif);
extern void speculation_ctrl_update_current(void);

#endif
