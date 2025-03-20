#define pr_fmt(fmt) "pseudo_mm:%s: " fmt, __func__

#include <asm/mmu_context.h>
#include <linux/hugetlb.h>
#include <linux/mempolicy.h>
#include <linux/mman.h>
#include <linux/rmap.h>
#include <linux/security.h>
#include <linux/userfaultfd_k.h>
#include <asm/cacheflush.h>
#include <linux/mm.h>
#include <linux/pseudo_mm.h>
#include <linux/slab.h>
#include <linux/xarray.h>
#include <linux/fs.h>
#include <linux/shmem_fs.h>
#include <linux/init.h>
#include <linux/namei.h>




#define stringify__(x) #x
#define stringify_(x) stringify__(x)
#define warn_weird_vma_flag(vma, pseudo_mm, flag_name)                  \
	pr_warn("Detect weird vma (#%lx - #%lx) with flag " stringify_( \
			flag_name) " in pseudo_mm %d !\n",              \
		vma->vm_start, vma->vm_end, pseudo_mm->id)

/* XArray used for id allocation */
DEFINE_XARRAY_ALLOC1(pseudo_mm_array);
DEFINE_XARRAY_ALLOC1(page_pool_array);
/* kmemcache for pseudo_mm struct */
static struct kmem_cache *pseudo_mm_cachep;
// static struct kmem_cache *page_pool_cachep;
static struct pseudo_mm_backend backend = {.filp = NULL, .page = NULL, .nr_pages = 0};
static pseudo_mm_rdma_pf_ops_t *pseudo_mm_rdma_pf_ops = NULL;
static int __pseudo_mm_rdma_prefer_node = NUMA_NO_NODE;

#define pseudo_mm_alloc() (kmem_cache_alloc(pseudo_mm_cachep, GFP_KERNEL))
// #define page_pool_alloc() (kmem_cache_alloc(page_pool_cachep, GFP_KERNEL))

#define PSEUDO_MM_ID_MAX INT_MAX

// #define HASH_BITS 10 

#ifdef PSEUDO_MM_DEBUG
static bool __maybe_unused show_rmap_vma(struct folio *folio,
					 struct vm_area_struct *vma,
					 unsigned long address, void *arg)
{
	pr_info("\tweird page %p (mapcount %d) found in vma %p at address #%lx\n",
		&folio->page, folio_mapcount(folio), vma, address);
	return true;
}

void __maybe_unused debug_weird_page(struct page *page, int expected_mapcount)
{
	struct folio *folio = page_folio(page);
	int we_locked = 0;
	struct rmap_walk_control rwc = {
		.rmap_one = show_rmap_vma,
		.arg = NULL,
	};

	if (folio_mapcount(folio) == expected_mapcount)
		return;

	if (!folio_test_locked(folio)) {
		we_locked = 1;
		folio_lock(folio);
	}
	rmap_walk(folio, &rwc);
	if (we_locked)
		folio_unlock(folio);
}
#else
void __maybe_unused debug_weird_page(struct page *page, int expected_mapcount)
{
}
#endif

int __init pseudo_mm_cache_init(void)
{
	pseudo_mm_cachep = KMEM_CACHE(pseudo_mm, SLAB_PANIC | SLAB_ACCOUNT);
	if (!pseudo_mm_cachep)
		return -ENOMEM;
	return 0;
}

// int __init page_pool_cache_init(void)
// {
// 	page_pool_cachep = KMEM_CACHE(func_page_pool, SLAB_PANIC | SLAB_ACCOUNT);
// 	if (!page_pool_cachep)
// 		return -ENOMEM;
// 	return 0;
// }

postcore_initcall(pseudo_mm_cache_init);
// postcore_initcall(page_pool_cache_init);


unsigned long register_pseudo_mm_rdma_pf_handler(pseudo_mm_rdma_pf_ops_t *op,
						 int node)
{
	if (pseudo_mm_rdma_pf_ops != NULL) {
		pr_err("only allowed to register one pseudo_mm_rdma_pf_handler, already set to %p!",
		       pseudo_mm_rdma_pf_ops);
		return -EEXIST;
	}
	pseudo_mm_rdma_pf_ops = op;
	if (node < 0 && node != NUMA_NO_NODE)
		return -EINVAL;
	if (!node_online(node)) {
		pr_warn_once("node %d not online in %s\n", node, __FUNCTION__);
		return -EINVAL;
	}
	__pseudo_mm_rdma_prefer_node = node;
	return 0;
}
EXPORT_SYMBOL(register_pseudo_mm_rdma_pf_handler);

inline int pseudo_mm_rdma_prefer_node(void)
{
	return __pseudo_mm_rdma_prefer_node;
}
EXPORT_SYMBOL(pseudo_mm_rdma_prefer_node);

int pseudo_mm_rdma_pf_handle(struct page *page, pgoff_t remote_pgoff)
{
	if (pseudo_mm_rdma_pf_ops)
		return pseudo_mm_rdma_pf_ops(page, remote_pgoff);
	return -ENOENT;
}

