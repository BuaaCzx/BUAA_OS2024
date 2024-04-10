/*
与本实验相关的映射与寻址规则（内存布局）如下:
• 若虚拟地址处于 0x80000000~0x9fffffff (kseg0)，则将虚拟地址的最高位置 0 得到物理
地址，通过 cache 访存。这一部分用于存放内核代码与数据。
• 若虚拟地址处于 0xa0000000~0xbfffffff (kseg1)，则将虚拟地址的最高 3 位置 0 得到
物理地址，不通过 cache 访存。这一部分可以用于访问外设。
• 若虚拟地址处于 0x00000000~0x7fffffff (kuseg)，则需要通过 TLB 转换成物理地址，
再通过 cache 访存。这一部分用于存放用户程序代码与数据。
*/

#include <bitops.h>
#include <env.h>
#include <malta.h>
#include <mmu.h>
#include <pmap.h>
#include <printk.h>

/* These variables are set by mips_detect_memory(ram_low_size); */
static u_long memsize; /* Maximum physical address */
u_long npage;	       /* Amount of memory(in pages) */

Pde *cur_pgdir;

struct Page *pages;
/*
	struct Page {
		Page_LIST_entry_t pp_link;

		u_short pp_ref;
		//  pp_ref 对应这一页物理内存被引用的次数，它等于有多少虚拟页映射到该物理页
	};
	pages 是一个大小为 npage 的全局数组，page 是一个页控制块，每一个页控制块对应一页的物理内存，MOS 用这个结构体来 **按页管理物理内存的分配**。

	用一个数组存放这些 Page 结构体，首个 Page 的地址为 P，则 P[i] 对应从 0 开始计数的第 i 个物理
	页面。

	Page 与其对应的物理页面地址的转换可以使用 include/pmap.h 中的 page2pa 和 pa2page 这两个函数。

	pa2page(u_long pa) ：物理地址 --> 页控制块（读取 pte 后可进行转换）
	page2pa(struct Page *pp) ：页控制块 --> 物理地址（填充 pte 时常用）

*/

static u_long freemem;

struct Page_list page_free_list; /* Free list of physical pages */

/*
	将空闲物理页对应的 Page 结构体全部插入一个链表中，该链表被称为空闲链表，即 page_free_list。

	当一个进程需要分配内存时，就需要将空闲链表头部的页控制块对应的那一页物理内存分
	配出去，同时将该页控制块从空闲链表中删去。

	当一页物理内存被使用完毕（准确来说，引用次数为 0）时，将其对应的页控制块重新插入
	到空闲链表的头部。
*/

/* Overview:
 *   Use '_memsize' from bootloader to initialize 'memsize' and
 *   calculate the corresponding 'npage' value.
	探测硬件可用内存，并对一些和内存管理相关的变量进行初始化
 */
void mips_detect_memory(u_int _memsize) {
	/* Step 1: Initialize memsize. */
	memsize = _memsize; // 设置物理内存大小

	/* Step 2: Calculate the corresponding 'npage' value. */
	/* Exercise 2.1: Your code here. */
	npage = memsize / PAGE_SIZE; // 计算页数

	printk("Memory size: %lu KiB, number of pages: %lu\n", memsize / 1024, npage);
}

