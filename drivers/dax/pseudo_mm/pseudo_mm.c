// SPDX-License-Identifier: GPL-2.0-only
/*
 * This module provides an interface to operate pseudo_mm
 */

#define pr_fmt(fmt) "pseudo_mm_driver:%s: " fmt, __func__

#include <linux/sched/mm.h>
#include <linux/mm.h>		/* for GFP_ATOMIC */
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pseudo_mm.h>
#include <asm/io.h>

#include "pseudo_mm_memory.h"
#include "pseudo_mm_ioctl.h"

static int pseudo_mm_open(struct inode *, struct file *);
static int pseudo_mm_release(struct inode *, struct file *);
static long pseudo_mm_unlocked_ioctl(struct file *, unsigned int,
				     unsigned long);

static const struct file_operations fops = {
	.open = pseudo_mm_open,
	.release = pseudo_mm_release,
	.unlocked_ioctl = pseudo_mm_unlocked_ioctl,
};

struct miscdevice pseudo_mm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "pseudo_mm",
	.fops = &fops,
};

static int pseudo_mm_open(struct inode *inode, struct file *filp)
{
	// actually we do nothing here when open the file for now
	return 0;
}

static int pseudo_mm_release(struct inode *inode, struct file *filp)
{
	// actually we do nothing here when close the file for now
	return 0;
}

// Return 0 when succeed
static inline long _pseudo_mm_add_map(void *__user args)
{
	struct pseudo_mm_add_map_param param;
	unsigned long err = 0;
	unsigned long start, size;

	err = copy_from_user(&param, args, sizeof(param));
	if (err)
		return err;
	start = param.start;
	if (param.end < param.start)
		return -EINVAL;
	size = param.end - param.start;

	err = pseudo_mm_add_map(param.id, start, size, param.prot, param.flags,
				param.fd, param.offset >> PAGE_SHIFT);
	return err;
}

static inline long _pseudo_mm_attach(void *__user args)
{
	struct pseudo_mm_attach_param param;
	unsigned long err;

	err = copy_from_user(&param, args, sizeof(param));
	if (err)
		return err;
	err = pseudo_mm_attach(param.pid, param.id);
	return err;
}

static inline long _pseudo_mm_setup_pt(void *__user args)
{
	struct pseudo_mm_setup_pt_param param;
	unsigned long err;
	err = copy_from_user(&param, args, sizeof(param));
	if (err)
		return err;
//  err = pseudo_mm_setup_pt(param.id, param.start, param.size, param.pgoff,
// 			 param.type);
	err = pseudo_mm_setup_pt(param.id, param.start, param.size, param.pgoff,
				 DAX_MEM);
	return err;
} 

// void *__user args
static inline long _pseudo_mm_getpte(pid_t pid)
{
	struct pseudo_mm_getpte_param param;
	param.pid=pid;
	unsigned long err;
	// err = copy_from_user(&param, args, sizeof(param));
	// if (err)
	// 	return err;
	
//  err = pseudo_mm_setup_pt(param.id, param.start, param.size, param.pgoff,
// 			 param.type);
	err = pseudo_mm_getpte(param.pid);
	return err;
}


static inline long _pseudo_mm_bring_back(void *__user args)
{
	struct pseudo_mm_bring_back_param param;
	unsigned long err;
	err = copy_from_user(&param, args, sizeof(param));
	if (err)
		return err;
	err = pseudo_mm_bring_back(param.id, param.start, param.size);
	return err;
}

static inline long _pseudo_mm_pf_stat(void *__user args)
{
	struct pseudo_mm_pf_stat_param param;
	unsigned long err;
	struct mm_struct *mm;
	struct task_struct *tsk;
	err = copy_from_user(&param, args, sizeof(param));
	if (err)
		return err;
	tsk = find_task_by_vpid(param.pid);
	if (!tsk)
		return -ESRCH;
	mm = get_task_mm(tsk);
	if (!mm)
		return -ESRCH;
	get_task_struct(tsk);
	param.cow_nr = atomic_read(&mm->pseudo_mm_cow_nr);
	param.fast_rdma_read_nr = atomic_read(&mm->pseudo_mm_fast_rdma_read_nr);
	param.slow_rdma_read_nr = atomic_read(&mm->pseudo_mm_slow_rdma_read_nr);
	param.rdma_read_nr = atomic_read(&mm->pseudo_mm_rdma_read_nr);
	err = copy_to_user(args, &param, sizeof(param));
	mmput(mm);
	put_task_struct(tsk);
	return err;
}


