#include <stdio.h>
#include <unistd.h>         /* For syscall */
#include <sys/syscall.h>    /* For SYS_open */
#include <fcntl.h>          /* For flags - O_CREAT, O_RDWR */

int main(void)
{
    int fd;
    const char *name = "test";
    const long flags = O_CREAT | O_RDWR;
    const long mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    /* Call "open" syscall */
    asm volatile(
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "movq %3, %%rsi\n"
        "movq %4, %%rdx\n"
        "syscall\n"
        "movq %%rax, %0\n"
        : "=m" (fd)
        : "X" (SYS_open), "X" (name), "X" (flags), "X" (mode)
        : "rax", "rdi", "rsi", "rdx"
    );

    if (fd <= 0) {
        perror("Error: Failed to open/create file");
        return -1;
    }

    /* Call "write" syscall */
    syscall(SYS_write, fd, "hello\n", 6);

    /* Call "close" syscall */
    close(fd);

    return 0;
}