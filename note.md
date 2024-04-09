# lab2 课下笔记

## 宏定义

``ROUND(a,n)`` 返回 (a / n)(向上取整) * n。要求 n 必须是 2 的非负整数次幂。

``PADDR(kva)`` ：kseg0 处虚地址 --> 物理地址. PADDR(x) 是一个返回虚拟地址 x 所对应物理地址的宏，它定义在 include/mmu.h，该宏要求 x 必须是 kseg0 中的虚拟地址

``KADDR(pa)`` ：物理地址 --> kseg0 处虚地址（读取 pte 后可进行转换）