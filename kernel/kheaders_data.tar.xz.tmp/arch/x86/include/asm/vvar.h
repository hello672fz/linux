/* SPDX-License-Identifier: GPL-2.0-only */


#ifndef _ASM_X86_VVAR_H
#define _ASM_X86_VVAR_H

#ifdef EMIT_VVAR

#define DECLARE_VVAR(offset, type, name) \
	EMIT_VVAR(name, offset)

#else

extern char __vvar_page;

#define DECLARE_VVAR(offset, type, name)				\
	extern type vvar_ ## name[CS_BASES]				\
	__attribute__((visibility("hidden")));				\
	extern type timens_ ## name[CS_BASES]				\
	__attribute__((visibility("hidden")));				\

#define VVAR(name) (vvar_ ## name)
#define TIMENS(name) (timens_ ## name)

#define DEFINE_VVAR(type, name)						\
	type name[CS_BASES]						\
	__attribute__((section(".vvar_" #name), aligned(16))) __visible

#endif



DECLARE_VVAR(128, struct vdso_data, _vdso_data)

#undef DECLARE_VVAR

#endif
