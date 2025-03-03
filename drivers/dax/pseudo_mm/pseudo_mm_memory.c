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




// number of function
#define HASH_BITS 10
// number of pagelist
#define FUNC_PAGES_BITS 10
// 
static DEFINE_HASHTABLE(func_hash, HASH_BITS);


// Page list based on source page
struct source_page_list {
    struct page *page;              // Memory page
    struct list_head list_head;          // List head & source page
};

// // Pseudo_mm_pagepool for a specific physical page
// struct pseudo_mm_pagepool {
//     int funcid;               // Unique ID for the funtion mm_pagepool
//     struct hlist_node hlist;        // Hash list node for pseudo_mm_pagepool
//     DECLARE_HASHTABLE(srcpages_hash_list, 1); // Hash table for pagelist based on source pages
// 	// struct hlist_head srcpages_hash_list[1024]; item:srcpages_hash_list[i],hashlist_head
// };


// Hash function to calculate the hash value
static inline u32 hash_function(unsigned long id)
{
    return jhash_1word(id, 0);
}

// Hash function to calculate the hash value for page hash table
static inline u32 page_hash_function(struct page *src_page)
{
    return jhash_1word((unsigned long)src_page, 0);
}

// // Prefetch data from the source page
// static void prefetch_data(void *src_vaddr)
// {
//     // Prefetch the first cache line
//     asm volatile ("prefetcht0 (%0)" :: "r" (src_vaddr) : "memory");
// }

// Allocate and copy pages to a specific NUMA node
static int allocate_and_copy_pages_numa(struct page *src_page, unsigned int num_pages, int numa_node, struct list_head *one_func_pool)
{
    void *src_vaddr, *dst_vaddr;
    unsigned int i;

    // Map the source page to virtual address space
    src_vaddr = vmap(&src_page, 1, VM_MAP, PAGE_KERNEL);
    if (!src_vaddr) {
        pr_err("Failed to map source page\n");
        return -ENOMEM;
    }

    // // Prefetch data from the source page
    // prefetch_data(src_vaddr);

    for (i = 0; i < num_pages; i++) {
        struct source_page_list *pl;
        struct page *dst_page;

        // Allocate a new page on the specified NUMA node
        dst_page = alloc_pages_node(numa_node, GFP_KERNEL, 0);
        if (!dst_page) {
            pr_err("Failed to allocate memory page on NUMA node %d\n", numa_node);
            vunmap(src_vaddr);
            return -ENOMEM;
        }

        // Map the destination page to virtual address space
        dst_vaddr = vmap(&dst_page, 1, VM_MAP, PAGE_KERNEL);
        if (!dst_vaddr) {
            pr_err("Failed to map destination page\n");
            __free_page(dst_page);
            vunmap(src_vaddr);
            return -ENOMEM;
        }

        // Copy data from the source page to the destination page
        memcpy(dst_vaddr, src_vaddr, PAGE_SIZE);

        // Unmap the destination page
        vunmap(dst_vaddr);

        // Add the destination page to the copy page list
        pl = kmalloc(sizeof(*pl), GFP_KERNEL);
        if (!pl) {
            pr_err("Failed to allocate source_page_list structure\n");
            __free_page(dst_page);
            vunmap(src_vaddr);
            return -ENOMEM;
        }

        pl->page = dst_page;
        list_add(&pl->list_head, one_func_pool);
    }

    // Unmap the source page
    vunmap(src_vaddr);

    return 0;
}


// Add a new physical page and fill to its copy page pool
static unsigned long add_physical_page(int id, phys_addr_t src_paddr, unsigned int num_pages, int numa_node)
{
    struct pseudo_mm_pagepool *fpp;
    struct page *src_page;

    // Allocate memory for the copy page pool structure
    fpp = kmalloc(sizeof(*fpp), GFP_KERNEL);
    if (!fpp) {
        pr_err("Failed to allocate pseudo_mm_pagepool structure\n");
        return -ENOMEM;
    }

    fpp->funcid = id;
    hash_init(fpp->srcpages_hash_list);

    // Get the source page from the physical address
    src_page = pfn_to_page(src_paddr >> PAGE_SHIFT);
    if (!src_page) {
        pr_err("Invalid source page address\n");
        kfree(fpp);
        return -EINVAL;
    }

    // Allocate and copy pages
    if (allocate_and_copy_pages_numa(src_page, num_pages, numa_node, &fpp->srcpages_hash_list)) {
        kfree(fpp);
        return -ENOMEM;
    }

    // Calculate the hash value and add the copy page pool to the hash table
    hash_add(func_hash, &fpp->hlist, hash_function(id));

    pr_info("Successfully added physical page %lu and copied %u pages to NUMA node %d\n", id, num_pages, numa_node);

    return 0;
}


