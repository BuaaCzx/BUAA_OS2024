#

## 系统调用函数

`void sys_putchar(int c)` : 输出一个字符到屏幕

`int sys_print_cons(const void *s, u_int num)` : 输出一个字符串到屏幕

`u_int sys_getenvid(void)` : 获取当前进程的id

`int sys_env_destroy(u_int envid)` : 杀死一个进程

`int sys_mem_alloc(u_int envid, u_int va, u_int perm)` : 给进程envid的虚拟地址va分配一个物理页