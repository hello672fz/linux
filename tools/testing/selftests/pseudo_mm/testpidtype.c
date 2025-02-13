#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid = getpid();
    printf("PID as pid_t: %d\n", pid);

    unsigned long ul = (unsigned long) pid;
    printf("PID as unsigned long: %lu\n", ul);

    // 假设某个步骤导致PID被误解为指针
    unsigned long address = (unsigned long) &pid;
    printf("PID's address as unsigned long: %lx\n", address);

    // 将地址重新解释为pid_t
    pid_t new_pid = (pid_t) address;
    printf("New PID as pid_t: %d\n", new_pid);

    return 0;
}