/* Lab 2 Key Code "alloc" */
/* Overview:
    Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, clear the
    allocated memory.
    This allocator is used only while setting up virtual memory system.
	用于在建立页式内存管理机制之前分配内存空间。
	为管理空闲物理页面的数据结构：页控制块数组 struct Page *pages 分配所用的内存空间
   Post-Condition:
    If we're out of memory, should panic, else return this address of memory we have allocated.

	分配 n 字节的空间并返回初始的虚拟地址，同时将地址按 align 字节对
	齐（保证 align 可以整除初始虚拟地址），若 clear 为真，则将对应内存空间的值清零，否则不
	清零。

	这个函数中对内存的操作位于 kseg0 段。里面表示地址的变量都是虚拟地址。
	位于 kseg0 段的地址转换宏：
	PADDR(kva) ：kseg0 处虚地址 --> 物理地址
	KADDR(pa) ：物理地址 --> kseg0 处虚地址（读取 pte 后可进行转换）
*/
void *alloc(u_int n, u_int align, int clear) {
	extern char end[];
	/*
		该变量对应 **虚拟地址** 0x80400000，在建立内存管理机制时，本实验都是通过
		kseg0 来访问内存。根据映射规则，0x80400000 对应的物理地址是 0x400000。
		接下来将从物理地址 0x400000 开始分配物理内存，用于建立管理内存的数据结构。
	*/
	u_long alloced_mem; // 即将分配出来的物理内存空间的首地址

	/* Initialize `freemem` if this is the first time. The first virtual address that the
	 * linker did *not* assign to any kernel code or global variables. */
	if (freemem == 0) {
		freemem = (u_long)end; // end
	}
	// 变量 freemem 表示：小于 freemem 对应物理地址的物理内存都已经被分配

	/* Step 1: Round up `freemem` up to be aligned properly */
	freemem = ROUND(freemem, align);
	/*
		找到 freemem 之上最小的、按
		align 对齐的初始虚拟地址，中间未用到的地址空间全部放弃。实际上是找到了一段空闲
		的、起始地址与 align 对齐的内存空间，并把它赋值给 freemem
	*/

	/* Step 2: Save current value of `freemem` as allocated chunk. */
	alloced_mem = freemem;
	// 当前的 freemem 要作为首地址被分配出去了

	/* Step 3: Increase `freemem` to record allocation. */
	freemem = freemem + n;
	// freemem 后面的 n 个空间被分配，更新 freemem

	// Panic if we're out of memory.
	panic_on(PADDR(freemem) >= memsize);
	// 检查分配后的内存空间有没有超出最大的内存上限。用 PADDR 把虚拟地址 freemem 转换成相应的物理地址

	/* Step 4: Clear allocated chunk if parameter `clear` is set. */
	if (clear) {
		memset((void *)alloced_mem, 0, n);
	}

	/* Step 5: return allocated chunk. */
	return (void *)alloced_mem;
	// 返回分配出来的空间的虚拟地址的首地址
}
/* End of Key Code "alloc" */

/* Overview:
    Set up two-level page table.
   Hint:
    You can get more details about `UPAGES` and `UENVS` in include/mmu.h. */
void mips_vm_init() {
	/* Allocate proper size of physical memory for global array `pages`,
	 * for physical memory management. Then, map virtual address `UPAGES` to
	 * physical address `pages` allocated before. For consideration of alignment,
	 * you should round up the memory size before map. 
	 为全局数组`pages`分配适当大小的物理内存，用于物理内存管理。然后，将虚拟地址`UPAGES`映射到之前分配的物理地址`pages`。
	 在映射之前，考虑到对齐问题，你应该将内存大小上调至下一个对齐边界。
	 */
	pages = (struct Page *)alloc(npage * sizeof(struct Page), PAGE_SIZE, 1);
	printk("to memory %x for struct Pages.\n", freemem);
	printk("pmap.c:\t mips vm init success\n");
}

/* Overview:
 *   Initialize page structure and memory free list. The 'pages' array has one 'struct Page' entry
 * per physical page. Pages are reference counted, and free pages are kept on a linked list.
 *
 * Hint: Use 'LIST_INSERT_HEAD' to insert free pages to 'page_free_list'.
 */
void page_init(void) {
	/* Step 1: Initialize page_free_list. */
	/* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
	/* Exercise 2.3: Your code here. (1/4) */
	LIST_INIT(&page_free_list); 
	/* Step 2: Align `freemem` up to multiple of PAGE_SIZE. */
	/* Exercise 2.3: Your code here. (2/4) */
	freemem = ROUND(freemem, PAGE_SIZE);
	/* Step 3: Mark all memory below `freemem` as used (set `pp_ref` to 1) */
	/* Exercise 2.3: Your code here. (3/4) */
	u_long page_used = PPN(PADDR(freemem)); // 求出当前已经被用过的物理内存的页数
	// PPN宏用于提取物理地址 pa 中的页号。
	for (int i = 0; i < page_used; i++) { // 把已经被用过的物理内存的 pp_ref(被引用次数) 设置为 1
		pages[i].pp_ref = 1;
	}
	/* Step 4: Mark the other memory as free. */
	/* Exercise 2.3: Your code here. (4/4) */
	for (int i = page_used; i < npage; i++) { // 其它的物理内存页都被设置为未被访问过，并且加入到 page_free_list 中
		pages[i].pp_ref = 0;
		LIST_INSERT_HEAD(&page_free_list, pages + i, pp_link);
	}
}