unsigned long register_backend_dax_device(int fd)
{
	struct file *backend_file;
	unsigned long ret;

	// if (backend.filp)
	// return -EEXIST;
	// For now, the backend do not keep state across
	// different request of setup page table, so I allow to register
	// multiple times for flexibility.
	if (backend.filp) {
		fput(backend.filp);
		pr_warn("pseudo_mm backend dax device has changed\n");
		backend.filp = NULL;
	}

	backend_file = fget(fd);
	if (!backend_file)
		return -EBADF;
	if (!IS_DAX(backend_file->f_mapping->host)) {
		ret = -EBADF;
		goto err;
	}
	backend.filp = backend_file;

	return 0;
err:
	fput(backend_file);
	return ret;
}

unsigned long register_backend_memory(int node, int order)
{	
	struct page *page;
	void *virt_addr;
	u64 base_pfn, phys_addr;
	u64 nr_pages;

	if(backend.page){
		base_pfn = page_to_pfn(backend.page);
		nr_pages = backend.nr_pages;
		free_contig_range(base_pfn, backend.nr_pages);
		pr_warn("pseudo_mm backend memory has changed\n");
		backend.page = NULL;
		backend.nr_pages = 0;
	}

	nr_pages = 1UL << order;
	page = alloc_contig_pages(nr_pages, GFP_KERNEL, node, NULL);
	if (!page){
		pr_err("alloc pages failed\n");
		return -1;	
	}
	backend.page = page;
	backend.nr_pages = (1UL << order);

	base_pfn = page_to_pfn(page);
	phys_addr = base_pfn << PAGE_SHIFT;

	pr_info("alloc %ld pages from node numa %d, base_phy_addr: %llx\n", 1UL << order, node,phys_addr);
	
	return 0;
}

u64 pseudo_mm_phy_addr(void)
{	
	struct page *page;
	u64 base_pfn, phys_addr;

	if(!backend.page){
		return -1;
	}

	page = backend.page;
	base_pfn = page_to_pfn(page);
	phys_addr = base_pfn << PAGE_SHIFT;
	
	return phys_addr;
}



inline bool pseudo_mm_rdma_pf_handler_enable(void)
{
	return pseudo_mm_rdma_pf_ops != NULL;
}

/*
 * create a pseudo_mm struct and initialize it
 *
 * Return its id (> 0) when SUCCESS, return errno otherwise
 */

int create_pseudo_mm(void)
{
	struct mm_struct *mm;
	struct pseudo_mm *pseudo_mm;
	struct xa_limit limit;
	int ret, id;

	mm = mm_alloc_wo_task();
	if (!mm)
		return -ENOMEM;

	pseudo_mm = pseudo_mm_alloc();
	if (!pseudo_mm) {
		ret = -ENOMEM;
		goto drop_mm;
	}
	pseudo_mm->mm = mm;
	INIT_LIST_HEAD(&pseudo_mm->pages_list);

	// insert newly created pseudo into xarray
	limit = XA_LIMIT(1, PSEUDO_MM_ID_MAX);
	ret = xa_alloc(&pseudo_mm_array, &id, pseudo_mm, limit, GFP_KERNEL);
	if (ret < 0)
		goto drop_pseudo_mm;

	pseudo_mm->id = id;

	return id;

drop_pseudo_mm:
	kmem_cache_free(pseudo_mm_cachep, pseudo_mm);
drop_mm:
	mmdrop(mm);
	return ret;
}



int create_func_page_pool(void)
{
	struct xa_limit limit;
	struct func_page_pool *fpp;
	spinlock_t *bucket_locks;
	struct hlist_head *buckets;
	int id, ret;

	fpp=kmalloc(sizeof(*fpp),GFP_KERNEL);
	if (!fpp) {
		ret = -ENOMEM;
		goto failed_page_pool;
	}
	pr_info("[Create_func_page_pool] start\n");
	//Initialize hashtable and lock
	fpp->hash_bits=10;
	int hnum=1<<(fpp->hash_bits);
	buckets = kmalloc_array(hnum,sizeof(struct hlist_head),GFP_KERNEL);
	pr_info("Buckets address %llx\n", buckets);
	if(!buckets){
		ret = -ENOMEM;
		goto drop_page_pool;
	}

	bucket_locks = kmalloc_array(hnum,sizeof(spinlock_t),GFP_KERNEL);
	pr_info("Buckets locks address %llx\n", bucket_locks);
	if(!bucket_locks){
		ret = -ENOMEM;
		goto drop_page_pool;
	}

	fpp->buckets=buckets;
	fpp->bucket_locks=bucket_locks;
	fpp->id = id;
	atomic_set(&fpp->refcount, 1);

	for (int i = 0; i < hnum; i++) {
		// pr_info("Initializing bucket %d at address %llx\n", i, &fpp->buckets[i]);
		// pr_info("Initializing bucket_lock %d at address %llx\n", i, &fpp->bucket_locks[i]);
		INIT_HLIST_HEAD(&fpp->buckets[i]);
		spin_lock_init(&fpp->bucket_locks[i]);
	}

	// insert newly created pagepool into xarray
	limit = XA_LIMIT(1, PSEUDO_MM_ID_MAX);
	ret = xa_alloc(&page_pool_array, &id, fpp, limit, GFP_KERNEL);
	if (ret < 0)
		goto drop_page_pool;

	pr_info("create_func_page_pool created:%d\n",fpp->id);
	return id;

drop_page_pool:
	kfree(fpp->buckets);
	kfree(fpp->bucket_locks);
	kfree(fpp);
	return ret;

failed_page_pool:
	return ret;
}

