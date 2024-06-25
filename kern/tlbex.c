#include <bitops.h>
#include <env.h>
#include <pmap.h>

/* Lab 2 Key Code "tlb_invalidate" */
/* Overview:
 *   Invalidate the TLB entry with specified 'asid' and virtual address 'va'.
 *
 * Hint:
 *   Construct a new Entry HI and call 'tlb_out' to flush TLB.
 *   'tlb_out' is defined in mm/tlb_asm.S
 */
void tlb_invalidate(u_int asid, u_long va) {
	tlb_out((va & ~GENMASK(PGSHIFT, 0)) | (asid & (NASID - 1)));
}
/* End of Key Code "tlb_invalidate" */

static void passive_alloc(u_int va, Pde *pgdir, u_int asid) {
	struct Page *p = NULL;

	if (va < UTEMP) {
		// panic("address too low");
		// printk("address too low, send SIGSEGV\n");
		sys_kill(0, SIGSEGV);
	}

	if (va >= USTACKTOP && va < USTACKTOP + PAGE_SIZE) {
		panic("invalid memory");
	}

	if (va >= UENVS && va < UPAGES) {
		panic("envs zone");
	}

	if (va >= UPAGES && va < UVPT) {
		panic("pages zone");
	}

	if (va >= ULIM) {
		panic("kernel address");
	}

	panic_on(page_alloc(&p));
	panic_on(page_insert(pgdir, asid, p, PTE_ADDR(va), (va >= UVPT && va < ULIM) ? 0 : PTE_D));
}

/* Overview:
 *  Refill TLB.
 */

/*
263 Overview:
264     Look up the Page that virtual address `va` map to.
265   Post-Condition:
266     Return a pointer to corresponding Page, and store it's page table entry to *ppte.
267     If `va` doesn't mapped to any Page, return NULL.
268 struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte)
*/
void _do_tlb_refill(u_long *pentrylo, u_int va, u_int asid) {
	tlb_invalidate(asid, va);
	Pte *ppte;
	/* Hints:
	 *  Invoke 'page_lookup' repeatedly in a loop to find the page table entry '*ppte'
	 * associated with the virtual address 'va' in the current address space 'cur_pgdir'.
	 *
	 *  **While** 'page_lookup' returns 'NULL', indicating that the '*ppte' could not be found,
	 *  allocate a new page using 'passive_alloc' until 'page_lookup' succeeds.
	 
	cur_pgdir 是一个在 kern/pmap.c 定义的全局变量，其中存储了当前进程一级页表基地址
	位于 kseg0 的虚拟地址。
	*/
	/* Exercise 2.9: Your code here. */
	while (page_lookup(cur_pgdir, va, &ppte) == NULL) {
		passive_alloc(va, cur_pgdir, asid);
	}



	ppte = (Pte *)((u_long)ppte & ~0x7);
	pentrylo[0] = ppte[0] >> 6;
	pentrylo[1] = ppte[1] >> 6;
}

void do_signal(struct Trapframe *tf) {

	if (curenv->env_cur_sig == SIGKILL) {
		return;
	}

	struct sigset_t *sig_todo = NULL; // 将要处理的信号

	struct sigset_t *ss;
	TAILQ_FOREACH(ss, &curenv->env_sig_list, sig_link) {
		if(ss->sig == SIGKILL) { // 如果有 SIGKILL，优先处理
			sig_todo = ss;
			break;
		}
		u_int cur_mask = curenv->env_mask_list[curenv->env_mask_cnt].sig;
		if(!((cur_mask >> (ss->sig)) & 1)) { // 如果信号未被屏蔽
			if(sig_todo == NULL) { // 如果当前还没有需要处理的信号
				sig_todo = ss;
			} else { // 已经有了，对比优先级
				if(ss->sig > sig_todo->sig){
					sig_todo = ss;
				}
			}
		}
	}

	if (!sig_todo) {
		return;
	} else {
		// printk("catch signal %d\n", sig_todo->sig);
	}

	TAILQ_REMOVE(&curenv->env_sig_list, sig_todo, sig_link);

	// 把新掩码 push 进掩码栈, 上一个掩码，该信号掩码及该信号本身
	u_int mask = curenv->env_mask_list[curenv->env_mask_cnt].sig | curenv->env_sigactions[sig_todo->sig].sa_mask.sig | (1 << (sig_todo->sig - 1));
	curenv->env_mask_cnt++;
	curenv->env_mask_list[curenv->env_mask_cnt].sig = mask;

	curenv->env_cur_sig = sig_todo->sig;

	struct Trapframe tmp_tf = *tf;

	if (tf->regs[29] < USTACKTOP || tf->regs[29] >= UXSTACKTOP) {
		tf->regs[29] = UXSTACKTOP;
	}
	tf->regs[29] -= sizeof(struct Trapframe);
	*(struct Trapframe *)tf->regs[29] = tmp_tf;
	
	tf->regs[4] = tf->regs[29];
	tf->regs[5] = (unsigned int) (curenv->env_sigactions[sig_todo->sig].sa_handler);
	tf->regs[6] = sig_todo->sig;
	tf->regs[7] = curenv->env_id;
	tf->regs[29] -= 4 * sizeof(tf->regs[4]);	
	tf->cp0_epc = curenv->env_sig_entry; // 跳转到异常处理入口
}

void do_sigill(struct Trapframe *tf) {
	sys_kill(0, SIGILL);
	return;
}

#if !defined(LAB) || LAB >= 4
/* Overview:
 *   This is the TLB Mod exception handler in kernel.
 *   Our kernel allows user programs to handle TLB Mod exception in user mode, so we copy its
 *   context 'tf' into UXSTACK and modify the EPC to the registered user exception entry.
 *
 * Hints:
 *   'env_user_tlb_mod_entry' is the user space entry registered using
 *   'sys_set_user_tlb_mod_entry'.
 *
 *   The user entry should handle this TLB Mod exception and restore the context.
 */
void do_tlb_mod(struct Trapframe *tf) {
	struct Trapframe tmp_tf = *tf;

	if (tf->regs[29] < USTACKTOP || tf->regs[29] >= UXSTACKTOP) {
		tf->regs[29] = UXSTACKTOP;
	}
	tf->regs[29] -= sizeof(struct Trapframe);
	*(struct Trapframe *)tf->regs[29] = tmp_tf;
	Pte *pte;
	page_lookup(cur_pgdir, tf->cp0_badvaddr, &pte);
	if (curenv->env_user_tlb_mod_entry) {
		tf->regs[4] = tf->regs[29];
		tf->regs[29] -= sizeof(tf->regs[4]);
		// Hint: Set 'cp0_epc' in the context 'tf' to 'curenv->env_user_tlb_mod_entry'.
		/* Exercise 4.11: Your code here. */
		tf->cp0_epc = curenv->env_user_tlb_mod_entry;

	} else {
		panic("TLB Mod but no user handler registered");
	}
}
#endif
