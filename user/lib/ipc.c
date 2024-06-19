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

int sigaction(int signum, const struct sigaction *newact, struct sigaction *oldact) {
	if (signum > 32) {
		return -1;
	}

	try(syscall_get_sigaction(0, signum, oldact));

	// TODO
}

/*
signum:需要设置的信号编号。如之前所说你只需考虑signum 小于或等于32的情况，当收到编号大于32的信号时直接返回异常码-1即可。

newact:为signum设置的sigaction结构体,如果newact不为空。

oldact: 将该信号之前的sigaction结构体其内容填充到oldact中(如果oldact不为空)。

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    if (signum > 64 || signum < 1) {
        return -1;
    }
    if (syscall_get_sig_act(0, signum, oldact) != 0) { 
        //旧的信号处理结构体则需要在 `oldact != NULL` 时保存该指针在对应的地址空间中
        return -1;
    }
    if (env_set_sig_entry() != 0) { //【注意这里的代码后续会解释！】
        return -1;
    }
    return syscall_set_sig_act(0, signum, act);
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    return syscall_set_sig_set(0, how, set, oldset);
}

int kill(u_int envid, int sig) {
    return syscall_kill(envid, sig);
    //向进程控制号编号为 `envid` 的进程发送 `sig` 信号
}

*/