struct pseudo_mm *find_pseudo_mm(int id)
{
	struct pseudo_mm *pseudo_mm = NULL;
	unsigned long orig_id;

	// invalid id
	if (unlikely(id <= 0)) {
		pr_warn("process %d find pseudo_mm with invalid id = %d\n",
			current->pid, id);
		return NULL;
	}

	orig_id = id;
	pseudo_mm = xa_find(&pseudo_mm_array, &orig_id, orig_id, XA_PRESENT);
	WARN_ON(pseudo_mm && pseudo_mm->id != id);
	return pseudo_mm;
}

struct func_page_pool *find_page_pool(int id)
{
	struct func_page_pool *fpp = NULL;
	unsigned long orig_id;

	// invalid id
	if (unlikely(id <= 0)) {
		pr_warn("process %d find pseudo_mm with invalid id = %d\n",
			current->pid, id);
		return NULL;
	}

	orig_id = id;
	fpp = xa_find(&page_pool_array, &orig_id, orig_id, XA_PRESENT);
	WARN_ON(fpp && fpp->id != id);
	return fpp;
}

static void put_pseudo_mm(struct pseudo_mm *pseudo_mm)
{
	struct pseudo_mm_pin_pages *pin_page, *tmp;
	list_for_each_entry_safe(pin_page, tmp, &pseudo_mm->pages_list, list) {
		list_del(&pin_page->list);
		unpin_user_pages(pin_page->pages, pin_page->nr_pin_pages);
		kvfree(pin_page->pages);
		kfree(pin_page);
	}
	//new page refcount--(==1 now)
	if (pseudo_mm->mm)
		mmput(pseudo_mm->mm);
	if (pseudo_mm->id > 0)
		xa_erase(&pseudo_mm_array, pseudo_mm->id);
	kmem_cache_free(pseudo_mm_cachep, pseudo_mm);
}

static void put_page_pool(struct func_page_pool *fpp)
{
	int bucket_num=1<<(fpp->hash_bits);
	for(int i=0;i< bucket_num;i++){
		struct special_page_entry *bucketnode;
		struct hlist_node *tmp;
		unsigned long flags;

		//add lock before delete the bucket
		spin_lock_irqsave(&fpp->bucket_locks[i],flags);

		//struct,stroage of hlist_node, hlist_head, hnode name in struct 
		hlist_for_each_entry_safe(bucketnode,tmp,&fpp->buckets[i],node){
			struct copy_page *cpage,*tmpp;
			list_for_each_entry_safe(cpage, tmpp, &bucketnode->free_copies,list) {
				//release phy-page
				__free_page(cpage->page);
				//del node
				list_del(&cpage->list);
				//del struct
				kfree(cpage);
			}
			list_for_each_entry_safe(cpage, tmpp, &bucketnode->used_copies,list) {
				//release phy-page
				__free_page(cpage->page);
				//del node
				list_del(&cpage->list);
				//del struct
				kfree(cpage);
			}
			hlist_del(&bucketnode->node);
			kfree(bucketnode);
		}

		//unlock bucket
		spin_unlock_irqrestore(&fpp->bucket_locks[i],flags);
	}

	kfree(fpp->buckets);
	kfree(fpp->bucket_locks);
	kfree(fpp);

	if (fpp->id > 0)
		xa_erase(&page_pool_array, fpp->id);
}

void put_pseudo_mm_with_id(int id)
{
	struct pseudo_mm *pseudo_mm;
	pr_info("process %d put pseudo_mm id %d\n", current->pid, id);
	// id == -1 is a specical case to delete all pseudo_mm
	if (id == -1) {
		unsigned long idx;
		xa_for_each(&pseudo_mm_array, idx, pseudo_mm) {
			if (pseudo_mm)
				put_pseudo_mm(pseudo_mm);
		}
		return;
	}

	pseudo_mm = find_pseudo_mm(id);
	if (pseudo_mm)
		put_pseudo_mm(pseudo_mm);
}

void put_page_pool_with_id(int id)
{
	// struct pseudo_mm *pseudo_mm;
	struct func_page_pool *fpp;
	pr_info("process %d put pseudo_mm id %d\n", current->pid, id);
	// id == -1 is a specical case to delete all pseudo_mm
	if (id == -1) {
		unsigned long idx;
		xa_for_each(&page_pool_array, idx, fpp) {
			if (fpp)
				put_page_pool(fpp);
		}
		return;
	}

	fpp = find_page_pool(id);
	if (fpp)
		put_page_pool(fpp);
}


/* 查找特殊页条目 */
struct special_page_entry *find_special_page(int id, unsigned long vaddr) {
	struct func_page_pool *fpp = find_page_pool(id);
	// get hash index
    u32 hash = hash_long(vaddr>>PAGE_SHIFT,fpp->hash_bits);
    struct special_page_entry *entry;
	struct hlist_node *tmp;
    unsigned long flags;

    spin_lock_irqsave(&fpp->bucket_locks[hash], flags);
    hlist_for_each_entry_safe(entry, tmp, &fpp->buckets[hash], node) {
        if (entry->vaddr == vaddr) {
            spin_unlock_irqrestore(&fpp->bucket_locks[hash], flags);
            return entry;
        }
    }
    spin_unlock_irqrestore(&fpp->bucket_locks[hash], flags);
    return NULL;
}

