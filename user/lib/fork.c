#include <env.h>
#include <lib.h>
#include <mmu.h>

/* Overview:
 *   Map the faulting page to a private writable copy.
 *
 * Pre-Condition:
 * 	'va' is the address which led to the TLB Mod exception.
 *
 * Post-Condition:
 *  - Launch a 'user_panic' if 'va' is not a copy-on-write page.
 *  - Otherwise, this handler should map a private writable copy of
 *    the faulting page at the same address.
 */
static void __attribute__((noreturn)) cow_entry(struct Trapframe *tf) {
	u_int va = tf->cp0_badvaddr;
	u_int perm;

	/* Step 1: Find the 'perm' in which the faulting address 'va' is mapped. */
	/* Hint: Use 'vpt' and 'VPN' to find the page table entry. If the 'perm' doesn't have
	 * 'PTE_COW', launch a 'user_panic'. */
	/* Exercise 4.13: Your code here. (1/6) */
	perm = PTE_FLAGS(vpt[VPN(va)]);
	if (!(perm & PTE_COW)) {
		user_panic("the perm doesn't have PTE_COW\n");
	}

	/* Step 2: Remove 'PTE_COW' from the 'perm', and add 'PTE_D' to it. */
	/* Exercise 4.13: Your code here. (2/6) */
	perm &= ~PTE_COW;
	perm |= PTE_D;

	/* Step 3: Allocate a new page at 'UCOW'. */
	/* Exercise 4.13: Your code here. (3/6) */
	syscall_mem_alloc(0 , UCOW, perm);

	/* Step 4: Copy the content of the faulting page at 'va' to 'UCOW'. */
	/* Hint: 'va' may not be aligned to a page! */
	/* Exercise 4.13: Your code here. (4/6) */
	memcpy(UCOW, ROUNDDOWN(va, PAGE_SIZE), PAGE_SIZE);

	// Step 5: Map the page at 'UCOW' to 'va' with the new 'perm'.
	/* Exercise 4.13: Your code here. (5/6) */
	syscall_mem_map(0, UCOW, 0, va, perm);

	// Step 6: Unmap the page at 'UCOW'.
	/* Exercise 4.13: Your code here. (6/6) */
	syscall_mem_unmap(0, va);

	// Step 7: Return to the faulting routine.
	int r = syscall_set_trapframe(0, tf);
	user_panic("syscall_set_trapframe returned %d", r);
}

/* Overview:
 *   Grant our child 'envid' access to the virtual page 'vpn' (with address 'vpn' * 'PAGE_SIZE') in
 * our (current env's) address space. 'PTE_COW' should be used to isolate the modifications on
 * unshared memory from a parent and its children.
 *
 * Post-Condition:
 *   If the virtual page 'vpn' has 'PTE_D' and doesn't has 'PTE_LIBRARY', both our original virtual
 *   page and 'envid''s newly-mapped virtual page should be marked 'PTE_COW' and without 'PTE_D',
 *   while the other permission bits are kept.
 *
 *   If not, the newly-mapped virtual page in 'envid' should have the exact same permission as our
 *   original virtual page.
 *
 * Hint:
 *   - 'PTE_LIBRARY' indicates that the page should be shared among a parent and its children.
 *   - A page with 'PTE_LIBRARY' may have 'PTE_D' at the same time, you should handle it correctly.
 *   - You can pass '0' as an 'envid' in arguments of 'syscall_*' to indicate current env because
 *     kernel 'envid2env' converts '0' to 'curenv').
 *   - You should use 'syscall_mem_map', the user space wrapper around 'msyscall' to invoke
 *     'sys_mem_map' in kernel.
 */
