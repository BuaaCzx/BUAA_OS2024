#

## 系统调用函数

`void sys_putchar(int c)` : 输出一个字符到屏幕

`int sys_print_cons(const void *s, u_int num)` : 输出一个字符串到屏幕

`u_int sys_getenvid(void)` : 获取当前进程的id

`int sys_env_destroy(u_int envid)` : 杀死一个进程

`int sys_mem_alloc(u_int envid, u_int va, u_int perm)` : 给进程envid的虚拟地址va分配一个物理页

## vpd, vpt 的使用

(vpt)[va >> PGSHIFT]       // 获得对应的页表项
(vpd)[va >> PDSHIFT]       // 获得对应的页目录项

一个访问例子：
if ((vpd[i >> 10] & PTE_V) && (vpt[i] & PTE_V)) // 一级页表：vpd，二级页表：vpt
i表示第几个页，使用时可以用VPN(va)来获取。

#define PTE_ADDR(pte) (((u_long)(pte)) & ~0xFFF)
// 获取页表项之后，把页表项转换成物理地址

## 2022 shared

```c
int make_shared(void *va) {
    int r;
    // 若 va 不在用户空间中（大于或等于 UTOP），
    // 或者当前进程的页表中已存在该虚拟页，但进程对其没有写入权限，则该函数应返回 -1 表示失败，不产生任何影响。
    if (va >= (void *) UTOP || ((*vpt)[VPN(va)] & PTE_W) == 0) {
        return -1;
    }

    // 若当前进程的页表中不存在该虚拟页，该函数应首先分配一页物理内存，并将该虚拟页映射到新分配的物理页，使当前进程能够读写该虚拟页。
    // 若无法分配新的物理页，该函数应返回 -1 表示失败。
    if ((*vpt)[VPN(va)] == 0) {
        r = syscall_mem_alloc(0, va, PTE_V | PTE_R | PTE_W);
        if (r < 0) {
            return -1;
        }
    }

    // 将当前进程中虚拟地址 va 所属的虚拟页标记为共享页，并返回其映射到的物理页的物理地址。
    (*vpt)[VPN(va)] |= PTE_SHARE;
    return (*vpt)[VPN(va)] & 0xfffff000; // 可以用PTE_ADDR来实现，定义在mmu.h里
}

// final

int make_shared(void *va) {
    int r;
    u_int perm = (*vpt)[VPN(va)] & 0xfff;
    if (va >= (void *) UTOP)
        return -1;

    if ((perm & PTE_V) == 0 || ((*vpd)[PDX(va)] & PTE_V) == 0) {
        if ((r = syscall_mem_alloc(syscall_getenvid(), ROUNDDOWN(va, BY2PG), PTE_V | PTE_R | PTE_LIBRARY)) < 0)
            return -1;
        return ROUNDDOWN((*vpt)[VPN(va)] & 0xfffff000, BY2PG);
    }

    if ((perm & PTE_R) == 0) {
        return -1;
    }

    (*vpt)[VPN(va)] |= PTE_LIBRARY;
    return ROUNDDOWN((*vpt)[VPN(va)] & 0xfffff000, BY2PG);
}
```
