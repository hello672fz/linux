cat << EOF > shm2.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "pseudo_mm_ioctl.h"

int main() {
    int pseudo_mm_fd;
    int ret;
    long long phy_addr;
    
    pseudo_mm_fd = open("/dev/pseudo_mm", O_RDWR);
    if(pseudo_mm_fd < 0){
        perror("Open pseudo_mm failed");
        return -1;
    }

    struct pseudo_mm_register_param param = {
        .node = 0,
        .order = 1
    };

    ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_REGISTER, (void *)(&param));
    if(ret){
        perror("Register failed");
        return -1;
    }

    ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_PHY_ADDR, (void *)(&phy_addr));
    if(ret){
        perror("Get phy addr failed");
        return -1;
    }

    printf("Phy Addr : %lld\n", phy_addr);

}
EOF