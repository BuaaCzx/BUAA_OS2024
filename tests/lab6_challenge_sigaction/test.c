#include <lib.h>

void sigsegv_handler(int sig) {
    debugf("capture SIGSEGV signal.\n");
    exit();
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigsegv_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    int* ptr = (int*)0x0;
    debugf("Accessing invalid memory address...\n");
    int val = *ptr; 
    debugf("If you see this on your screen, it means that the signal is not handled correctly.\n");
    return 0;
}
