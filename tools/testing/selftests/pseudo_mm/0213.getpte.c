cat << EOF > getpte.c 
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include "pseudo_mm_ioctl.h"
#include <errno.h>
#include <limits.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)


int getpte(int pseudo_mm_fd,pid_t pid)
{
	struct pseudo_mm_getpte_param getpte_param = {
		.pid = pid
	};

	int ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_GETPTE, (void *)(&getpte_param));
	return ret;
}


int main(int argc, char *argv[]) {
    int pseudo_mm_fd;
    int ret;
    // int *endptr; 

    if (argc < 2) {
        printf("Usage: %s <pid>\n", argv[0]);
        return 0;
    }
    char *endptr;
    errno = 0; // 重置 errno 以检测错误
    long val = strtol(argv[1], &endptr, 10);

    // 检查转换错误
    if (errno != 0 || *endptr != '\0' || val > INT_MAX || val < INT_MIN) {
        fprintf(stderr, "Invalid pid: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    pid_t pid = (pid_t)val;
    // 现在可以使用 pid 变量进行后续操作
    printf("The pid is: %d\n", pid);

    pseudo_mm_fd = open("/dev/pseudo_mm", O_RDWR);
    if(pseudo_mm_fd < 0){
        perror("Open pseudo_mm failed");
        return -1;
    }


    // step 5: getpte
    ret = getpte(pseudo_mm_fd,pid);
    if(ret){
        perror("get pte failed");
        return -1;
    }

    return 0;
}
EOF