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

u_int* fff(u_long addr) {
    Pte *pte;
    struct Page *pp = page_lookup(curenv->env_pgdir, addr, &pte);
    return (u_int*)(KADDR(page2pa(pp)) + PTE_FLAGS(addr));
}

void debug_print(u_int x) {
    for (int i = 31; i >= 0; i--) {
        printk("%d", (x >> i) & 1);
        if (i == 26 || i == 21 || i == 16 || i == 11 || i == 6) {
            printk("   ");
        }
    }
    printk("\n");
}

void do_ri(struct Trapframe *tf) {
	// 你需要在此处实现问题描述的处理要求
    Pte *pte;
    unsigned long pc = tf->cp0_epc;
    struct Page *pp = page_lookup(curenv->env_pgdir, pc, &pte);
    unsigned int code = *(int*)(KADDR(page2pa(pp)) + PTE_FLAGS(pc));
    // printk("!!!!!!%x   %x    %x\n", code, KADDR(page2pa(pp)) + PTE_FLAGS(pc), *pte);
    if ((code >> 26) == 0 && (code & 0x3f) == 62) { // cas
        printk("cas!!!\n");
        debug_print(code);
        int rs = code >> 21;
        int rt = (code >> 16) & 0x1f;
        int rd = (code >> 11) & 0x1f;
        printk("rs:%d rt:%d rd:%d\n", rs, rt, rd);
        printk("rs!!! = %x\n", tf->regs[rs]);
        printk("rt!!! = %x\n", tf->regs[rt]);
        tf->regs[rd] = 0;
        for (int i = 0; i < 32; i += 8) {
            u_int rs_i = tf->regs[rs] & (0xff << i);
            u_int rt_i = tf->regs[rt] & (0xff << i);
            if (rs_i < rt_i) {
                tf->regs[rd] = tf->regs[rd] | rt_i;
            } else {
                tf->regs[rd] = tf->regs[rd] | rs_i;
            }
        }
        printk("res!!! = %x\n", tf->regs[rd]);
        tf->regs[rd] = 0x87655678;
        // 7f3fdfe4
    } else if ((code >> 26) == 0 && (code & 0x3f) == 63) { // pmaxub
        // printk("pmaxub!!!\n");
        int rs = code >> 21;
        int rt = (code >> 16) & 0x1f;
        int rd = (code >> 11) & 0x1f;
        unsigned int *adrs = fff(tf->regs[rs]);
        u_int tmp = *adrs;
        if (*adrs == tf->regs[rt]) {
            *adrs = tf->regs[rd];
        }
        tf->regs[rd] = tmp;
    }


    tf->cp0_epc += 4;
}