/* Overview:
 *   Allocate a physical page from free memory, and fill this page with zero.
 *
 * Post-Condition:
 *   If failed to allocate a new page (out of memory, there's no free page), return -E_NO_MEM.
 *   Otherwise, set the address of the allocated 'Page' to *pp, and return 0.
 *
 * Note:
 *   This does NOT increase the reference count 'pp_ref' of the page - the caller must do these if
 *   necessary (either explicitly or via page_insert).
 *
 * Hint: Use LIST_FIRST and LIST_REMOVE defined in include/queue.h.

	将 page_free_list 空闲链表头部页控制
	块对应的物理页面分配出去，将其从空闲链表中移除，并清空此页中的数据，最后将 new
	指向的空间赋值为这个页控制块的地址。
 */
int page_alloc(struct Page **new) {
	/* Step 1: Get a page from free memory. If fails, return the error code.*/
	struct Page *pp;
	/* Exercise 2.4: Your code here. (1/2) */
	if (LIST_EMPTY(&page_free_list)) {
		return -E_NO_MEM;
	}
	pp = LIST_FIRST(&page_free_list); 
	LIST_REMOVE(pp, pp_link);

	/* Step 2: Initialize this page with zero.
	 * Hint: use `memset`. */
	/* Exercise 2.4: Your code here. (2/2) */
	memset(page2kva(pp), 0, PAGE_SIZE);
	// page2kva 函数位于 pmap.h 中，可以完成从页控制块地址到对应的物理地址的虚拟地址的转换。
	*new = pp;
	return 0;
}

/* Overview:
 *   Release a page 'pp', mark it as free.
 *
 * Pre-Condition:
 *   'pp->pp_ref' is '0'.

	它的作用是将 pp 指向的页控制块重新插入到 page_free_list
	中。此外需要先确保 pp 指向的页控制块对应的物理页面引用次数为 0。
 */
void page_free(struct Page *pp) {
	assert(pp->pp_ref == 0);
	/* Just insert it into 'page_free_list'. */
	/* Exercise 2.5: Your code here. */
	LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
}

/* Overview:
 *   Given 'pgdir', a pointer to a page directory, 'pgdir_walk' returns a pointer to
 *   the page table entry for virtual address 'va'.
 *
 * Pre-Condition:
 *   'pgdir' is a two-level page table structure.
 *   'ppte' is a valid pointer, i.e., it should NOT be NULL.
 *
 * Post-Condition:
 *   If we're out of memory, return -E_NO_MEM.
 *   Otherwise, we get the page table entry, store
 *   the value of page table entry to *ppte, and return 0, indicating success.
 *
 * Hint:
 *   We use a two-level pointer to store page table entry and return a state code to indicate
 *   whether this function succeeds or not.

	该函数将 **一级页表基地址 pgdir** 对应的两级页表结构中 va 虚拟地址所在的二级页表项的指针存储在 ppte 指
	向的空间上。如果 create 不为 0 且对应的二级页表不存在，则会使用 page_alloc 函数分配一
	页物理内存用于存放二级页表，如果分配失败则返回错误码。

	简单来说就是把虚拟地址 va 所在的二级页表项的指针存在指针 ppte 对应的空间上
 */
