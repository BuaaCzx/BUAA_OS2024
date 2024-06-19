#ifndef _ENV_H_
#define _ENV_H_

#include <mmu.h>
#include <queue.h>
#include <trap.h>
#include <types.h>

#define LOG2NENV 10
#define NENV (1 << LOG2NENV)
#define ENVX(envid) ((envid) & (NENV - 1))
// 用法示例：struct Env *e = envs + ENVX(envid);
// 获取一个envid对应的env进程块的位置

// All possible values of 'env_status' in 'struct Env'.
#define ENV_FREE 0
#define ENV_RUNNABLE 1
#define ENV_NOT_RUNNABLE 2

// challenge

typedef struct sigset_t {
    uint32_t sig;
} sigset_t;

struct sigaction {
    void     (*sa_handler)(int);
    sigset_t   sa_mask;
};

#define SIGINT 2
#define SIGKILL 4
#define SIGKILL 9
#define SIGSEGV 11
#define SIGCHLD 17
#define SIGSYS 31

#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3

/*
SIGINT	2	中断信号	停止进程
SIGILL	4	非法指令	停止进程
SIGKILL	9	停止进程信号	强制停止该进程，不可被阻塞
SIGSEGV	11	访问地址错误，当访问[0, 0x003f_e000)内地址时	停止进程
SIGCHLD	17	子进程终止信号	忽略
SIGSYS	31	系统调用号未定义	忽略
*/

// end_challenge

// Control block of an environment (process).
struct Env {
	struct Trapframe env_tf;	 // saved context (registers) before switching
	LIST_ENTRY(Env) env_link;	 // intrusive entry in 'env_free_list'
	u_int env_id;			 // unique environment identifier
	u_int env_asid;			 // ASID of this env
	u_int env_parent_id;		 // env_id of this env's parent
	u_int env_status;		 // status of this env
	Pde *env_pgdir;			 // page directory
	TAILQ_ENTRY(Env) env_sched_link; // intrusive entry in 'env_sched_list'
	u_int env_pri;			 // schedule priority

	// Lab 4 IPC
	u_int env_ipc_value;   // the value sent to us
	u_int env_ipc_from;    // envid of the sender
	u_int env_ipc_recving; // whether this env is blocked receiving
	u_int env_ipc_dstva;   // va at which the received page should be mapped
	u_int env_ipc_perm;    // perm in which the received page should be mapped

	// Lab 4 fault handling
	u_int env_user_tlb_mod_entry; // userspace TLB Mod handler

	// Lab 6 scheduler counts
	u_int env_runs; // number of times we've been env_run'ed

	// challenge
	u_int env_handlers[105];
    sigset_t env_sa_mask; 
	/*
	当前的信号掩码。信号掩码（Signal Mask）是操作系统提供的一种机制，用于控制进程对接收特定信号的临时阻塞。
	它定义了一组当前进程中被阻止递送到进程的信号集合。
	当一个信号被设置在信号掩码中时，该信号不会立即传递给进程，直到从掩码中清除或进程特地检查并处理这些被阻塞的信号。
	*/
	sigset_t env_pending_sa; // 被阻塞且未处理的信号集

};

LIST_HEAD(Env_list, Env);
TAILQ_HEAD(Env_sched_list, Env);
extern struct Env *curenv;		     // the current env
extern struct Env_sched_list env_sched_list; // runnable env list

void env_init(void);
int env_alloc(struct Env **e, u_int parent_id);
void env_free(struct Env *);
struct Env *env_create(const void *binary, size_t size, int priority);
void env_destroy(struct Env *e);

int envid2env(u_int envid, struct Env **penv, int checkperm);
void env_run(struct Env *e) __attribute__((noreturn));

void env_check(void);
void envid2env_check(void);

#define ENV_CREATE_PRIORITY(x, y)                                                                  \
	({                                                                                         \
		extern u_char binary_##x##_start[];                                                \
		extern u_int binary_##x##_size;                                                    \
		env_create(binary_##x##_start, (u_int)binary_##x##_size, y);                       \
	})

#define ENV_CREATE(x)                                                                              \
	({                                                                                         \
		extern u_char binary_##x##_start[];                                                \
		extern u_int binary_##x##_size;                                                    \
		env_create(binary_##x##_start, (u_int)binary_##x##_size, 1);                       \
	})



#endif // !_ENV_H_