static void duppage(u_int envid, u_int vpn) {
	int r;
	u_int addr;
	u_int perm;

	/* Step 1: Get the permission of the page. */
	/* Hint: Use 'vpt' to find the page table entry. */
	/*
	在MOS（可能是某种类似于MIT的操作系统课程中的教学操作系统）的OS内核中，`vpt` 和 `vpd` 这两个宏定义用于访问页表（Page Table）和页目录（Page Directory）。
	这里的 `UVPT` 是一个常量，它代表了用户地址空间中的虚拟地址，该地址映射到内核页表。在许多操作系统中，内核会映射一部分自己的页表到用户地址空间，以便用户进程能够访问它，通常是用于实现系统调用或者访问某些内核数据结构。
	`Pte` 和 `Pde` 分别代表了页表项（Page Table Entry）和页目录项（Page Directory Entry）的结构。这些项包含了关于虚拟地址到物理地址映射的信息，例如物理页号、访问权限等。
	`PDX` 宏是一个用于从虚拟地址中提取页目录索引的宏，`PGSHIFT` 是一个常量，表示页大小的位移（在x86架构中通常是12，因为页大小是2^12即4KB）。
	因此，这两个宏定义的作用是：
	- `vpt` 宏定义了一个指向页表的常量、易变指针。这个指针可以用来访问和修改用户进程的页表项。
	- `vpd` 宏定义了一个指向页目录的常量、易变指针。由于页目录紧跟在页表之后，所以这个指针是通过 `UVPT` 加上页目录的偏移量来计算的。这个偏移量是通过将 `UVPT` 的页目录索引（通过 `PDX` 宏获得）左移 `PGSHIFT` 位来得到的，这样就可以得到页目录的起始地址。
	通过这两个宏，内核代码可以方便地访问和操作页表和页目录，以实现虚拟内存管理功能，例如地址翻译、权限检查、页表项的创建和删除等。

	*/
	/* Exercise 4.10: Your code here. (1/2) */
	addr = vpn * PAGE_SIZE;
	perm = PTE_FLAGS(vpt[vpn]);

	/* Step 2: If the page is writable, and not shared with children, and not marked as COW yet,
	 * then map it as copy-on-write, both in the parent (0) and the child (envid). */
	/* Hint: The page should be first mapped to the child before remapped in the parent. (Why?)
	 */
	/* Exercise 4.10: Your code here. (2/2) */
	if ((perm & PTE_D) && !(perm & PTE_LIBRARY) && !(perm & PTE_COW)) {
		perm &= ~PTE_D;
		perm |= PTE_COW;
		syscall_mem_map(0, addr, envid, addr, perm);
		syscall_mem_map(0, addr, 0, addr, perm);
	}

}

/* Overview:
 *   User-level 'fork'. Create a child and then copy our address space.
 *   Set up ours and its TLB Mod user exception entry to 'cow_entry'.
 *
 * Post-Conditon:
 *   Child's 'env' is properly set.
 *
 * Hint:
 *   Use global symbols 'env', 'vpt' and 'vpd'.
 *   Use 'syscall_set_tlb_mod_entry', 'syscall_getenvid', 'syscall_exofork',  and 'duppage'.
 */
int fork(void) {
	u_int child;
	u_int i;

	/* Step 1: Set our TLB Mod user exception entry to 'cow_entry' if not done yet. */
	if (env->env_user_tlb_mod_entry != (u_int)cow_entry) {
		try(syscall_set_tlb_mod_entry(0, cow_entry));
	}

	/* Step 2: Create a child env that's not ready to be scheduled. */
	// Hint: 'env' should always point to the current env itself, so we should fix it to the
	// correct value.
	child = syscall_exofork();
	if (child == 0) {
		env = envs + ENVX(syscall_getenvid());
		return 0;
	}

	debugf("### %d\n", child);

	/* Step 3: Map all mapped pages below 'USTACKTOP' into the child's address space. */
	// Hint: You should use 'duppage'.
	/* Exercise 4.15: Your code here. (1/2) */
	for (int i = 0; i < VPN(USTACKTOP); i++) {
		if ((vpd[i >> 10] & PTE_V) && (vpt[i] & PTE_V)) {
			duppage(child, i);
		}
	}

	debugf("### %d\n", child);

	/* Step 4: Set up the child's tlb mod handler and set child's 'env_status' to
	 * 'ENV_RUNNABLE'. */
	/* Hint:
	 *   You may use 'syscall_set_tlb_mod_entry' and 'syscall_set_env_status'
	 *   Child's TLB Mod user exception entry should handle COW, so set it to 'cow_entry'
	 */
	/* Exercise 4.15: Your code here. (2/2) */
	try(syscall_set_tlb_mod_entry(child, cow_entry));
	try(syscall_set_env_status(child, ENV_RUNNABLE));

	
	debugf("### %d\n", child);

	return child;
}