/* 分配一个副本页给进程 */
struct copy_page *snapshot_alloc_copy(struct special_page_entry *entry) {
    struct copy_page *copy;
    unsigned long flags;

    spin_lock_irqsave(&entry->lock, flags);

    /* 优先从空闲链表获取副本页 */
    if (!list_empty(&entry->free_copies)) {
        copy = list_first_entry(&entry->free_copies, struct copy_page, list);
        list_move(&copy->list, &entry->used_copies);
        copy->state = COPY_PAGE_IN_USE;
        spin_unlock_irqrestore(&entry->lock, flags);
        return copy;
    }

	return NULL;

    // /* 无空闲页时分配新副本页 */
    // copy = kzalloc(sizeof(*copy), GFP_ATOMIC);
    // if (!copy) {
    //     spin_unlock_irqrestore(&entry->lock, flags);
    //     return NULL;
    // }

    // copy->page = alloc_page(GFP_KERNEL); // 分配物理页
    // if (!copy->page) {
    //     kfree(copy);
    //     spin_unlock_irqrestore(&entry->lock, flags);
    //     return NULL;
    // }

    // /* 复制原始页内容 */
    // memcpy(page_address(copy->page), page_address(entry->master_page), PAGE_SIZE);

    // /* 添加到已用链表 */
    // copy->state = COPY_PAGE_IN_USE;
    // INIT_LIST_HEAD(&copy->list);
    // list_add(&copy->list, &entry->used_copies);
    // spin_unlock_irqrestore(&entry->lock, flags);

    // return copy;
}

/* 释放副本页（由进程退出时触发） */
void snapshot_free_copy(struct special_page_entry *entry, struct copy_page *copy) {
    // unsigned long flags;

    // spin_lock_irqsave(&entry->lock, flags);
    // list_move(&copy->list, &entry->free_copies);
    // copy->state = COPY_PAGE_FREE;
    // spin_unlock_irqrestore(&entry->lock, flags);
}

unsigned long pseudo_mm_add_map(int id, unsigned long start, unsigned long size,
				unsigned long prot, unsigned long flags, int fd,
				pgoff_t pgoff)
{
	struct pseudo_mm *pseudo_mm;
	struct mm_struct *mm;
	struct file *file = NULL;
	unsigned long ret;

	// we only accept PageAligned address and size
	if (!PAGE_ALIGNED(start) || !PAGE_ALIGNED(size) || size == 0) {
		return -EINVAL;
	}
	// do not support huge tlb now
	if (flags & MAP_HUGETLB)
		return -EINVAL;

	if ((flags & (MAP_ANONYMOUS | MAP_SHARED)) ==
	    (MAP_ANONYMOUS | MAP_SHARED)) {
		pr_warn("do not support anonymous shared mapping!\n");
		return -EINVAL;
	}

	if ((flags & MAP_ANONYMOUS) && fd != -1)
		return -EINVAL;

	if (!(flags & MAP_ANONYMOUS)) {
		file = fget(fd);
		if (!file)
			return -EBADF;
	}

	pseudo_mm = find_pseudo_mm(id);
	if (!pseudo_mm) {
		ret = -ENOENT;
		goto out;
	}
	mm = pseudo_mm->mm;

	if (mmap_write_lock_killable(mm)) {
		ret = -EINTR;
		goto out;
	}
	// we skip userfaultfd here
	ret = do_mmap_to(mm, file, start, size, prot, flags, pgoff, NULL);
	if (ret != start)
		pr_warn("Warning: add anonymous map to pseudo_mm at #%lx, but result at #%lx\n",
			start, ret);
	mmap_write_unlock(mm);
	// userfaultfd_unmap_complete(mm, &uf);
	if (!IS_ERR_VALUE(ret))
		ret = 0;
out:
	if (file)
		fput(file);
	return ret;
}

/*
 * pseudo_dup_mmap() - insert mmap from pseudo_mm into mm
 * @id: id of pseudo_mm
 * @pseudo_mm: The source to insert from
 * @tsk: Owner of the @mm
 * @mm: The destination to insert into
 *
 * Similar to dup_mmap() which in kernel/fork.c, but we are doing insert not dup.
 * Some steps will be skipped, while some additional step will be added.
 */
static unsigned long pseudo_mm_attach_mmap(int id, struct pseudo_mm *pseudo_mm,
					   struct task_struct *tsk,
					   struct mm_struct *mm)
{
	//mm-struct Metadata copy: mm copied oldmm via vm_area_dup()
	struct mm_struct *oldmm = pseudo_mm->mm;
	struct vm_area_struct *mpnt, *tmp;
	int retval = 0;
	unsigned long addr,ret;
	unsigned long charge = 0, tmp_vm_flags;
	LIST_HEAD(uf);
	MA_STATE(old_mas, &oldmm->mm_mt, 0, 0);
	MA_STATE(mas, &mm->mm_mt, 0, 0);

	pr_info("pseudo_mm_attach_mmap\n");
	// ret=pseudo_mm_getpte_from_oldmm(oldmm);
	// if(ret){
	// 	pr_err("Can't open oldmm\n");
	// 	goto fail_getoldpte;
	// }
	uprobe_start_dup_mmap();
	if (mmap_write_lock_killable(oldmm)) {
		retval = -EINTR;
		goto fail_uprobe_end;
	}
	flush_cache_dup_mm(oldmm);
	uprobe_dup_mmap(oldmm, mm);
	/*
	 * Not linked in yet - no deadlock potential:
	 */
	mmap_write_lock_nested(mm, SINGLE_DEPTH_NESTING);