static int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) {
// Pde 是一级页表项类型，Pte是二级页表项类型
	Pde *pgdir_entryp;
	struct Page *pp;

	/* Step 1: Get the corresponding page directory entry. */
	/* Exercise 2.6: Your code here. (1/3) */
	pgdir_entryp = pgdir + PDX(va);
	// 找到对应的一级页表项


	/* Step 2: If the corresponding page table is not existent (valid) then:
	 *   * If parameter `create` is set, create one. Set the permission bits 'PTE_C_CACHEABLE |
	 *     PTE_V' for this new page in the page directory. If failed to allocate a new page (out
	 *     of memory), return the error.
	 *   * Otherwise, assign NULL to '*ppte' and return 0.
	 */
	/* Exercise 2.6: Your code here. (2/3) */
	if (!(*pgdir_entryp & PTE_V)) { // 一级页表项有效的条件：这个页表项有东西（非零），且页表的有效位 PTE_V 为 1.
		if (create) {
			int error = page_alloc(&pp); // 尝试分配一个新的页作为二级页表
			if (error) {
				return error;
			} else {
				pp->pp_ref++;
				*pgdir_entryp = page2pa(pp) | PTE_C_CACHEABLE | PTE_V;
				// 页表项里存的是物理页号
				// 用 page2pa 获取页控制块对应的物理地址
			}
		} else {
			*ppte = NULL;
		}
	}

	/* Step 3: Assign the kernel virtual address of the page table entry to '*ppte'. */
	/* Exercise 2.6: Your code here. (3/3) */
	Pte *pte_entry = (Pte*) (KADDR(PTE_ADDR(*pgdir_entryp)));
	// 获取二级页表基地址：先用 PTE_ADDR 获取页表项中的物理地址，然后再将这个物理地址转换为虚拟地址。进而得到二级页表基地址的虚拟地址
	*ppte = pte_entry + PTX(va);

	return 0;
}

/* Overview:
 *   Map the physical page 'pp' at virtual address 'va'. The permission (the low 12 bits) of the
 *   page table entry should be set to 'perm | PTE_C_CACHEABLE | PTE_V'.
 *
 * Post-Condition:
 *   Return 0 on success
 *   Return -E_NO_MEM, if page table couldn't be allocated
 *
 * Hint:
 *   If there is already a page mapped at `va`, call page_remove() to release this mapping.
 *   The `pp_ref` should be incremented if the insertion succeeds.

	将一级页表基地址 pgdir 对应的两级页表结构中虚拟地址 va 映射到页控制块 pp 对应的
	物理页面，并将页表项权限为设置为 perm
 */
int page_insert(Pde *pgdir, u_int asid, struct Page *pp, u_long va, u_int perm) {
	Pte *pte;

	/* Step 1: Get corresponding page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);
	// 先找一下对应的二级页表

	if (pte && (*pte & PTE_V)) { // 如果有对应的二级页表
		if (pa2page(*pte) != pp) { // 如果二级页表项对应的控制页块不是pp，把它删掉
			/*
				pa2page不能简单的理解为物理页号-->页控制块！！
				它实际上是用于 **二级页表** 项到控制页块地址的转换。
			*/
			page_remove(pgdir, asid, va); // 解除 va 当前的映射
		} else {
			tlb_invalidate(asid, va);
			/*
				使用 tlb_invalidate 函数可以实现删除特定虚拟地址的映射，每当页表被修改，就需要调
				用该函数以保证下次访问相应虚拟地址时一定触发 TLB 重填，进而保证访存的正确性。
			*/
			*pte = page2pa(pp) | perm | PTE_C_CACHEABLE | PTE_V; // 直接修改二级页表项
			return 0;
		}
	}

	/* Step 2: Flush TLB with 'tlb_invalidate'. */
	/* Exercise 2.7: Your code here. (1/3) */
	tlb_invalidate(asid, va);
	/* Step 3: Re-get or create the page table entry. */
	/* If failed to create, return the error. */
	/* Exercise 2.7: Your code here. (2/3) */
	if (pgdir_walk(pgdir, va, 1, &pte)) { // 找一下对应的二级页表，如果没有的话就新建一个二级页表
		return -E_NO_MEM;
	}
	/* Step 4: Insert the page to the page table entry with 'perm | PTE_C_CACHEABLE | PTE_V'
	 * and increase its 'pp_ref'. */
	/* Exercise 2.7: Your code here. (3/3) */
	*pte = page2pa(pp) | perm | PTE_C_CACHEABLE | PTE_V; // 设置二级页表项
	pp->pp_ref++; // 更新 pp_ref
	return 0;
}