// Free all copy page pools
static void free_copy_page_pools(void)
{
    int bkt;
    struct pseudo_mm_pagepool *fpp;
    struct hlist_node *tmp;

    hash_for_each_safe(func_hash, bkt, tmp, fpp, hlist) {
        int page_bkt;
        struct source_page_list *pl;
        struct hlist_node *tmp_fp;

        hash_for_each_safe(fpp->srcpages_hash_list, page_bkt, tmp_fp, pl, list_head) {
            list_del(&pl->list_head);
            __free_page(pl->page);
            kfree(pl);
        }

        hash_del(&fpp->hlist);
        kfree(fpp);
    }
}


// Remove a physical page and its copy page pool
static void remove_physical_page(unsigned long id)
{
    struct pseudo_mm_pagepool *fpp;
    u32 hash = hash_function(id);

    hash_for_each_possible(func_hash, fpp, hlist, hash) {
        if (fpp->funcid == id) {
            int page_bkt;
            struct source_page_list *pl;
            struct hlist_node *tmp_fp;

            hash_for_each_safe(fpp->srcpages_hash_list, page_bkt, tmp_fp, pl, list_head) {
                list_del(&pl->list_head);
                __free_page(pl->page);
                kfree(pl);
            }

            hash_del(&fpp->hlist);
            kfree(fpp);
            pr_info("Successfully removed physical page %lu\n", id);
            return;
        }
    }
    pr_err("Physical page %lu not found\n", id);
}

// Traverse all copy page pools
static void traverse_copy_page_pools(void)
{
    int bkt;
    struct pseudo_mm_pagepool *fpp;

    hash_for_each(func_hash, bkt, fpp, hlist) {
        int page_bkt;
        struct source_page_list *pl;

        pr_info("Physical page %lu:\n", fpp->id);
        hash_for_each(fpp->srcpages_hash_list, page_bkt, pl, list_head) {
            pr_info("  Copy page at %p\n", page_address(pl->page));
        }
    }
}

// Find a copy page pool by ID
static struct pseudo_mm_pagepool *find_function_page_pool(unsigned long id)
{
    struct pseudo_mm_pagepool *fpp;
    u32 hash = hash_function(id);

    hash_for_each_possible(func_hash, fpp, hlist, hash) {
        if (fpp->funcid == id) {
            return fpp;
        }
    }

    return NULL;
}

// Find a free page chain by PFN
static struct source_page_list *find_page_chain(struct pseudo_mm_pagepool *fpp, struct page *src_page)
{
    struct source_page_list *pl;
    u32 hash = page_hash_function(src_page);

    hash_for_each_possible(fpp->srcpages_hash_list, pl, list_head, hash) {
        if (pl->page == src_page) {
            return pl;
        }
    }

    return NULL;
}

bool isWorH(unsigned long vaddr){
	return true;
}

struct srcpages_hash_list *create_srcpages_hash_list(struct pseudo_mm_pagepool *pool, struct page *head_page, unsigned long vaddr, int numa_node, unsigned int nr_pages)
{
	struct pseudo_mm_pagepool *pool;
    struct srcpages_hash_list *hh;
    unsigned int i;
	// struct page *head_page;
    struct page *new_page;

    hh = kmalloc(sizeof(*hh), GFP_KERNEL);
    if (!hh)
        return ERR_PTR(-ENOMEM);
	

	hh->vaddr = vaddr;
    hh->head_page = head_page;
    hh->nr_pages = nr_pages;
    INIT_LIST_HEAD(&hh->page_list);

    /*
     * 将头物理页加入链表。
     * 这里假设 struct page 内含有 list_head 成员（如 lru）用于链表操作，
     * 具体情况根据实际定义调整。
     */
    list_add(&head_page->lru, &hh->page_list);

    /* 为后续页分配新的物理页并拷贝头物理页内容 */
    for (i = 1; i < nr_pages; i++) {
         new_page = alloc_page(GFP_KERNEL);
         if (!new_page)
             goto err_free;
         copy_highpage(new_page, head_page);
         list_add_tail(&new_page->lru, &hh->page_list);
    }
	// vaddr as hash index
	unsigned int index = hash_ptr((void *)hh->vaddr, POOL_HASH_BITS);
	hlist_add_head(&hh->hnode, &pool->srcpages_hash_list);
    return hh;

err_free:
    /* 出错时释放已经分配的资源 */
    free_srcpages_hash_list(hh);
    return ERR_PTR(-ENOMEM);
}


void free_srcpages_hash_list(struct srcpages_hash_list *hh)
{
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &hh->page_list) {
         struct page *p = list_entry(pos, struct page, lru);
         __free_page(p);
    }
    kfree(hh);
}

/* 
 * setup read-only page table entry for vma in pseudo_mm
 * @start: start virtual address
 * @nr_pages: number of pages needed to be set
 * @pgoff: page offset of dax device
 */
 //hash pool
