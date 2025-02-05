/* SPDX-License-Identifier: GPL-2.0 */


#ifndef IOSF_MBI_SYMS_H
#define IOSF_MBI_SYMS_H

#include <linux/notifier.h>

#define MBI_MCR_OFFSET		0xD0
#define MBI_MDR_OFFSET		0xD4
#define MBI_MCRX_OFFSET		0xD8

#define MBI_RD_MASK		0xFEFFFFFF
#define MBI_WR_MASK		0X01000000

#define MBI_MASK_HI		0xFFFFFF00
#define MBI_MASK_LO		0x000000FF
#define MBI_ENABLE		0xF0


#define MBI_MMIO_READ		0x00
#define MBI_MMIO_WRITE		0x01
#define MBI_CFG_READ		0x04
#define MBI_CFG_WRITE		0x05
#define MBI_CR_READ		0x06
#define MBI_CR_WRITE		0x07
#define MBI_REG_READ		0x10
#define MBI_REG_WRITE		0x11
#define MBI_ESRAM_READ		0x12
#define MBI_ESRAM_WRITE		0x13


#define BT_MBI_UNIT_AUNIT	0x00
#define BT_MBI_UNIT_SMC		0x01
#define BT_MBI_UNIT_CPU		0x02
#define BT_MBI_UNIT_BUNIT	0x03
#define BT_MBI_UNIT_PMC		0x04
#define BT_MBI_UNIT_GFX		0x06
#define BT_MBI_UNIT_SMI		0x0C
#define BT_MBI_UNIT_CCK		0x14
#define BT_MBI_UNIT_USB		0x43
#define BT_MBI_UNIT_SATA	0xA3
#define BT_MBI_UNIT_PCIE	0xA6


#define QRK_MBI_UNIT_HBA	0x00
#define QRK_MBI_UNIT_HB		0x03
#define QRK_MBI_UNIT_RMU	0x04
#define QRK_MBI_UNIT_MM		0x05
#define QRK_MBI_UNIT_SOC	0x31


#define MBI_PMIC_BUS_ACCESS_BEGIN	1
#define MBI_PMIC_BUS_ACCESS_END		2

#if IS_ENABLED(CONFIG_IOSF_MBI)

bool iosf_mbi_available(void);


int iosf_mbi_read(u8 port, u8 opcode, u32 offset, u32 *mdr);


int iosf_mbi_write(u8 port, u8 opcode, u32 offset, u32 mdr);


int iosf_mbi_modify(u8 port, u8 opcode, u32 offset, u32 mdr, u32 mask);


void iosf_mbi_punit_acquire(void);


void iosf_mbi_punit_release(void);


int iosf_mbi_block_punit_i2c_access(void);


void iosf_mbi_unblock_punit_i2c_access(void);


int iosf_mbi_register_pmic_bus_access_notifier(struct notifier_block *nb);


int iosf_mbi_unregister_pmic_bus_access_notifier(struct notifier_block *nb);


int iosf_mbi_unregister_pmic_bus_access_notifier_unlocked(
	struct notifier_block *nb);


void iosf_mbi_assert_punit_acquired(void);

#else 
static inline
bool iosf_mbi_available(void)
{
	return false;
}

static inline
int iosf_mbi_read(u8 port, u8 opcode, u32 offset, u32 *mdr)
{
	WARN(1, "IOSF_MBI driver not available");
	return -EPERM;
}

static inline
int iosf_mbi_write(u8 port, u8 opcode, u32 offset, u32 mdr)
{
	WARN(1, "IOSF_MBI driver not available");
	return -EPERM;
}

static inline
int iosf_mbi_modify(u8 port, u8 opcode, u32 offset, u32 mdr, u32 mask)
{
	WARN(1, "IOSF_MBI driver not available");
	return -EPERM;
}

static inline void iosf_mbi_punit_acquire(void) {}
static inline void iosf_mbi_punit_release(void) {}

static inline
int iosf_mbi_register_pmic_bus_access_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline
int iosf_mbi_unregister_pmic_bus_access_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int
iosf_mbi_unregister_pmic_bus_access_notifier_unlocked(struct notifier_block *nb)
{
	return 0;
}

static inline
int iosf_mbi_call_pmic_bus_access_notifier_chain(unsigned long val, void *v)
{
	return 0;
}

static inline void iosf_mbi_assert_punit_acquired(void) {}

#endif 

#endif 
