// SPDX-License-Identifier: GPL-2.0-only
/*
 * This module provides the feature to manage memory on the dax device
 */
#define pr_fmt(fmt) "pseudo_mm_memory:%s: " fmt, __func__

#include <linux/file.h>
#include <linux/dax.h>
#include <linux/pfn_t.h>
#include <linux/spinlock.h>
#include <linux/pseudo_mm.h>
#include <linux/mm.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/topology.h>
#include <linux/hashtable.h>

#include "../bus.h"
#include "../dax-private.h"
#include "pseudo_mm_memory.h"


#define POOL_HASH_BITS 10


bool isWorH(unsigned long vaddr){
	return true;
}

u32 get_page_index(struct func_page_pool* fpp, unsigned long vaddr){
	return hash_long(vaddr>>PAGE_SHIFT,fpp->hash_bits);
}


/* 
 * setup read-only page table entry for vma in pseudo_mm
 * @start: start virtual address
 * @nr_pages: number of pages needed to be set
 * @pgoff: page offset of dax device
 */
 //hash pool
static unsigned long __setup_pool_for_func_vma(int id,
					    unsigned long start,
					    unsigned long nr_pages,
					    pgoff_t pgoff)
{
	struct pseudo_mm_backend *backend = pseudo_mm_get_backend();
	struct func_page_pool *fpp = find_page_pool(id);

	struct page *head_page, **pages;
	phys_addr_t phys;
	pfn_t pfn;
	unsigned long ret = 0, i, vaddr;

	// pr_debug("__setup_pool_for_func_vma start!!!!!!!!!!!\n");


	if (!backend->page) {
		pr_err("do not register mem backend for pseudo_mm\n");
		return -ENOENT;
	}

	for (i = 0; i < nr_pages; i++) {
		vaddr = start + (i << PAGE_SHIFT);
		phys = (page_to_pfn(backend->page) + pgoff + i) << PAGE_SHIFT;
		if (phys == -1) {
			pr_warn("pgoff_to_phys(%ld) failed\n", pgoff + i);
			ret = -EFAULT;
			goto err_free;
			// goto out;
		}
		pfn = phys_to_pfn_t(phys, PFN_DEV | PFN_MAP);
		head_page = pfn_t_to_page(pfn);

		//func vaddr,page
		if(isWorH(vaddr)){
			// create_srcpages_hash_list(fpp, page, vaddr, numa_node,copy_nr_pages);
			struct special_page_entry *spe;
			unsigned int copy_nr_pages=2;
			unsigned int max_prealloc=3;
			unsigned int flags;
			int numa_node=0;
			u32 index=get_page_index(fpp,vaddr);

			//initialize special_page_entry
			spe = kmalloc(sizeof(*spe), GFP_KERNEL);
			if (!spe)
				return ERR_PTR(-ENOMEM);

			spe->vaddr = vaddr;
			spe->master_page = head_page;
			spe->max_prealloc = max_prealloc;

			//add to func_page_pool
			INIT_LIST_HEAD(&spe->free_copies);
			INIT_LIST_HEAD(&spe->used_copies);
			spin_lock_init(&spe->lock);
		
			/* 为后续页分配新的物理页并拷贝头物理页内容 */
			for (int i = 1; i < copy_nr_pages; i++) {
				struct copy_page *cpage = kmalloc(sizeof(*cpage), GFP_KERNEL);
				if (!cpage){
					ret=ERR_PTR(-ENOMEM);
					goto err_free;
				}
				struct page *new_page = alloc_pages_node(numa_node, GFP_KERNEL, 0);
				 if (!new_page){
					ret=ERR_PTR(-ENOMEM);
					// free_srcpages_hash_list(spe);
					goto err_free;
				 }
				 copy_highpage(new_page, head_page);
				 cpage->page=new_page;
				 cpage->state = COPY_PAGE_FREE;
				//  pr_info("PagePool:New page paddr:%llx,New page vaddr:%llx",cpage->page,spe->vaddr);
				//  pr_info("PagePool:Old page paddr:%llx,Old page vaddr:%llx",spe->master_page,vaddr);
				 list_add_tail(&cpage->list, &spe->free_copies);
				 spe->prealloc_count++;
			}

			spin_lock_irqsave(&fpp->bucket_locks[index],flags);
			// vaddr as hash index
			// unsigned int index = page_pool_hash(fpp,vaddr);
			// INIT_HLIST_HEAD(&fpp->srcpages_hash_head[index]);
        	hlist_add_head(&spe->node, &fpp->buckets[index]);
			spin_unlock_irqrestore(&fpp->bucket_locks[index],flags);
			return 0;
			// return spe;
		}
	pr_info("__setup_pool_for_func_vma end!!!!!!!!!!!\n");
	}

out:
	return ret;
err_free:
	pr_info("__setup_pool_for_func_vma failed\n");
	goto out;
}


