#include <lib.h>
#include <signal.h>

void sigchld_handler(int sig) {
    debugf("capture SIGCHLD signal.\n");
    debugf("shouldn't see this.\n");
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);
    u_int pid = syscall_getenvid();
    if (fork() == 0) {
        debugf("send kill to %x\n", pid);
        kill(pid, SIGKILL);
        exit();
    }
    while (1);
   return 0;
}