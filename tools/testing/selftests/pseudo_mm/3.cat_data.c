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

    off_t phys_addr = strtol(argv[1], &endptr, 16);

    if (*endptr != '\0') {
        printf("Conversion failed. Invalid character: %s\n", endptr);
        return EXIT_FAILURE;
    }

    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *map_base = mmap(NULL,
			PAGE_SIZE,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			mem_fd,
			phys_addr);	// phys_addr should be page-aligned.	

    if (map_base == MAP_FAILED) {
        perror("Can't map memory");
        return EXIT_FAILURE;
    }

    hexdump(map_base, PAGE_SIZE);
    return 0;
}