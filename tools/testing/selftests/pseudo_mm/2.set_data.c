#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void hexdump(const void *data, size_t size) {
    const unsigned char *byte_data = (const unsigned char *)data;
    size_t offset = 0;

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

int main(int argc, char *argv[]) {

    char *endptr; 
    size_t PAGE_SIZE = 4096;

    if (argc < 2) {
        printf("Usage: %s <phys_addr>\n", argv[0]);
        return 0;
    }

    // Parse physical address
    unsigned long long phys_addr_ull = strtoull(argv[1], &endptr, 16);
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid physical address: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    off_t phys_addr = (off_t)phys_addr_ull;

    // Check if the address is page-aligned
    if (phys_addr % PAGE_SIZE != 0) {
        fprintf(stderr, "Physical address 0x%llx is not aligned to page size 0x%zx\n",
        phys_addr_ull, PAGE_SIZE);
        return EXIT_FAILURE;
    }

    // Open /dev/mem
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    // Map the physical memory
    off_t phys_offset = (off_t)phys_addr_ull;
    void *map_base = mmap(NULL,
			PAGE_SIZE,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			mem_fd,
			phys_offset);	// phys_offset should be page-aligned.	

    if (map_base == MAP_FAILED) {
        perror("Can't map memory");
        close(mem_fd);
        return EXIT_FAILURE;
    }

    // Cleanup file descriptor as it's no longer needed
    close(mem_fd);

    // memory write
    memset(map_base, 0xcc, PAGE_SIZE);

    // Unmap the memory
    if (munmap(map_base, PAGE_SIZE) == -1) {
        perror("munmap failed");
        return EXIT_FAILURE;
    }

    return 0;
}