#include <env.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);
extern void handle_ri(void);

void (*exception_handlers[32])(void) = {
    [0 ... 31] = handle_reserved,
    [0] = handle_int,
    [2 ... 3] = handle_tlb,
    [10] = handle_ri,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};

/* Overview:
 *   The fallback handler when an unknown exception code is encountered.
 *   'genex.S' wraps this function in 'handle_reserved'.
 */
void do_reserved(struct Trapframe *tf) {
	print_tf(tf);
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}

void do_ri(struct Trapframe *tf) {
	// 你需要在此处实现问题描述的处理要求
    Pte *pte;
    unsigned long pc = tf->cp0_epc;
    struct Page *pp = page_lookup(curenv->env_pgdir, pc, &pte);
    unsigned int code = *(int*)(KADDR(page2pa(pp)) + PTE_FLAGS(*pte));
    printk("!!!!!!%x   %x    %x\n", code, KADDR(page2pa(pp)) + PTE_FLAGS(*pte), *pte);
    if ((code >> 26) == 0 && (code & 0x3f) == 62) { // cas
        printk("cas!!!\n");
    } else if ((code >> 26) == 0 && (code & 0x3f) == 63) { // pmaxub
        printk("pmaxub!!!\n");
    }


    tf->cp0_epc += 4;
}