	// do not dup mm exe file

	mm->total_vm += oldmm->total_vm;
	mm->data_vm += oldmm->data_vm;
	mm->exec_vm += oldmm->exec_vm;
	mm->stack_vm += oldmm->stack_vm;

	// do not do ksm_fork or khugepaged_fork

	retval = mas_expected_entries(&mas, oldmm->map_count);
	if (retval)
		goto out;

	mas_for_each(&old_mas, mpnt, ULONG_MAX)
	{
		struct file *file;

		// This is roughly weird
		if (mpnt->vm_flags & VM_DONTCOPY) {
			warn_weird_vma_flag(mpnt, pseudo_mm, DONTCOPY);
			vm_stat_account(mm, mpnt->vm_flags, -vma_pages(mpnt));
			continue;
		}
		charge = 0;
		/*
		 * Don't duplicate many vmas if we've been oom-killed (for
		 * example)
		 */
		if (fatal_signal_pending(tsk)) {
			retval = -EINTR;
			goto loop_out;
		}
		if (mpnt->vm_flags & VM_ACCOUNT) {
			unsigned long len = vma_pages(mpnt);

			if (security_vm_enough_memory_mm(oldmm, len)) /* sic */
				goto fail_nomem;
			charge = len;
		}
		tmp = vm_area_dup(mpnt);
		if (!tmp)
			goto fail_nomem;
		retval = vma_dup_policy(mpnt, tmp);
		if (retval)
			goto fail_nomem_policy;
		tmp->vm_mm = mm;
		retval = dup_userfaultfd(tmp, &uf);
		if (retval)
			goto fail_nomem_anon_vma_fork;
		if (tmp->vm_flags & VM_WIPEONFORK) {
			/*
			 * VM_WIPEONFORK gets a clean slate in the child.
			 * Don't prepare anon_vma until fault since we don't
			 * copy page for current vma.
			 */
			warn_weird_vma_flag(tmp, pseudo_mm, WIPEONFORK);
			tmp->anon_vma = NULL;
		} else if (anon_vma_fork(tmp, mpnt))
			goto fail_nomem_anon_vma_fork;

		tmp->vm_flags &= ~(VM_LOCKED | VM_LOCKONFAULT);
		// newly created vma should not be master
		tmp->pseudo_mm_flag &= ~PSEUDO_MM_VMA_MASTER;
		// we try to setup a new zero shmem file in page_fault_handler
		if (vma_is_pseudo_anon_shared(mpnt)) {
			// tmp->pseudo_mm_flag |= id;
			// setup a new sheme zero file when attach
			// pr_info("pseudo_mm create new vma %p old vm_file's mapping = #%p",
			// 	tmp, tmp->vm_file->f_mapping);
			pr_warn("pseudo_mm create anon shared vma which is not well supported\n");
			tmp->vm_file = NULL;
			retval = shmem_zero_setup(tmp);
			if (retval)
				goto fail_with_retval;
			file = tmp->vm_file;
			BUG_ON(!file);

			i_mmap_lock_write(file->f_mapping);
			mapping_allow_writable(file->f_mapping);
			flush_dcache_mmap_lock(file->f_mapping);
			vma_interval_tree_insert(tmp, &file->f_mapping->i_mmap);
			flush_dcache_mmap_unlock(file->f_mapping);
			i_mmap_unlock_write(file->f_mapping);
			mapping_unmap_writable(file->f_mapping);
			/* TODO (huang-jl) is this needed ? */
			// uprobe_mmap(tmp);
			goto skip_normal_file;
		}

		// TODO (huang-jl) check file-related logic
		file = tmp->vm_file;
		if (file) {
			struct address_space *mapping = file->f_mapping;

			get_file(file);
			i_mmap_lock_write(mapping);
			if (tmp->vm_flags & VM_SHARED)
				mapping_allow_writable(mapping);
			flush_dcache_mmap_lock(mapping);
			/* insert tmp into the share list, just after mpnt */
			vma_interval_tree_insert_after(tmp, mpnt,
						       &mapping->i_mmap);
			flush_dcache_mmap_unlock(mapping);
			i_mmap_unlock_write(mapping);
		}

skip_normal_file:
		// TODO (huang-jl) how about file-backed mapping ?
		// Want to make sure that all pages are copy-on-write,
		// so simply mark it PRIVATE here and restore after copy_page_range().
		// The pte will be write-protected.
		if (vma_is_pseudo_anon_shared(mpnt)) {
			WARN(tmp->vm_flags != mpnt->vm_flags,
			     "tmp and mpnt flag corrupt: %lx vs %lx\n",
			     tmp->vm_flags, mpnt->vm_flags);
			tmp_vm_flags = tmp->vm_flags;
			tmp->vm_flags &= ~VM_SHARED;
			mpnt->vm_flags &= ~VM_SHARED;
		}
		/*
		 * TODO (huang-jl) Copy/update hugetlb private vma information.
		 */
		if (is_vm_hugetlb_page(tmp)) {
			warn_weird_vma_flag(tmp, pseudo_mm, HUGHTLB);
			hugetlb_dup_vma_private(tmp);
		}

		/* Link the vma into the MT, and
		 * make sure that there is **no overlapping**.
		 */
		mas_set_range(&mas, tmp->vm_start, tmp->vm_end - 1);
		mas_insert(&mas, tmp);
		if (mas_is_err(&mas)) {
			retval = xa_err(mas.node);
			goto fail_with_retval;
		}

		// mas.index = tmp->vm_start;
		// mas.last = tmp->vm_end - 1;
		// mas_store(&mas, tmp);
		// if (mas_is_err(&mas))
		// 	goto fail_nomem_mas_store;

		mm->map_count++;

		if (!(tmp->vm_flags & VM_WIPEONFORK))
			retval = copy_page_range(tmp, mpnt);

#ifdef PSEUDO_MM_DEBUG
		// Debug: check for page table entry
		if (vma_is_pseudo_anon_shared(tmp)) {
			addr = tmp->vm_start;
			while (addr < tmp->vm_end) {
				pgd_t *pgd = pgd_offset(mm, addr);
				WARN(pgd_none(*pgd), "va #%lx pgd is none",
				     addr);
				p4d_t *p4d = p4d_offset(pgd, addr);
				WARN(p4d_none(*p4d), "va #%lx p4d is none",
				     addr);
				pud_t *pud = pud_offset(p4d, addr);
				WARN(pud_none(*pud), "va #%lx pud is none",
				     addr);
				pmd_t *pmd = pmd_offset(pud, addr);
				WARN(pmd_none(*pmd), "va #%lx pmd is none",
				     addr);
				pte_t *pte = pte_offset_kernel(pmd, addr);
				WARN(pte_none(*pte), "va #%lx pte is none",
				     addr);
				WARN(pte_write(*pte), "va #%lx pte is writable",
				     addr);
				addr += PAGE_SIZE;
			}
		}
#endif

		if (vma_is_pseudo_anon_shared(mpnt)) {
			tmp->vm_flags = tmp_vm_flags;
			mpnt->vm_flags = tmp_vm_flags;
		}

		if (tmp->vm_ops && tmp->vm_ops->open)
			tmp->vm_ops->open(tmp);

		if (retval)
			goto loop_out;
	}
	/* a new mm has just been created */
	// retval = arch_dup_mmap(oldmm, mm);
loop_out:
	mas_destroy(&mas);
out:
	mmap_write_unlock(mm);
	flush_tlb_mm(oldmm);
	mmap_write_unlock(oldmm);
	dup_userfaultfd_complete(&uf);
fail_uprobe_end:
	uprobe_end_dup_mmap();
	ret=pseudo_mm_getpte_from_mm(mm);
	if(ret){
		pr_err("Can't open oldmm\n");
		goto fail_getoldpte;
	}
	return retval;

fail_getoldpte:
	retval = -ENOMEM;
	goto fail_uprobe_end;
fail_nomem_anon_vma_fork:
	mpol_put(vma_policy(tmp));
fail_nomem_policy:
	vm_area_free(tmp);
fail_nomem:
	retval = -ENOMEM;
	vm_unacct_memory(charge);
	goto loop_out;

fail_with_retval:
	unlink_anon_vmas(tmp);
	mpol_put(vma_policy(tmp));
	vm_area_free(tmp);
	vm_unacct_memory(charge);
	goto loop_out;
}
//TODO
// static unsigned long pseudo_mm_attach_remap(int id, struct pseudo_mm *pseudo_mm,
// 					   struct task_struct *tsk,
// 					   struct mm_struct *mm)
// {