static unsigned long __setup_pool_for_func_vma(int id,
						struct pseudo_mm *pseudo_mm,
					    struct vm_area_struct *vma,
					    unsigned long start,
					    unsigned long nr_pages,
					    pgoff_t pgoff)
{
	struct pseudo_mm_backend *backend = pseudo_mm_get_backend();
	struct pseudo_mm_pagepool *pseudo_mm_hash = find_pseudo_mm_hash(id);
	struct page *head_page, **pages;
	phys_addr_t phys;
	pfn_t pfn;
	unsigned long ret = 0, i, vaddr;
	unsigned int copy_nr_pages;
	int numa_node;

	if (!backend->page) {
		pr_err("do not register mem backend for pseudo_mm\n");
		return -ENOENT;
	}

	// Map pages to dax device one by one
	// since insert_mixed api is insert one pfn at a time.
	// However, its performance not a big deal, since __setup_pt_for_vma is
	// called on prepare phase, it will not effect the attach performance.
	for (i = 0; i < nr_pages; i++) {
		vaddr = start + (i << PAGE_SHIFT);
		phys = (page_to_pfn(backend->page) + pgoff + i) << PAGE_SHIFT;
		if (phys == -1) {
			pr_warn("pgoff_to_phys(%ld) failed\n", pgoff + i);
			ret = -EFAULT;
			goto failed;
		}
		
		pfn = phys_to_pfn_t(phys, PFN_DEV | PFN_MAP);
		head_page = pfn_t_to_page(pfn);

		if(isWorH(vaddr)){
			copy_nr_pages=2;
			numa_node=0;
			// create_srcpages_hash_list(pseudo_mm_hash, page, vaddr, numa_node,copy_nr_pages);
			struct srcpages_hash_list *hh;
			unsigned int i;
			
			struct page *new_page;
		
			hh = kmalloc(sizeof(*hh), GFP_KERNEL);
			if (!hh)
				return ERR_PTR(-ENOMEM);

			hh->vaddr = vaddr;
			hh->head_page = head_page;
			hh->nr_pages = copy_nr_pages;
			INIT_LIST_HEAD(&hh->hnode);
		
			/* 为后续页分配新的物理页并拷贝头物理页内容 */
			for (i = 1; i < copy_nr_pages; i++) {
				struct pages_in_list *spil;
				spil = kmalloc(sizeof(*spil), GFP_KERNEL);
				if (!spil)
					return ERR_PTR(-ENOMEM);				
				new_page = alloc_pages_node(node, GFP_KERNEL, 0);
				 if (!new_page){
					ret=ERR_PTR(-ENOMEM);
					goto err_free;
				 }
				 copy_highpage(new_page, head_page);
				 spil->vaddr=vaddr;
				 spil->page=new_page;
				 spil->is_used=0;
				 spil->is_vaild=1;
				 INIT_LIST_HEAD(&spil->list);
				 list_add(&spil->list, &hh->pages_list);
			}
			// vaddr as hash index
			unsigned int index = hash_ptr((void *)hh->vaddr, POOL_HASH_BITS);
			hlist_add_head(&hh->hnode, &pseudo_mm_hash->srcpages_hash_list);
			// return hh;
		}
	}

	// pin_page = kmalloc(sizeof(*pin_page), GFP_KERNEL);
	// if (!pin_page) {
	// 	ret = -ENOMEM;
	// 	goto failed;
	// }

	// BUG_ON(nr_pin_pages != nr_pages);
	// INIT_LIST_HEAD(&pin_page->list);
	// pin_page->pages = pages;
	// pin_page->nr_pin_pages = nr_pin_pages;
	// list_add(&pin_page->list, &pseudo_mm->pages_list);

out:
	// if (pgmap) 
	// 	put_dev_pagemap(pgmap);
	// dax_read_unlock(id);
	return ret;

err_free:
	// if (nr_pin_pages > 0)
	// 	unpin_user_pages(pages, nr_pin_pages);
	// kvfree(pages);
	// if (pin_page)
	// 	kfree(pin_page);
	free_srcpages_hash_list(hh);
    // return ERR_PTR(-ENOMEM);
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
	// struct dev_dax *dev_dax;
	// struct pseudo_mm_pagepool *pseudo_mm_hash = find_pseudo_mm_hash(id);
	struct pseudo_mm_pin_pages *pin_page = NULL;
	struct dev_pagemap *pgmap = NULL;
	struct page *page, **pages;
	phys_addr_t phys;
	pfn_t pfn;
	unsigned long ret = 0, i, vaddr;
	long nr_pin_pages = 0;
	vm_fault_t vmf_ret;
	unsigned long ret;
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
		// if(isWorH(vaddr)){
		// 	copy_nr_pages=2;
		// 	numa_node=0;
		// 	create_srcpages_hash_list(pseudo_mm_hash, page, vaddr, numa_node,copy_nr_pages);
		// }

		// if (unlikely(!try_grab_page(page, FOLL_PIN))) {
		// 	ret = -ENOMEM;
		// 	goto failed;
		// }
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
	// struct pseudo_mm_pagepool *pseudo_mm_hash = find_pseudo_mm_hash(id);
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
	case DAX_MEM:
		ret = __setup_pt_for_vma_dax(id,pseudo_mm, vma, start,
					     size >> PAGE_SHIFT, pgoff);
		break;
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
