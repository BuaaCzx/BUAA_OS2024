#include <lib.h>

#define assert(x)                                                                                  \
	do {                                                                                       \
		if (!(x)) {                                                                        \
			user_panic("assertion failed: %s", #x);                                         \
		}                                                                                  \
	} while (0)

void sigint_handler(int sig) {
	debugf("sth unexpected happened\n");
	exit();
}

int main() {
	sigset_t st1, st2, st3, st4;
	u_int _st1 = 0x289ca9ed, _st2 = 0xab3cfe27;
	assert(sigemptyset(&st1) == 0);
	assert(sigemptyset(&st2) == 0);
	for(int i = 0; i <= 10; i++) {
		if (i != SIG_BLOCK && i != SIG_UNBLOCK && i != SIG_SETMASK) {
			assert(sigprocmask(i, &st1, &st2) == -1);
		}
	}
	assert(sigaddset(&st1, 0) == -1);
	assert(sigdelset(&st1, 33) == -1);
	assert(sigismember(&st1, 321) == -1);
	assert(sigisemptyset(&st1) == 1);
	assert(sigfillset(&st1) == 0);
	for(int i = 0; i < 32; i++) {
		if (!(_st1 >> i & 1)) {
			assert(sigdelset(&st1, i + 1) == 0);
		}
		if (_st2 >> i & 1) {
			assert(sigaddset(&st2, i + 1) == 0);
		}
	}
	for(int i = 0; i < 32; i++) {
		assert(sigismember(&st1, i + 1) == (_st1 >> i & 1));
		assert(sigismember(&st2, i + 1) == (_st2 >> i & 1));
	}
	assert(sigandset(&st3, &st1, &st2) == 0);
	for(int i = 0; i < 32; i++) {
		assert(sigismember(&st3, i + 1) == ((_st1 & _st2) >> i & 1));
	}
	assert(sigorset(&st3, &st1, &st2) == 0);
	for(int i = 0; i < 32; i++) {
		assert(sigismember(&st3, i + 1) == ((_st1 | _st2) >> i & 1));
	}
	assert(sigprocmask(SIG_BLOCK, NULL, NULL) == 0);
	assert(sigprocmask(SIG_BLOCK, &st3, &st4) == 0);
	for(int i = 0; i < 32; i++) {
		assert(sigismember(&st4, i + 1) == 0);
	}
	assert(sigprocmask(SIG_UNBLOCK, &st1, &st4) == 0);
	for(int i = 0; i < 32; i++) {
		assert(sigismember(&st4, i + 1) == ((_st2 | _st1) >> i & 1));
	}
	assert(sigprocmask(SIG_SETMASK, &st1, &st4) == 0);
	for(int i = 0; i < 32; i++) {
		assert(sigismember(&st4, i + 1) == ((_st2 & ~_st1) >> i & 1));
	}
	assert(sigprocmask(SIG_SETMASK, NULL, &st4) == 0);
	for(int i = 0; i < 32; i++) {
		assert(sigismember(&st4, i + 1) == (_st1 >> i & 1));
	}
	struct sigaction sa, sa2;
	sa.sa_handler = sigint_handler;
	assert(sigemptyset(&sa.sa_mask) == 0);
	assert(sigaction(SIGSEGV, &sa, NULL) == 0);
	assert(sigaction(SIGSEGV, NULL, &sa2) == 0);
	assert(sa2.sa_handler == sa.sa_handler);
	assert(sigaction(0, &sa, NULL) == -1);
	assert(sigaction(33, &sa, NULL) == -1);
	assert(sigaction(32, NULL, NULL) == 0);
	debugf("Congratulations!\n");
	debugf("Congratulations!\n");
	debugf("Congratulations!\n");
	debugf("Congratulations!\n");
	debugf("Congratulations!\n");
	debugf("Congratulations!\n");
	return 0;
}