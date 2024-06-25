// User-level IPC library routines

#include <env.h>
#include <lib.h>
#include <mmu.h>

// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.
//
// Hint: use syscall_yield() to be CPU-friendly.
void ipc_send(u_int whom, u_int val, const void *srcva, u_int perm) {
	int r;
	while ((r = syscall_ipc_try_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV) {
		syscall_yield();
	}
	user_assert(r == 0);
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.
//
// Hint: use env to discover the value and who sent it.
u_int ipc_recv(u_int *whom, void *dstva, u_int *perm) {
	int r = syscall_ipc_recv(dstva);
	if (r != 0) {
		user_panic("syscall_ipc_recv err: %d", r);
	}

	if (whom) {
		*whom = env->env_ipc_from;
	}

	if (perm) {
		*perm = env->env_ipc_perm;
	}

	return env->env_ipc_value;
}

// challenge

int is_illegal_sig(int signum) {
	return signum < 1 || signum > 32;
}

int sigaction(int signum, const struct sigaction *newact, struct sigaction *oldact){
	if (is_illegal_sig(signum)) {
		return -1;
	}
	return syscall_set_sigaction(0, signum, newact, oldact);
}

int kill(u_int envid, int sig){
	if (is_illegal_sig(sig)) {
		return 0;
	}
	return syscall_kill(envid, sig);
}

int sigemptyset(sigset_t *__set) {
	if (__set) {
		__set->sig = 0;
	}
	return 0;
}
// 清空参数中的__set掩码，初始化信号集以排除所有信号。这意味着__set将不包含任何信号。(清0)

int sigfillset(sigset_t *__set) {
	if (__set) {
		__set->sig = ~0;
	}
	return 0;
}
// 将参数中的__set掩码填满，使其包含所有已定义的信号。这意味着__set将包括所有信号。(全为1)

int sigaddset(sigset_t *__set, int __signo) {
	if (__set) {
		__set->sig |= 1 << (__signo - 1);
	}
	return 0;
}
// 向__set信号集中添加一个信号__signo。如果操作成功，__set将包含该信号。(置位为1)

int sigdelset(sigset_t *__set, int __signo) {
	if (__set) {
		__set->sig &= ~(1 << (__signo - 1));
	}
	return 0;
}
// 从__set信号集中删除一个信号__signo。如果操作成功，__set将不再包含该信号。(置位为0)

int sigismember(const sigset_t *__set, int __signo) {
	return (__set->sig >> (__signo - 1)) & 1;
}
// 检查信号__signo是否是__set信号集的成员。如果是，返回1；如果不是，返回0。

int sigisemptyset(const sigset_t *__set) {
	return __set->sig == 0;
}
// 检查信号集__set是否为空。如果为空，返回1；如果不为空，返回0。

int sigandset(sigset_t *__set, const sigset_t *__left, const sigset_t *__right) {
	__set->sig = __left->sig & __right->sig;
	return 0;
}
// 计算两个信号集__left和__right的交集，并将结果存储在__set中。

int sigorset(sigset_t *__set, const sigset_t *__left, const sigset_t *__right) {
	__set->sig = __left->sig | __right->sig;
}
// 计算两个信号集__left和__right的并集，并将结果存储在__set中。

int sigprocmask(int __how, const sigset_t * __set, sigset_t * __oset) {
	return syscall_proc_mask(0, __how, __set, __oset);
}
// 根据__how的值更改当前进程的信号屏蔽字。__set是要应用的新掩码，__oset（如果非NULL）则保存旧的信号屏蔽字。
// __how可以是SIG_BLOCK（添加__set到当前掩码）、SIG_UNBLOCK（从当前掩码中移除__set）、或SIG_SETMASK（设置当前掩码为__set）。

int sigpending(sigset_t *__set) {
	*__set = env->env_pending_sa;
}
// 获取当前被阻塞且未处理的信号集，并将其存储在__set中。