/* Lab 2 Key Code "page_lookup" */
/*Overview:
    Look up the Page that virtual address `va` map to.
  Post-Condition:
    Return a pointer to corresponding Page, and store it's page table entry to *ppte.
    If `va` doesn't mapped to any Page, return NULL.
	返回一级页表基地址 pgdir 对应的两级页表结构中虚拟地址 va 映射的物理页面的页控制块，同时将
	ppte 指向的空间设为对应的二级页表项地址。

	这玩意可以看作是一个加强版的 page_walk，它不仅可以获取二级页表项的地址，还可以返回相应页控制块的地址
*/
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
	struct Page *pp;
	Pte *pte;

	/* Step 1: Get the page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);
	// 先找一下二级页表项

	/* Hint: Check if the page table entry doesn't exist or is not valid. */
	if (pte == NULL || (*pte & PTE_V) == 0) {
		return NULL;
	}

	/* Step 2: Get the corresponding Page struct. */
	/* Hint: Use function `pa2page`, defined in include/pmap.h . */
	pp = pa2page(*pte); // 找一下对应的页控制块
	if (ppte) {
		*ppte = pte;
	}

	return pp;
}
/* End of Key Code "page_lookup" */

/* Overview:
 *   Decrease the 'pp_ref' value of Page 'pp'.
 *   When there's no references (mapped virtual address) to this page, release it.
	令 pp 对应页控制块的引用次数减少 1，如果引
	用次数为 0 则会调用 page_free 函数将对应物理页面重新设置为空闲页面。
 */
void page_decref(struct Page *pp) {
	assert(pp->pp_ref > 0);

	/* If 'pp_ref' reaches to 0, free this page. */
	if (--pp->pp_ref == 0) {
		page_free(pp);
	}
}

/* Lab 2 Key Code "page_remove" */
// Overview:
//   Unmap the physical page at virtual address 'va'.
/*
	作用是删除一级页表基地址
	pgdir 对应的两级页表结构中虚拟地址 va 对物理地址的映射。如果存在这样的映射，那么对应
	物理页面的引用次数会减少一次。
*/
void page_remove(Pde *pgdir, u_int asid, u_long va) {
	Pte *pte;

	/* Step 1: Get the page table entry, and check if the page table entry is valid. */
	struct Page *pp = page_lookup(pgdir, va, &pte); // 找一下页控制项，如果本来就没有对应的，就不管，直接return
	if (pp == NULL) {
		return;
	}

	/* Step 2: Decrease reference count on 'pp'. */
	page_decref(pp);

	/* Step 3: Flush TLB. */
	*pte = 0; // 二级页表项置为0
	tlb_invalidate(asid, va);
	return;
}
/* End of Key Code "page_remove" */

u_int page_filter(Pde *pgdir, u_int va_lower_limit, u_int va_upper_limit, u_int num) {
	u_int res = 0;
	for (u_int i = va_lower_limit; i < va_upper_limit; i += 4096) {
		if ((Pte*)i == NULL || (*(Pte*)i & PTE_V) == 0) {
			continue;
		}
	
		/* Step 2: Get the corresponding Page struct. */
		/* Hint: Use function `pa2page`, defined in include/pmap.h . */
		struct Page *pp = pa2page(*(*Pte)i); // 找一下对应的页控制块
		if (pp->pp_ref >= num) {
			res++;
		}
	}
	return res;
}