// }



// TODO:add argvs [id,id2],for remapped attach
unsigned long pseudo_mm_attach(pid_t pid, int id)
{
	struct task_struct *tsk;
	struct mm_struct *tsk_mm;
	struct pseudo_mm *pseudo_mm;
	unsigned long err;

	pseudo_mm = find_pseudo_mm(id);
	// struct mm_struct *mm=pseudo_mm->mm;
	pr_info("Pseudo_mm attach with id %d\n", id);
	if (!pseudo_mm) {
		pr_warn("cannot find pseudo_mm with id %d\n", id);
		return -ENOENT;
	}

	rcu_read_lock();
	tsk = find_task_by_vpid(pid);
	if (!tsk) {
		pr_warn("cannot find task of pid %d\n", pid);
		return -ESRCH;
	}
	rcu_read_unlock();

	tsk_mm = get_task_mm(tsk);
	if (!tsk_mm) {
		// no mm_struct for task, do nothing
		pr_warn("cannot get tsk mm of pid %d!\n", pid);
		return 0;
	}
	// pseudo_mm_getpte_from_oldmm(pseudo_mm->mm);
	// pseudo_mm_getpte_from_mm(tsk_mm);

	err = pseudo_mm_attach_mmap(id, pseudo_mm, tsk, tsk_mm);
	// err = pseudo_mm_attach_remap(id, pseudo_mm, tsk, tsk_mm);
	if (err)
		pr_warn("attach pseudo_mm (id = %d)'s mmap to pid %d failed!\n",
			id, pid);

	mmput(tsk_mm);

	return err ? err : 0;
}

