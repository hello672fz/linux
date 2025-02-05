/* SPDX-License-Identifier: GPL-2.0 */
#if !defined(KVM_X86_PMU_OP) || !defined(KVM_X86_PMU_OP_OPTIONAL)
BUILD_BUG_ON(1)
#endif


KVM_X86_PMU_OP(hw_event_available)
KVM_X86_PMU_OP(pmc_is_enabled)
KVM_X86_PMU_OP(pmc_idx_to_pmc)
KVM_X86_PMU_OP(rdpmc_ecx_to_pmc)
KVM_X86_PMU_OP(msr_idx_to_pmc)
KVM_X86_PMU_OP(is_valid_rdpmc_ecx)
KVM_X86_PMU_OP(is_valid_msr)
KVM_X86_PMU_OP(get_msr)
KVM_X86_PMU_OP(set_msr)
KVM_X86_PMU_OP(refresh)
KVM_X86_PMU_OP(init)
KVM_X86_PMU_OP(reset)
KVM_X86_PMU_OP_OPTIONAL(deliver_pmi)
KVM_X86_PMU_OP_OPTIONAL(cleanup)

#undef KVM_X86_PMU_OP
#undef KVM_X86_PMU_OP_OPTIONAL
