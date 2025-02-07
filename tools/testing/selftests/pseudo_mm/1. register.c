#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "pseudo_mm_ioctl.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

long long phy_addr;

int main() {
    int pseudo_mm_fd, pseudo_mm_id;
    int ret;
    unsigned long start, end;
    off_t offset;
    unsigned long pgoff;
    long long phy_addr;

    start = 0xdead0UL << PAGE_SHIFT;
	end = start + PAGE_SIZE;
    
    
    pseudo_mm_fd = open("/dev/pseudo_mm", O_RDWR);
    if(pseudo_mm_fd < 0){
        perror("Open pseudo_mm failed");
        return -1;
    }

    // step 1: register a 4GB contiguous physical mem
    struct pseudo_mm_register_param param = {
        .node = 0,
        .order = 20
    };

    ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_REGISTER, (void *)(&param));
    if(ret){
        perror("Register failed");
        return -1;
    }

    // step 2: get phy addr
    ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_PHY_ADDR, (void *)(&phy_addr));
    if(ret){
        perror("Get phys_addr failed");
        return -1;
    }
    printf("phys_addr : %llx\n", phy_addr);

	close(pseudo_mm_fd);
  
    return 0;
}