#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include "pseudo_mm_ioctl.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

pid_t pid;

int add_mmap_to(int pseudo_mm_fd, int pseudo_mm_id, unsigned long start, unsigned long end, unsigned long flags, int fd, off_t offset)
{
	struct pseudo_mm_add_map_param add_map_param = {
		.id = pseudo_mm_id,
		.start = start,
		.end = end,
		.prot = PROT_READ | PROT_WRITE,
		.flags = flags,
		.fd = fd,
		.offset = offset
	};

	int ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_ADD_MAP, (void *)(&add_map_param));
	return ret;
}

int set_map_pt(int pseudo_mm_fd, int pseudo_mm_id, unsigned long start, unsigned long size, unsigned long pgoff)
{
	struct pseudo_mm_setup_pt_param set_pt_param = {
		.id = pseudo_mm_id,
		.start = start,
		.size = size,
		.pgoff = pgoff
	};

	int ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_SETUP_PT, (void *)(&set_pt_param));
	return ret;
}

int attach_to(int pseudo_mm_fd, int pseudo_mm_id){

	pid = getpid();

	struct pseudo_mm_attach_param attach_param = {
		.pid = pid,
		.id = pseudo_mm_id
	};

	int ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_ATTACH, (void *)(&attach_param));
	return ret;
}

int getpte(int pseudo_mm_fd, unsigned long start, unsigned long size)
{
	struct pseudo_mm_getpte_param getpte_param = {
		.pid = pid,
		.start = start,
		.size = size
	};

	int ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_GETPTE, (void *)(&getpte_param));
	return ret;
}

void hexdump(const void *data, long size) {
    const unsigned char *byte_data = (const unsigned char *)data;
    long offset = 0;

    while (offset < size) {
        // Print the offset (address) in hexadecimal
        printf("%08zx: ", offset);

        // Print the hexadecimal values for the current line (16 bytes per line)
        for (int i = 0; i < 16; i++) {
            if (offset + i < size) {
                printf("%02x ", byte_data[offset + i]);
            } else {
                printf("   "); // Pad with spaces if fewer than 16 bytes remain
            }
        }

        // Print the ASCII representation of the current line
        printf(" ");
        for (int i = 0; i < 16; i++) {
            if (offset + i < size) {
                unsigned char c = byte_data[offset + i];
                printf("%c", (c >= 32 && c < 127) ? c : '.'); // Print printable characters, else '.'
            } else {
                printf(" "); // Pad with spaces if fewer than 16 bytes remain
            }
        }

        printf("\n");
        offset += 16; // Move to the next 16-byte block
    }
}



int main() {
    int pseudo_mm_fd, pseudo_mm_id;
    int ret;
    unsigned long start, end;
    off_t offset;
    unsigned long pgoff;

    start = 0xdead0UL << PAGE_SHIFT;
    end = start + PAGE_SIZE;
        
    pseudo_mm_fd = open("/dev/pseudo_mm", O_RDWR);
    if(pseudo_mm_fd < 0){
        perror("Open pseudo_mm failed");
        return -1;
    }

    // step 1: create
    ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_CREATE, (void *)(&pseudo_mm_id));
    if(ret){
        perror("create failed");
        return -1;
    }

    // step 2: add map
    ret = add_mmap_to(pseudo_mm_fd, pseudo_mm_id, start, end, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if(ret){
        perror("add map failed");
        return -1;
    }

    // step 3: set pt
    ret = set_map_pt(pseudo_mm_fd, pseudo_mm_id, start, PAGE_SIZE, 0);
    if(ret){
        perror("set pt failed");
        return -1;
    }

    // step 4: attach
    ret = attach_to(pseudo_mm_fd, pseudo_mm_id);
    if(ret){
        perror("attach failed");
        return -1;
    }

    // step 5: getpte
    ret = getpte(pseudo_mm_fd, start, PAGE_SIZE);
    if(ret){
        perror("get pte failed");
        return -1;
    }

    hexdump(start, PAGE_SIZE);
    
    // step 5: delete
    ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_DELETE, (void *)(&pseudo_mm_id));

    return 0;
}