unsigned long pseudo_template_getpte(struct mm_struct *mm, int id) {

	struct maple_tree *mt;
	struct vm_area_struct *vma;
	unsigned long err;
	
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    // unsigned long vaddr, i, len;
    unsigned long len;
    struct file *file;
    loff_t pos = 0;
    char *log;
	char filename[256];

    if (!mm) {
        pr_warn("cannot find pseudo_mm with id %d\n", id);
        return -ENOENT;
    }

	mt=&mm->mm_mt;
	MA_STATE(mas, mt, 0, 0);
	snprintf(filename, sizeof(filename), "/tmp/pte_bf_%d.txt", id);

    file = filp_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(file)) {
        pr_warn("Failed to open file /tmp/pte_log.txt\n");
        mmput(mm);
        return -ENOENT;
    }

    log = kmalloc(256, GFP_KERNEL);
    if (!log) {
        pr_warn("Failed to allocate memory for log buffer\n");
        filp_close(file, NULL);
        mmput(mm);
        return -ENOENT;
    }

	rcu_read_lock();
	mas_for_each(&mas, vma, ULONG_MAX) {
        unsigned long vma_start = vma->vm_start;
        unsigned long vma_end = vma->vm_end;
        unsigned long vma_size = vma_end - vma_start;

        if (vma_size <= 0)
            continue;

        len = snprintf(log, 256, "VMA: 0x%lx - 0x%lx\n", vma_start, vma_end);
        kernel_write(file, log, len, &pos);
        pr_info("VMA: 0x%lx - 0x%lx\n", vma_start, vma_end);

        unsigned long vma_nr_pages = vma_size >> PAGE_SHIFT;
        unsigned long j;
        for (j = 0; j < vma_nr_pages; j++) {
            unsigned long current_vaddr = vma_start + (j << PAGE_SHIFT);

            pgd = pgd_offset(mm, current_vaddr);
            if (pgd_none(*pgd) || pgd_bad(*pgd))
                continue;

            p4d = p4d_offset(pgd, current_vaddr);
            if (p4d_none(*p4d) || p4d_bad(*p4d))
                continue;

            pud = pud_offset(p4d, current_vaddr);
            if (pud_none(*pud) || pud_bad(*pud))
                continue;

            pmd = pmd_offset(pud, current_vaddr);
            if (pmd_none(*pmd) || pmd_bad(*pmd))
                continue;

            pte = pte_offset_map(pmd, current_vaddr);
            if (!pte || pte_none(*pte)) {
                pte_unmap(pte);
                continue;
            }

            unsigned long pfn = pte_pfn(*pte);
            pgprot_t prot = pte_pgprot(*pte);

            len = snprintf(log, 256, "Vaddr: 0x%lx, PFN: 0x%lx, Prot: 0x%lx\n",
                          current_vaddr, pfn, pgprot_val(prot));
            kernel_write(file, log, len, &pos);
            pr_info("Vaddr: 0x%lx, PFN: 0x%lx, Prot: 0x%lx\n",
                   current_vaddr, pfn, pgprot_val(prot));

            pte_unmap(pte);
        }
	}
	rcu_read_unlock();


    kfree(log);
    filp_close(file, NULL);
    mmput(mm);
    return 0;
}


unsigned long pseudo_mm_getpte_from_mm(struct mm_struct *mm) {
    // struct mm_struct *mm=inmm;
	struct maple_tree *mt;
	struct vm_area_struct *vma;
	
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    // unsigned long vaddr, i, len;
    unsigned long len;
    struct file *file;
    loff_t pos = 0;
    char *log;
	ssize_t ret;

	mt=&mm->mm_mt;
	MA_STATE(mas, mt, 0, 0);

    file = filp_open("/tmp/pte_log_bf_mm.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(file)) {
        pr_warn("Failed to open file /tmp/pte_log_bf_mm.txt\n");
        // mmput(mm);
        return -ENOENT;
    }

    log = kmalloc(256, GFP_KERNEL);
    if (!log) {
        pr_warn("Failed to allocate memory for log buffer\n");
        filp_close(file, NULL);
        // mmput(mm);
        return -ENOENT;
    }

	rcu_read_lock();
	mas_for_each(&mas, vma, ULONG_MAX) {
        unsigned long vma_start = vma->vm_start;
        unsigned long vma_end = vma->vm_end;
        unsigned long vma_size = vma_end - vma_start;

        if (vma_size <= 0)
            continue;

        len = snprintf(log, 256, "VMA: 0x%lx - 0x%lx\n", vma_start, vma_end);
        ret=kernel_write(file, log, len, &pos);
		if (ret != len) {
            pr_warn("Failed to write VMA info: %ld\n", ret);
        }
        // pr_info("VMA: 0x%lx - 0x%lx\n", vma_start, vma_end);

        unsigned long vma_nr_pages = vma_size >> PAGE_SHIFT;
        unsigned long j;
        for (j = 0; j < vma_nr_pages; j++) {
            unsigned long current_vaddr = vma_start + (j << PAGE_SHIFT);

            pgd = pgd_offset(mm, current_vaddr);
            if (pgd_none(*pgd) || pgd_bad(*pgd))
                continue;

            p4d = p4d_offset(pgd, current_vaddr);
            if (p4d_none(*p4d) || p4d_bad(*p4d))
                continue;

            pud = pud_offset(p4d, current_vaddr);
            if (pud_none(*pud) || pud_bad(*pud))
                continue;

            pmd = pmd_offset(pud, current_vaddr);
            if (pmd_none(*pmd) || pmd_bad(*pmd))
                continue;

            pte = pte_offset_map(pmd, current_vaddr);
            if (!pte || pte_none(*pte)) {
                pte_unmap(pte);
                continue;
            }

            unsigned long pfn = pte_pfn(*pte);
            pgprot_t prot = pte_pgprot(*pte);

            len = snprintf(log, 256, "Vaddr: 0x%lx, PFN: 0x%lx, Prot: 0x%lx\n",
                          current_vaddr, pfn, pgprot_val(prot));
            ret=kernel_write(file, log, len, &pos);
			if (ret != len) {
				pr_warn("Failed to write VMA info: %ld\n", ret);
			}
            
            pte_unmap(pte);
        }
	}
	rcu_read_unlock();
	vfs_fsync(file, 0);
	print_file_path(file);

    kfree(log);
    filp_close(file, NULL);
    // mmput(mm);
    return 0;
}


