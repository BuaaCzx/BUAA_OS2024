#include <lib.h>

void sigsys_handler(int sig) {
    debugf("capture SIGSYS signal.\n");
    exit();
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigsys_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSYS, &sa, NULL);

    asm("\t li $a0,0x2233\r\n"
        "\t syscall \r\n");

    debugf("If you see this on your screen, it means that the signal is not handled correctly.\n");
    return 0;
}