void physical_memory_manage_check(void) {
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;
	int *temp;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);
	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	temp = (int *)page2kva(pp0);
	// write 1000 to pp0
	*temp = 1000;
	// free pp0
	page_free(pp0);
	printk("The number in address temp is %d\n", *temp);

	// alloc again
	assert(page_alloc(&pp0) == 0);
	assert(pp0);

	// pp0 should not change
	assert(temp == (int *)page2kva(pp0));
	// pp0 should be zero
	assert(*temp == 0);

	page_free_list = fl;
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	struct Page_list test_free;
	struct Page *test_pages;
	test_pages = (struct Page *)alloc(10 * sizeof(struct Page), PAGE_SIZE, 1);
	LIST_INIT(&test_free);
	// LIST_FIRST(&test_free) = &test_pages[0];
	int i, j = 0;
	struct Page *p, *q;
	for (i = 9; i >= 0; i--) {
		test_pages[i].pp_ref = i;
		// test_pages[i].pp_link=NULL;
		// printk("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
		LIST_INSERT_HEAD(&test_free, &test_pages[i], pp_link);
		// printk("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
	}
	p = LIST_FIRST(&test_free);
	int answer1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	assert(p != NULL);
	while (p != NULL) {
		// printk("%d %d\n",p->pp_ref,answer1[j]);
		assert(p->pp_ref == answer1[j++]);
		// printk("ptr: 0x%x v: %d\n",(p->pp_link).le_next,((p->pp_link).le_next)->pp_ref);
		p = LIST_NEXT(p, pp_link);
	}
	// insert_after test
	int answer2[] = {0, 1, 2, 3, 4, 20, 5, 6, 7, 8, 9};
	q = (struct Page *)alloc(sizeof(struct Page), PAGE_SIZE, 1);
	q->pp_ref = 20;

	// printk("---%d\n",test_pages[4].pp_ref);
	LIST_INSERT_AFTER(&test_pages[4], q, pp_link);
	// printk("---%d\n",LIST_NEXT(&test_pages[4],pp_link)->pp_ref);
	p = LIST_FIRST(&test_free);
	j = 0;
	// printk("into test\n");
	while (p != NULL) {
		//      printk("%d %d\n",p->pp_ref,answer2[j]);
		assert(p->pp_ref == answer2[j++]);
		p = LIST_NEXT(p, pp_link);
	}

	printk("physical_memory_manage_check() succeeded\n");
}

void page_check(void) {
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate a page for directory
	assert(page_alloc(&pp) == 0);
	Pde *boot_pgdir = (Pde *)page2kva(pp);

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// there is no free memory, so we can't allocate a page table
	assert(page_insert(boot_pgdir, 0, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(boot_pgdir, 0, pp1, 0x0, 0) == 0);
	assert(PTE_FLAGS(boot_pgdir[0]) == (PTE_C_CACHEABLE | PTE_V));
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	assert(PTE_FLAGS(*(Pte *)page2kva(pp0)) == (PTE_C_CACHEABLE | PTE_V));

	printk("va2pa(boot_pgdir, 0x0) is %x\n", va2pa(boot_pgdir, 0x0));
	printk("page2pa(pp1) is %x\n", page2pa(pp1));
	//  printk("pp1->pp_ref is %d\n",pp1->pp_ref);
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);

	// should be able to map pp2 at PAGE_SIZE because pp0 is already allocated for page table
	assert(page_insert(boot_pgdir, 0, pp2, PAGE_SIZE, 0) == 0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	printk("start page_insert\n");
	// should be able to map pp2 at PAGE_SIZE because it's already there
	assert(page_insert(boot_pgdir, 0, pp2, PAGE_SIZE, 0) == 0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_pgdir, 0, pp0, PDMAP, 0) < 0);

	// insert pp1 at PAGE_SIZE (replacing pp2)
	assert(page_insert(boot_pgdir, 0, pp1, PAGE_SIZE, 0) == 0);

	// should have pp1 at both 0 and PAGE_SIZE, pp2 nowhere, ...
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	printk("pp2->pp_ref %d\n", pp2->pp_ref);
	assert(pp2->pp_ref == 0);
	printk("end page_insert\n");

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at PAGE_SIZE
	page_remove(boot_pgdir, 0, 0x0);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at PAGE_SIZE should free it
	page_remove(boot_pgdir, 0, PAGE_SIZE);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	boot_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	page_free(pa2page(PADDR(boot_pgdir)));

	printk("page_check() succeeded!\n");
}
