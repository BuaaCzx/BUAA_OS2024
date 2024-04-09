# lab2 课下笔记

## 宏定义

``ROUND(a,n)`` 返回 (a / n)(向上取整) * n。要求 n 必须是 2 的非负整数次幂。

``PADDR(kva)`` ：kseg0 处虚地址 --> 物理地址. PADDR(x) 是一个返回虚拟地址 x 所对应物理地址的宏，它定义在 include/mmu.h，该宏要求 x 必须是 kseg0 中的虚拟地址

``KADDR(pa)`` ：物理地址 --> kseg0 处虚地址（读取 pte 后可进行转换）

``PPN(pa)`` ：#define PPN(pa) (((u_long)(pa)) >> PGSHIFT)
PPN宏用于提取物理地址pa中的页号。这里，pa是一个指向物理内存的指针。宏首先将pa转换为u_long类型，然后使用右移操作符>>将地址右移PGSHIFT位。PGSHIFT是一个宏，它定义了页面大小（通常是2的幂）的对数。右移PGSHIFT位相当于将地址除以页面大小，得到的结果就是页号。

Lab2 中地址相关的常用宏
在 include/pmap.h 、 include/mmu.h 中：

PDX(va) ：页目录偏移量（查找遍历页表时常用）
PTX(va) ：页表偏移量（查找遍历页表时常用）
PTE_ADDR(pte) ：获取页表项中的物理地址（读取 pte 时常用）
PADDR(kva) ：kseg0 处虚地址 物理地址
KADDR(pa) ：物理地址 kseg0 处虚地址（读取 pte 后可进行转换）
va2pa(Pde *pgdir, u_long va) ：查页表，虚地址 物理地址（测试时常用）
pa2page(u_long pa) ：物理地址 页控制块（读取 pte 后可进行转换）
page2pa(struct Page *pp) ：页控制块 物理地址（填充 pte 时常用）

## 链表宏

2.4.1 链表宏
在后续的 Lab 中，也有一些地方需要用到链表，因此 MOS 中使用宏对链表的操作进行了
封装。这部分功能非常有用，设计技巧应用广泛，大家需要仔细阅读代码，深入理解。
链表宏的定义位于 include/queue.h，其实现了双向链表功能，下面将对一些主要的宏进
行解释:
• LIST_HEAD(name, type)，创建一个名称为 name 链表的头部结构体，包含一个指向 type
类型结构体的指针，这个指针可以指向链表的首个元素。

• LIST_ENTRY(type)，作为一个特殊的类型出现，例如可以进行如下的定义：

```
1 LIST_ENTRY(Page) a;
```

它的本质是一个链表项，包括指向下一个元素的指针 le_next，以及指向前一个元素链表
项 le_next 的指针 le_prev。le_prev 是一个指针的指针，它的作用是当删除一个元素
时，更改前一个元素链表项的 le_next。

• LIST_EMPTY(head)，判断 head 指针指向的头部结构体对应的链表是否为空。

• LIST_FIRST(head)，将返回 head 对应的链表的首个元素。

• LIST_INIT(head)，将 head 对应的链表初始化。

• LIST_NEXT(elm, field)，返回指针 elm 指向的元素在对应链表中的下一个元素的指针。
elm 指针指向的结构体需要包含一个名为 field 的字段，类型是一个链表项 LIST_ENTRY(type)，
下面出现的 field 含义均和此相同。

• LIST_INSERT_AFTER(listelm, elm, field)，将 elm 插到已有元素 listelm 之后。

• LIST_INSERT_BEFORE(listelm, elm, field)，将 elm 插到已有元素 listelm 之前。

• LIST_INSERT_HEAD(head, elm, field)，将 elm 插到 head 对应链表的头部。

• LIST_REMOVE(elm, field)，将 elm 从对应链表中删除。

## pmap.h 中的函数

```c
static inline u_long page2kva(struct Page *pp) {
	return KADDR(page2pa(pp));
}
// 这个函数完成了一个页控制块地址到对应的物理地址再到对应的虚拟地址的转换。
```

