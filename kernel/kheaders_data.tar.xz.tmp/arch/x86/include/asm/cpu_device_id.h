/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_CPU_DEVICE_ID
#define _ASM_X86_CPU_DEVICE_ID


#include <linux/mod_devicetable.h>

#include <asm/intel-family.h>

#include <asm/processor.h>


#define X86_CENTAUR_FAM6_C7_A		0xa
#define X86_CENTAUR_FAM6_C7_D		0xd
#define X86_CENTAUR_FAM6_NANO		0xf

#define X86_STEPPINGS(mins, maxs)    GENMASK(maxs, mins)

#define X86_MATCH_VENDOR_FAM_MODEL_STEPPINGS_FEATURE(_vendor, _family, _model, \
						    _steppings, _feature, _data) { \
	.vendor		= X86_VENDOR_##_vendor,				\
	.family		= _family,					\
	.model		= _model,					\
	.steppings	= _steppings,					\
	.feature	= _feature,					\
	.driver_data	= (unsigned long) _data				\
}


#define X86_MATCH_VENDOR_FAM_MODEL_FEATURE(vendor, family, model, feature, data) \
	X86_MATCH_VENDOR_FAM_MODEL_STEPPINGS_FEATURE(vendor, family, model, \
						X86_STEPPING_ANY, feature, data)


#define X86_MATCH_VENDOR_FAM_FEATURE(vendor, family, feature, data)	\
	X86_MATCH_VENDOR_FAM_MODEL_FEATURE(vendor, family,		\
					   X86_MODEL_ANY, feature, data)


#define X86_MATCH_VENDOR_FEATURE(vendor, feature, data)			\
	X86_MATCH_VENDOR_FAM_FEATURE(vendor, X86_FAMILY_ANY, feature, data)


#define X86_MATCH_FEATURE(feature, data)				\
	X86_MATCH_VENDOR_FEATURE(ANY, feature, data)


#define X86_MATCH_VENDOR_FAM_MODEL(vendor, family, model, data)		\
	X86_MATCH_VENDOR_FAM_MODEL_FEATURE(vendor, family, model,	\
					   X86_FEATURE_ANY, data)


#define X86_MATCH_VENDOR_FAM(vendor, family, data)			\
	X86_MATCH_VENDOR_FAM_MODEL(vendor, family, X86_MODEL_ANY, data)


#define X86_MATCH_INTEL_FAM6_MODEL(model, data)				\
	X86_MATCH_VENDOR_FAM_MODEL(INTEL, 6, INTEL_FAM6_##model, data)

#define X86_MATCH_INTEL_FAM6_MODEL_STEPPINGS(model, steppings, data)	\
	X86_MATCH_VENDOR_FAM_MODEL_STEPPINGS_FEATURE(INTEL, 6, INTEL_FAM6_##model, \
						     steppings, X86_FEATURE_ANY, data)



struct x86_cpu_desc {
	u8	x86_family;
	u8	x86_vendor;
	u8	x86_model;
	u8	x86_stepping;
	u32	x86_microcode_rev;
};

#define INTEL_CPU_DESC(model, stepping, revision) {		\
	.x86_family		= 6,				\
	.x86_vendor		= X86_VENDOR_INTEL,		\
	.x86_model		= (model),			\
	.x86_stepping		= (stepping),			\
	.x86_microcode_rev	= (revision),			\
}

extern const struct x86_cpu_id *x86_match_cpu(const struct x86_cpu_id *match);
extern bool x86_cpu_has_min_microcode_rev(const struct x86_cpu_desc *table);

#endif 