void print_file_path(struct file *file) {
    char *path_buf;
    struct path file_path;
    char *abs_path;

    // 1. 分配缓冲区（PATH_MAX 通常为 4096）
    path_buf = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!path_buf) {
        pr_err("Failed to allocate path buffer\n");
        return;
    }

    // 2. 获取文件的 path 结构（dentry + vfsmount）
    file_path = file->f_path;
    path_get(&file_path); // 增加引用计数

    // 3. 生成绝对路径
    abs_path = d_path(&file_path, path_buf, PATH_MAX);
    if (IS_ERR(abs_path)) {
        pr_err("Failed to get path: %ld\n", PTR_ERR(abs_path));
        kfree(path_buf);
        return;
    }

    // 4. 输出路径（如写入内核日志）
    pr_info("File path: %s\n", abs_path);

    // 5. 释放资源
    path_put(&file_path);
    kfree(path_buf);
}


unsigned long pseudo_mm_getpte(pid_t pid) {
    struct task_struct *task;
    struct mm_struct *mm;
	struct maple_tree *mt;
	struct vm_area_struct *vma;
	
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    // unsigned long vaddr, i, len;
    unsigned long len;
    struct file *file;
    loff_t pos = 0;
    char *log;
	char filename[256];

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        pr_warn("cannot find task with pid %d\n", pid);
        return -ENOENT;
    }

    mm = get_task_mm(task);
    if (!mm) {
        pr_warn("Failed to get mm_struct for PID %d\n", pid);
        return -ENOENT;
    }

	mt=&mm->mm_mt;
	MA_STATE(mas, mt, 0, 0);

	snprintf(filename, sizeof(filename), "/tmp/pte_af_%d.txt", pid);

    file = filp_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(file)) {
        pr_warn("Failed to open file /tmp/pte_log.txt\n");
        mmput(mm);
        return -ENOENT;
    }

    log = kmalloc(256, GFP_KERNEL);
    if (!log) {
        pr_warn("Failed to allocate memory for log buffer\n");
        filp_close(file, NULL);
        mmput(mm);
        return -ENOENT;
    }

	rcu_read_lock();
	mas_for_each(&mas, vma, ULONG_MAX) {
        unsigned long vma_start = vma->vm_start;
        unsigned long vma_end = vma->vm_end;
        unsigned long vma_size = vma_end - vma_start;

        if (vma_size <= 0)
            continue;

        len = snprintf(log, 256, "VMA: 0x%lx - 0x%lx\n", vma_start, vma_end);
        kernel_write(file, log, len, &pos);
        pr_info("VMA: 0x%lx - 0x%lx\n", vma_start, vma_end);

        unsigned long vma_nr_pages = vma_size >> PAGE_SHIFT;
        unsigned long j;
        for (j = 0; j < vma_nr_pages; j++) {
            unsigned long current_vaddr = vma_start + (j << PAGE_SHIFT);

            pgd = pgd_offset(mm, current_vaddr);
            if (pgd_none(*pgd) || pgd_bad(*pgd))
                continue;

            p4d = p4d_offset(pgd, current_vaddr);
            if (p4d_none(*p4d) || p4d_bad(*p4d))
                continue;

            pud = pud_offset(p4d, current_vaddr);
            if (pud_none(*pud) || pud_bad(*pud))
                continue;

            pmd = pmd_offset(pud, current_vaddr);
            if (pmd_none(*pmd) || pmd_bad(*pmd))
                continue;

            pte = pte_offset_map(pmd, current_vaddr);
            if (!pte || pte_none(*pte)) {
                pte_unmap(pte);
                continue;
            }

            unsigned long pfn = pte_pfn(*pte);
            pgprot_t prot = pte_pgprot(*pte);

            len = snprintf(log, 256, "Vaddr: 0x%lx, PFN: 0x%lx, Prot: 0x%lx\n",
                          current_vaddr, pfn, pgprot_val(prot));
            kernel_write(file, log, len, &pos);
            pr_info("Vaddr: 0x%lx, PFN: 0x%lx, Prot: 0x%lx\n",
                   current_vaddr, pfn, pgprot_val(prot));

            pte_unmap(pte);
        }
	}
	rcu_read_unlock();


    kfree(log);
    filp_close(file, NULL);
    mmput(mm);
    return 0;
}


bool vma_is_pseudo_anon_shared(struct vm_area_struct *vma)
{
	return !!(vma->pseudo_mm_flag & PSEUDO_MM_VMA_ANON_SHARED);
}

inline struct pseudo_mm_backend *pseudo_mm_get_backend(void)
{
	return &backend;
}
