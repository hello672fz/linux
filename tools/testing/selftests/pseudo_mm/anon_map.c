#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "pseudo_mm_ioctl.h"

#define DEVICE_PATH "/dev/pseudo_mm"
#define DAX_DEVICE_PATH "/dev/dax0.0"
#define IMAGE_FILE "one-page.img"
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define REGION_SIZE (1UL << 32)

//#define ASSERT(condition) \
//	do { \
//		if (!(condition)) { \
//			return -1; \
//		} \
//	} while (0)

#define ASSERT_GOTO(condition, label, err_message) \
	do { \
		if (!(condition)) { \
			perror(err_message); \
			goto label; \
		} \
} while (0)

#define TEST(not_expect, err_message) \
	do { \
		if (not_expect) { \
			perror(err_message); \
		} \
	} while (0)

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
	pid_t pid = getpid();

	struct pseudo_mm_attach_param attach_param = {
		.pid = pid,
		.id = pseudo_mm_id
	};

	int ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_ATTACH, (void *)(&attach_param));
	return ret;
}

void fill_single_page(void *start, unsigned long seed){
	int i;
	char *iter;

	for(i = 0; i < PAGE_SIZE; i++){
		iter = (char *)start + i;
		*iter = (seed + i) % 26 + 'a';
	}
}

int fill_dax_device(unsigned long pgoff, unsigned long nr_pages, unsigned long seed){
	int dax_fd;
	void *addr;
	int i;

	dax_fd = open(DAX_DEVICE_PATH, O_RDWR);
	ASSERT_GOTO(dax_fd > 0, err, "fill_dax_device: open dax device failed");

	addr = mmap(NULL, REGION_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dax_fd, pgoff << PAGE_SHIFT);
	ASSERT_GOTO(addr != MAP_FAILED, err, "fill_dax_device: mmap failed");

	for(i = 0; i < nr_pages; i++){
		fill_single_page(addr + i * PAGE_SIZE, seed);
	}
	close(dax_fd);
	return 0;
err:
	return -1;
}

int check_context(void* start, unsigned long nr_pages, unsigned long seed){
	int i, j;
	char *iter;
	char expect_ch;

	for(i = 0; i < nr_pages; i++){
		for (j = 0; j < PAGE_SIZE; j++){
			iter = (char *)start + i * PAGE_SIZE + j;
			expect_ch = (seed + j) % 26 + 'a';
			if (*iter != expect_ch){
				return -1;
			}
		}
	}
	return 0;
}

void write_context(void* start, unsigned long nr_pages, unsigned long seed){
	int i, j;
	char *iter;

	for(i = 0; i < nr_pages; i++){
		for (j = 0; j < PAGE_SIZE; j++){
			iter = (char *)start + i * PAGE_SIZE + j;
			*iter = (seed + j) % 26 + 'a';
		}
	}
}


int main(){
	int dax_fd;
	int pseudo_mm_fd, pseudo_mm_id;
	int ret = -1;
	unsigned long start, end;

	start = 0xdead0UL << PAGE_SHIFT;
	end = start + PAGE_SIZE;

	// fill page
	fill_dax_device(/* pgoff */ 0, /* nr_pages */ 1, /* seed */ 0);

	dax_fd = open(DAX_DEVICE_PATH, O_RDWR);
	ASSERT_GOTO(dax_fd > 0, err0, "open dax device failed");

	pseudo_mm_fd = open(DEVICE_PATH, O_RDWR);
	ASSERT_GOTO(pseudo_mm_fd > 0, err1, "open pseudo_mm device failed");

	// step1: register
	ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_REGISTER, (void *)(&dax_fd));
	ASSERT_GOTO(ret == 0, err2, "register failed");

	// step2: create
	ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_CREATE, (void *)(&pseudo_mm_id));
	ASSERT_GOTO(ret == 0, err2, "create pseudo_mm failed");

	// step3: add map
	ret = add_mmap_to(pseudo_mm_fd, pseudo_mm_id, start, end, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	ASSERT_GOTO(ret == 0, err3, "add map failed");

	// step4: set pt
	ret = set_map_pt(pseudo_mm_fd, pseudo_mm_id, start, PAGE_SIZE, 0);
	ASSERT_GOTO(ret == 0, err3, "set pt failed");

	// step5: attach
	ret = attach_to(pseudo_mm_fd, pseudo_mm_id);
	ASSERT_GOTO(ret == 0, err3, "attach failed");

	ret = check_context(/* start */ (void *)start, /* nr_pages */ 1, /* seed*/ 0);

err3:
	// step6: delete
	ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_DELETE, (void *)(&pseudo_mm_id));
err2:
	close(pseudo_mm_fd);
err1:
	close(dax_fd);
err0:
	return ret;
}