pte_t *get_pte_from_vaddr(struct mm_struct *mm, unsigned long vaddr){
	
	pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

	pgd = pgd_offset(mm, vaddr);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return NULL;

	p4d = p4d_offset(pgd, vaddr);
	if (p4d_none(*p4d) || p4d_bad(*p4d))
		return NULL;

	pud = pud_offset(p4d, vaddr);
	if (pud_none(*pud) || pud_bad(*pud))
		return NULL;

	pmd = pmd_offset(pud, vaddr);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return NULL;

	pte = pte_offset_map(pmd, vaddr);
	if (!pte || pte_none(*pte)) {
		pte_unmap(pte);
		return NULL;
	}

	return pte;
}


unsigned long pseudo_mm_add_page(int id,
					    unsigned long vaddr,
					    unsigned long copy_nr_pages, int numa_node) {
	struct func_page_pool *fpp;
	struct pseudo_mm *pseudo_mm;
	struct mm_struct *mm;
	spinlock_t *ptl;
	pte_t *pte;
	struct page *head_page;
	phys_addr_t phys;
	pfn_t pfn;
	unsigned long ret = 0;
	unsigned int flags;
	u32 index;

	fpp = find_page_pool(id);
	if(!fpp)
		return -1;

	pseudo_mm = find_pseudo_mm(id);
	if (!pseudo_mm)
		return -ENOENT;
	
	mm = pseudo_mm->mm;
	mmap_read_lock_killable(mm); 
	pte = get_pte_from_vaddr(mm, vaddr);
	if (!pte) {
		return -ENOMEM;
	}
	head_page = pte_page(*pte);
	phys = page_to_pfn(head_page) << PAGE_SHIFT;

	//initialize special_page_entry
	struct special_page_entry *spe;
	spe = kmalloc(sizeof(*spe), GFP_KERNEL);
	if (!spe)
		return ERR_PTR(-ENOMEM);
	spe->vaddr = vaddr;
	spe->master_page = head_page;
	spe->max_prealloc = 100;	
	INIT_LIST_HEAD(&spe->free_copies);
	INIT_LIST_HEAD(&spe->used_copies);
	spin_lock_init(&spe->lock);

	for (int i = 0; i < copy_nr_pages; i++) {
		struct copy_page *cpage = kmalloc(sizeof(*cpage), GFP_KERNEL);
		if (!cpage){
			ret = ERR_PTR(-ENOMEM);
			goto err_free;
		}
		struct page *new_page = alloc_pages_node(numa_node, GFP_KERNEL, 0);
		if (!new_page){
			ret = ERR_PTR(-ENOMEM);
			// free_srcpages_hash_list(spe);
			goto err_free;
		 }
		 copy_highpage(new_page, head_page);
		 cpage->page = new_page;
		 cpage->state = COPY_PAGE_FREE;
		 list_add_tail(&cpage->list, &spe->free_copies);
		 spe->prealloc_count++;
		 pr_info("[add page] vaddr: %lx, pfn: %lx, numa_node %d", vaddr, page_to_pfn(new_page), numa_node);
	}

	index = get_page_index(fpp,vaddr);
	spin_lock_irqsave(&fpp->bucket_locks[index],flags);
    hlist_add_head(&spe->node, &fpp->buckets[index]);
	spin_unlock_irqrestore(&fpp->bucket_locks[index],flags);
out:
	mmap_read_unlock(mm);
	return ret;
err_free:
	pr_info("pseudo_mm_add_page failed\n");
	goto out;
}