static inline long _register_backend_memory(void *__user args)
{
	struct pseudo_mm_register_param param;
	unsigned long err;
	err = copy_from_user(&param, args, sizeof(param));
	if (err)
		return err;
	
	return register_backend_memory(param.node, param.order);
}

static long pseudo_mm_unlocked_ioctl(struct file *filp, unsigned int cmd,
				     unsigned long args)
{
	int pseudo_mm_id, page_pool_id, fd;
	long err = 0;
	long phy_addr = 0;
	pid_t pid;

	// all ioctl in pseudo_mm need args
	if (!args)
		return -EINVAL;

	switch (cmd) {
	case PSEUDO_MM_IOC_REGISTER:
		err = _register_backend_memory((void *)args);
		if (err)
			return err;
		break;
	case PSEUDO_MM_IOC_CREATE:
		// create a new pseudo_mm and return the id of it
		pseudo_mm_id = create_pseudo_mm();
		page_pool_id = create_func_page_pool();
		if (pseudo_mm_id < 0||page_pool_id!=pseudo_mm_id)
			return pseudo_mm_id;
		err = copy_to_user((void *)args, &pseudo_mm_id,
				   sizeof(pseudo_mm_id));
		if (err)
			return err;
		break;
	case PSEUDO_MM_IOC_DELETE:
		err = copy_from_user(&pseudo_mm_id, (const void *)args,
				     sizeof(pseudo_mm_id));
		if (err)
			return err;
		put_pseudo_mm_with_id(pseudo_mm_id);
		put_page_pool_with_id(pseudo_mm_id);
		break;
	case PSEUDO_MM_IOC_ADD_MAP:
		err = _pseudo_mm_add_map((void *)args);
		if (err)
			return err;
		break;
	case PSEUDO_MM_IOC_SETUP_PT:
		err = _pseudo_mm_setup_pt((void *)args);
		if (err)
			return err;
		break;
	case PSEUDO_MM_IOC_ATTACH:
		err = _pseudo_mm_attach((void *)args);
		if (err)
			return err;
		break;
	case PSEUDO_MM_IOC_GETPTE:
		err = copy_from_user(&pid, (const void *)args,
				     sizeof(pid));
		pr_info("Attention:%d",pid);
		if (err)
			return err;
		err=_pseudo_mm_getpte(pid);
		// break;
		// err = _pseudo_mm_getpte((void *)args);
		// if (err)
			return err;
		break;
	case PSEUDO_MM_IOC_BRING_BACK:
		err = _pseudo_mm_bring_back((void *)args);
		if (err)
			return err;
		break;
	case PSEUDO_MM_IOC_PF_STAT:
		err = _pseudo_mm_pf_stat((void *)args);
		if (err)
			return err;
		break;
	case PSEUDO_MM_IOC_PHY_ADDR:
		phy_addr = pseudo_mm_phy_addr();
		err = copy_to_user((void *)args, &phy_addr,
				   sizeof(phy_addr));
		if (err)
			return err;
		break;
	default:
		pr_warn("pseudo_mm_misc driver receive known cmd %u\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static int __init pseudo_mm_driver_init(void)
{
	int err;

	err = misc_register(&pseudo_mm_device);
	if (err) {
		pr_err("pseudo_mm_misc driver register failed\n");
		return err;
	}

	pr_info("pseudo_mm_misc driver register succeed !\n");
	return 0;
}

static void __exit pseudo_mm_driver_exit(void)
{
	misc_deregister(&pseudo_mm_device);
	pr_info("pseudo_mm_misc driver exit !\n");
}

module_init(pseudo_mm_driver_init);
module_exit(pseudo_mm_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("huang-jl <huangjl22@mails.tsinghua.edu.cn>");
MODULE_DESCRIPTION("A driver used for exposing interface of pseudo_mm");
MODULE_VERSION("0.1");
