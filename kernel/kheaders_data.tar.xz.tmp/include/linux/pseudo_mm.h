#ifndef __LINUX_PSEUDO_MM__
#define __LINUX_PSEUDO_MM__

#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/xarray.h>
#include <linux/rmap.h>

#define PSEUDO_MM_DEBUG

typedef struct {
	unsigned long val;
} pseudo_mm_rdma_entry_t;

struct pseudo_mm {
	struct mm_struct *mm;
	int id;
	/* list of pseudo_mm_pin_pages */
	struct list_head pages_list;
};

struct pseudo_mm_pin_pages {
	struct list_head list;
	long nr_pin_pages;
	struct page **pages;
};

struct pseudo_mm_unmap_args {
	enum ttu_flags flags;
	struct vm_area_struct *curr;
	pte_t orig_pte;
	struct folio *old_folio;
};

enum pseudo_mm_pt_type;

struct pseudo_mm_backend {
	struct file *filp;
};

// read single page from remote
// @page: the local page, which will be filled with remote memory content
// @rpgoff: the remote page offset
//
// NOTE: that the page should be locked before call this method
// after return with 0 and get the page lock AGAIN to guarantee
// that page content is loaded.
typedef int (pseudo_mm_rdma_pf_ops_t)(struct page *page, pgoff_t rpgoff);

/* return 0 if succeed */
unsigned long register_backend_dax_device(int fd);
unsigned long register_backend_memory(int node, int order);
inline struct pseudo_mm_backend *pseudo_mm_get_backend(void);

unsigned long pseudo_mm_phy_addr();

/*
 * Register page fault handler for rdma-backed pages in pseudo_mm
 * @ op: the rdma read function, which read from remote `offset` into `page`.
 * @ node: the node to alloc pages for rdma-backed pseudo_mm.
 *
 * return 0 if succeed
 */
unsigned long register_pseudo_mm_rdma_pf_handler(pseudo_mm_rdma_pf_ops_t *op, int node);
bool pseudo_mm_rdma_pf_handler_enable(void);
pseudo_mm_rdma_pf_ops_t pseudo_mm_rdma_pf_handle;
int pseudo_mm_rdma_prefer_node(void);

/*
 * create_pseudo_mm() - create and init the pseudo_mm
 *
 * return the id of that pseudo_mm, which can be used to find_pseudo_mm()
 */
int create_pseudo_mm(void);
struct pseudo_mm *find_pseudo_mm(int id);

/*
 * put_pseudo_mm_with_id() - delete the pseudo_mm corresponding to id
 * @id: the id of the pseudo_mm that needed to be deleted, -1 to delete
 * all pseudo_mm
 */
void put_pseudo_mm_with_id(int id);
/*
 * Add a memory mapping to this pseudo_mm.
 * This will not fill content of the physical page.
 * Support file-backed mapping and anonymous private mapping.
 * (Do not support anonymous shared mapping for now.)
 *
 * The meaning of params is the same as mmap()
 */
unsigned long pseudo_mm_add_map(int id, unsigned long start, unsigned long size,
				unsigned long prot, unsigned long flags, int fd,
				pgoff_t pgoff);

/*
 * pseudo_mm_setup_pt() - setup page table of pseudo_mm's virtual address
 * @id: pseudo_mm id
 * @start: start address
 * @size: size of continuous virtual address
 * @pgoff: physical page offset of backend dax device (page number) or remote rdma page offset
 * @type: the page table entry type, currently only support RDMA and DAX
 *
 * This function will establish the page table of virtual address range
 * [start, start + size) and let it point to physical page start at pgoff
 * in backend dax device (the page table is read-only).
 *
 * *Note*: The [start, start + size) should not exceed the boundary of single
 * vma. Return 0 when succeed.
 */
unsigned long pseudo_mm_setup_pt(int id, unsigned long start,
				 unsigned long size, pgoff_t pgoff,
				 enum pseudo_mm_pt_type type);

/*
 * pseudo_mm_bring_back() - bring the virtual memory in pseudo_mm back to local memory.
 * @id: pseudo_mm id
 * @start: start address
 * @size: size of continuous virtual address
 *
 * This function will bring the memory of virtual address range
 * [start, start + size) back to local memory. 
 *
 * *Note*: The [start, start + size) should not exceed the boundary of single
 * vma. Return 0 when succeed.
 */
unsigned long pseudo_mm_bring_back(int id, unsigned long start,
				   unsigned long size);

/*
 * pseudo_mm_attach() - insert *all* memory mapping into an existing process's address space
 * @pid: process id
 * @id: pseudo_mm_id
 */
unsigned long pseudo_mm_attach(pid_t pid, int id);
unsigned long pseudo_mm_getpte(pid_t pid);
unsigned long pseudo_mm_getpte_from_mm(struct mm_struct *mm);
unsigned long pseudo_mm_getpte_from_oldmm(struct mm_struct *mm);
void print_file_path(struct file *file);
/* debug purpose */
void debug_weird_page(struct page *page, int expected_mapcount);

static inline bool vma_is_pseudo_mm_master(struct vm_area_struct *vma)
{
	return (vma->pseudo_mm_flag & (PSEUDO_MM_VMA_MASTER | PSEUDO_MM_VMA)) ==
	       (PSEUDO_MM_VMA_MASTER | PSEUDO_MM_VMA);
}

static inline bool vma_is_pseudo_mm(struct vm_area_struct *vma)
{
	return !!(vma->pseudo_mm_flag & PSEUDO_MM_VMA);
}

/*
 * Following macro only valid in x86 arch
 * NOTE that BIT 4 (PCD) is unused by swap, so we can use it to indicate
 * that this page should be read by pseudo_mm rdma.
 */
/* We always extract/encode the offset by shifting it all the way up, and then down again */
#define PSEUDO_MM_RDMA_RDMA_OFFSET_SHIFT (_PAGE_BIT_PROTNONE + 1)

/* Shift up (to get rid of type), then down to get value */
#define pseudo_mm_rdma_offset(x) (~(x).val >> PSEUDO_MM_RDMA_RDMA_OFFSET_SHIFT)

/*
 * Shift the offset up "too far" by TYPE bits, then down again
 * The offset is inverted by a binary not operation to make the high
 * physical bits set.
 */
#define pseudo_mm_rdma_entry(offset)                         \
	((pseudo_mm_rdma_entry_t){ (~(unsigned long)(offset) \
				    << PSEUDO_MM_RDMA_RDMA_OFFSET_SHIFT) })

#define pte_to_pseudo_mm_rdma_entry(pte) \
	((pseudo_mm_rdma_entry_t){ pte_val((pte)) & (~_PAGE_PCD) })
#define pseudo_mm_rdma_entry_to_pte(x) ((pte_t){ .pte = (x).val | _PAGE_PCD })

static inline bool is_pseudo_mm_rdma_fault(struct vm_fault *vmf)
{
	return (!pte_present(vmf->orig_pte)) &&
	       (pte_flags(vmf->orig_pte) & _PAGE_PCD) &&
	       vma_is_pseudo_mm(vmf->vma);
}

#endif