static unsigned long __setup_pt_for_vma_dax(int id,
						struct pseudo_mm *pseudo_mm,
					    struct vm_area_struct *vma,
					    unsigned long start,
					    unsigned long nr_pages,
					    pgoff_t pgoff)
{
	struct pseudo_mm_backend *backend = pseudo_mm_get_backend();
	struct pseudo_mm_pin_pages *pin_page = NULL;
	struct dev_pagemap *pgmap = NULL;
	struct page *page, **pages;
	phys_addr_t phys;
	pfn_t pfn;
	unsigned long ret = 0, i, vaddr;
	long nr_pin_pages = 0;
	vm_fault_t vmf_ret;
	// unsigned long ret;
	// unsigned int copy_nr_pages;
	// int numa_node;

	if (!backend->page) {
		pr_err("do not register mem backend for pseudo_mm\n");
		return -ENOENT;
	}

	// dev_dax = backend->filp->private_data;

	pages = kvmalloc_array(nr_pages, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	// id = dax_read_lock();
	// if (dev_dax->align != PAGE_SIZE) {
	// 	pr_warn("dax alignment (%#x) != PAGE_SIZE\n", dev_dax->align);
	// 	ret = -EIO;
	// 	goto failed;
	// }

	// Map pages to dax device one by one
	// since insert_mixed api is insert one pfn at a time.
	// However, its performance not a big deal, since __setup_pt_for_vma is
	// called on prepare phase, it will not effect the attach performance.
	for (i = 0; i < nr_pages; i++) {
		vaddr = start + (i << PAGE_SHIFT);
		// phys = dax_pgoff_to_phys(dev_dax, pgoff + i, PAGE_SIZE);
		phys = (page_to_pfn(backend->page) + pgoff + i) << PAGE_SHIFT;
		if (phys == -1) {
			pr_warn("pgoff_to_phys(%ld) failed\n", pgoff + i);
			ret = -EFAULT;
			goto failed;
		}
		
		// if(pfn_in_file(phys)){
		// if(i==-1){
		// 	ret=add_physical_page(id, phys, num_pages, numa_node);
		// 	if (ret) {
		// 		return -ENOMEM;
		// 	}
		// 	traverse_copy_page_pools();
		// 	free_copy_page_pools();
		// }
		
		pfn = phys_to_pfn_t(phys, PFN_DEV | PFN_MAP);
		vmf_ret = pseudo_mm_insert_dax(vma, vaddr, pfn);
		if (unlikely(vmf_ret & VM_FAULT_ERROR)) {
			pr_warn("pseudo_mm_insert_dax vaddr %#lx pfn %#llx phys %#llx failed\n",
				vaddr, pfn.val, phys);
			ret = -EFAULT;
			goto failed;
		}
		// BEGIN imitate pin_user_pages()
		// pgmap = get_dev_pagemap(pfn_t_to_pfn(pfn), pgmap);
		// WARN_ON(!pgmap);
		page = pfn_t_to_page(pfn);
		ret = arch_make_page_accessible(page);
		if (ret) {
			unpin_user_page(page);
			goto failed;
		}
		pages[nr_pin_pages++] = page;
		flush_anon_page(vma, page, start);
		flush_dcache_page(page);
		// END imitate pin_user_pages()
	}

	pin_page = kmalloc(sizeof(*pin_page), GFP_KERNEL);
	if (!pin_page) {
		ret = -ENOMEM;
		goto failed;
	}

	BUG_ON(nr_pin_pages != nr_pages);
	INIT_LIST_HEAD(&pin_page->list);
	pin_page->pages = pages;
	pin_page->nr_pin_pages = nr_pin_pages;
	list_add(&pin_page->list, &pseudo_mm->pages_list);

#ifdef PSEUDO_MM_DEBUG
	pr_info("setup page table %#lx - %#lx (V) to DAX pgoff %#lx - %#lx\n",
		start, start + (nr_pages << PAGE_SHIFT), pgoff,
		pgoff + nr_pages);
#endif

out:
	// if (pgmap) 
	// 	put_dev_pagemap(pgmap);
	// dax_read_unlock(id);
	return ret;

failed:
	if (nr_pin_pages > 0)
		unpin_user_pages(pages, nr_pin_pages);
	kvfree(pages);
	if (pin_page)
		kfree(pin_page);
	goto out;
}


/* 
 * setup rdma page table entry for vma in pseudo_mm
 * @start: start virtual address
 * @nr_pages: number of pages needed to be set
 * @pgoff: page offset of dax device
 */
static unsigned long __setup_pt_for_vma_rdma(struct pseudo_mm *pseudo_mm,
					     struct vm_area_struct *vma,
					     unsigned long start,
					     unsigned long nr_pages,
					     pgoff_t pgoff)
{
	unsigned long ret = 0, i, vaddr;
	vm_fault_t vmf_ret;

	if (!pseudo_mm_rdma_pf_handler_enable()) {
		pr_err("pseudo_mm_rdma_pf_handler not enable\n");
		return -ENOENT;
	}

	// Map pages to dax device one by one
	// since insert_mixed api is insert one pfn at a time.
	// However, its performance not a big deal, since __setup_pt_for_vma is
	// called on prepare phase, it will not effect the attach performance.
	for (i = 0; i < nr_pages; i++) {
		vaddr = start + (i << PAGE_SHIFT);
		vmf_ret = pseudo_mm_insert_rdma(vma, vaddr, pgoff + i);
		if (unlikely(vmf_ret & VM_FAULT_ERROR)) {
			pr_warn("pseudo_mm_insert_rdma vaddr %#lx pgoff %#lx failed\n",
				vaddr, pgoff + i);
			ret = -EFAULT;
			goto out;
		}
	}

#ifdef PSEUDO_MM_DEBUG
	pr_info("setup page table %#lx - %#lx (V) to RDMA pgoff %#lx - %#lx\n",
		start, start + (nr_pages << PAGE_SHIFT), pgoff,
		pgoff + nr_pages);
#endif

out:
	return ret;
}

unsigned long pseudo_mm_setup_pt(int id, unsigned long start,
				 unsigned long size, pgoff_t pgoff,
				 enum pseudo_mm_pt_type type)
{
	struct pseudo_mm *pseudo_mm = find_pseudo_mm(id);
	// struct func_page_pool *fpp = find_page_pool(id);
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned long end = start + size;
	unsigned long ret;

	if (!pseudo_mm)
		return -ENOENT;
	mm = pseudo_mm->mm;
	mmap_read_lock_killable(mm); // find_vma_intersection() need mmap lock
	vma = find_vma_intersection(mm, start, end);
	if (!range_in_vma(vma, start, end)) {
		pr_warn("(%#lx - %#lx) is not within single vma\n", start, end);
		ret = -EFAULT;
		goto out;
	}
	if (!vma_is_pseudo_mm_master(vma)) {
		pr_warn("vma (%#lx - %#lx) is not pseudo mm master\n",
			vma->vm_start, vma->vm_end);
		ret = -EINVAL;
		goto out;
	}

	// Couple of things that happened in normal anon private page fault handler:
	// 1. prepare_anon_vma
	// 2. add page to anon_vma and lru list
	// we need to do 1 (so that copy_page_range will copy page table) but do not need to do 2:
	// since these pages are used exclusively by pseudo_mm module
	if (unlikely(anon_vma_prepare(vma))) {
		ret = -ENOMEM;
		goto out;
	}

	switch (type) {
	case DAX_MEM:{
		ret = __setup_pt_for_vma_dax(id,pseudo_mm, vma, start,
					     size >> PAGE_SHIFT, pgoff);
		// ret1 = __setup_pool_for_func_vma(id,start,size >> PAGE_SHIFT,pgoff);
		break;	
	}
	case RDMA_MEM:
		ret = __setup_pt_for_vma_rdma(pseudo_mm, vma, start,
					      size >> PAGE_SHIFT, pgoff);
		break;
	default:
		ret = -EINVAL;
	}
out:
	mmap_read_unlock(mm);
	return ret;
}


unsigned long pseudo_mm_bring_back(int id, unsigned long start,
				   unsigned long size)
{
	struct pseudo_mm *pseudo_mm;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned long vaddr, end = start + size;
	unsigned long ret;

	// start and size must be page aligned
	if (!PAGE_ALIGNED(start) || !PAGE_ALIGNED(size) || size == 0)
		return -EINVAL;
	pseudo_mm = find_pseudo_mm(id);
	if (!pseudo_mm)
		return -ENOENT;

	mm = pseudo_mm->mm;
	mmap_read_lock_killable(mm); // find_vma_intersection() need mmap lock
	vma = find_vma_intersection(mm, start, end);
	if (!range_in_vma(vma, start, end)) {
		pr_warn("(%#lx - %#lx) is not within single vma\n", start, end);
		ret = -EFAULT;
		goto out;
	}
	if (!vma_is_pseudo_mm_master(vma)) {
		pr_warn("vma (%#lx - %#lx) is not pseudo mm master\n",
			vma->vm_start, vma->vm_end);
		ret = -EINVAL;
		goto out;
	}
	// TODO(huang-jl): I only support to bring back private anonymous vma for now.
	// For shared anonymous area: it is really hard to implement a mm template (more info
	// can be found at pseudo_mm branch and git commit message).
	// For file-backed area: it is already backed by page-cache (or local memory) by default.
	//
	// In fact this is not a todo, I just do not want to implement for shared anonymous vma :(
	if (!vma_is_anonymous(vma) || (vma->vm_flags & VM_SHARED)) {
		pr_warn("try to bring back memory within vma (%#lx - %#lx), which is not anonymous private vma\n",
			vma->vm_start, vma->vm_end);
		ret = -EINVAL;
		goto out;
	}

	// bring back pages one by one
	for (vaddr = start; vaddr < end; vaddr += PAGE_SIZE) {
		ret = pseudo_mm_bring_back_single_page(mm, vma, vaddr);
		if (ret) {
			goto out;
		}
	}
out:
	mmap_read_unlock(mm);
	return ret;
}


unsigned long pseudo_mm_update_page(pid_t pid, int id, unsigned long start,
	unsigned long size){
			struct pseudo_mm *pseudo_mm;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned long vaddr, end = start + size;
	unsigned long ret;
	struct task_struct *tsk;

	// start and size must be page aligned
	if (!PAGE_ALIGNED(start) || !PAGE_ALIGNED(size) || size == 0)
		return -EINVAL;
	pseudo_mm = find_pseudo_mm(id);
	if (!pseudo_mm)
		return -ENOENT;
	
	rcu_read_lock();
	tsk = find_task_by_vpid(pid);
	if(!tsk){
		pr_warn("cannot find task of pif %d\n", pid);
		return -ESRCH;
	}
	rcu_read_unlock();

	mm = get_task_mm(tsk);
	if(!mm){
		pr_warn("cannot get tsk mm of pid %d\n", pid);
		return -1;
	}

	mmap_read_lock_killable(mm); // find_vma_intersection() need mmap lock
	vma = find_vma_intersection(mm, start, end);
	if (!range_in_vma(vma, start, end)) {
		pr_warn("(%#lx - %#lx) is not within single vma\n", start, end);
		ret = -EFAULT;
		goto out;
	}
	// if (!vma_is_pseudo_mm_master(vma)) {
	// 	pr_warn("vma (%#lx - %#lx) is not pseudo mm master\n",
	// 		vma->vm_start, vma->vm_end);
	// 	ret = -EINVAL;
	// 	goto out;
	// }
	// TODO(huang-jl): I only support to bring back private anonymous vma for now.
	// For shared anonymous area: it is really hard to implement a mm template (more info
	// can be found at pseudo_mm branch and git commit message).
	// For file-backed area: it is already backed by page-cache (or local memory) by default.
	//
	// In fact this is not a todo, I just do not want to implement for shared anonymous vma :(
	if (!vma_is_anonymous(vma) || (vma->vm_flags & VM_SHARED)) {
		pr_warn("try to bring back memory within vma (%#lx - %#lx), which is not anonymous private vma\n",
			vma->vm_start, vma->vm_end);
		ret = -EINVAL;
		goto out;
	}

	// bring back pages one by one
	for (vaddr = start; vaddr < end; vaddr += PAGE_SIZE) {
		struct special_page_entry *entry = find_special_page(id, vaddr);
		struct copy_page *copy_page = snapshot_alloc_copy(entry);
		if (!copy_page){
			pr_warn("No free page of vaddr:%lx, for pid:%d, with pseudo_mm_id:%d\n", vaddr, pid, id);
			goto out;
		}
		ret = pseudo_mm_update_single_page(mm, vma, vaddr, copy_page->page);
		if (ret) {
			goto out;
		}
	}
out:
	mmap_read_unlock(mm);
	return ret;
}


