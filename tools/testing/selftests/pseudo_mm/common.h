#ifndef __PSEUDO_MM_TEST_COMMON__
#define __PSEUDO_MM_TEST_COMMON__

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/pseudo_mm"
#define DAX_DEVICE_PATH "/dev/dax0.0"
#define IMAGE_FILE "one-page.img"
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

static inline char random_char(int i, unsigned long seed)
{
	unsigned long tmp = i * seed + seed - 1;
	return tmp % 30 + 'a';
}

static void fill_single_page(void *start, unsigned long seed)
{
	int i;
	char *iter;
	for (i = 0; i < PAGE_SIZE; i++) {
		iter = (char *)(start) + i;
		*iter = random_char(i, seed);
	}
}

int check_page_content(void *start, unsigned long seed)
{
	int i;
	char *iter;
	for (i = 0; i < PAGE_SIZE; i++) {
		iter = (char *)(start) + i;
		if (*iter != random_char(i, seed)) {
			printf("check_anon_page_content(%#lx, %#lx): (i = %d) expect %d at address %#lx find %d\n",
			       (unsigned long)start, seed, i,
			       random_char(i, seed), (unsigned long)iter,
			       *iter);
			return -1;
		}
	}
	return 0;
}

static int __fill_dax_device(unsigned long pgoff, unsigned long nr_pages,
			     unsigned long seed)
{
	int i, dax_fd;
	void *addr;
	dax_fd = open(DAX_DEVICE_PATH, O_RDWR);
	if (dax_fd < 0)
		return -1;
	addr = mmap(NULL, nr_pages << PAGE_SHIFT, PROT_READ | PROT_WRITE,
		    MAP_SHARED, dax_fd, pgoff << PAGE_SHIFT);
	if (!addr)
		return -1;
	for (i = 0; i < nr_pages; i++) {
		fill_single_page(addr + i * PAGE_SIZE, seed);
	}
	close(dax_fd);
	return 0;
}

int fill_dax_device(unsigned long pgoff, unsigned long nr_pages,
		    unsigned long seed)
{
	pid_t pid;

	pid = fork();
	if (pid < 0)
		return -1;

	if (pid == 0) {
		int ret;
		ret = __fill_dax_device(pgoff, nr_pages, seed);
		if (ret != 0)
			exit(EXIT_FAILURE);
		exit(EXIT_SUCCESS);
	} else {
		int status;
		if (waitpid(pid, &status, 0) != pid)
			return -1;
		if (!WIFEXITED(status))
			return -1;
		if (WEXITSTATUS(status) != 0)
			return -1;
	}
	return 0;
}

/*
 * This add a read-write map to `pseudo_mm_id`.
 */
int add_mmap_to(int pseudo_mm_fd, int pseudo_mm_id, unsigned long start,
		unsigned long end, unsigned long flags, int fd, off_t offset)
{
	struct pseudo_mm_add_map_param add_anon_param;
	int ret;

	add_anon_param.id = pseudo_mm_id;
	add_anon_param.start = start;
	add_anon_param.end = end;
	add_anon_param.prot = PROT_READ | PROT_WRITE;
	add_anon_param.flags = flags;
	add_anon_param.fd = fd;
	add_anon_param.offset = offset;
	printf("add map start\n");
	fflush(stdout);
	ret = ioctl(pseudo_mm_fd, PSEUDO_MM_IOC_ADD_MAP,
		    (void *)(&add_anon_param));
	printf("add map finish %d\n", ret);
	fflush(stdout);
	return ret;
}

int setup_anon_map_pt(int fd, int pseudo_mm_id, unsigned long start,
		      unsigned long size, unsigned long pgoff)
{
	struct pseudo_mm_setup_pt_param param;
	int ret;

	param.id = pseudo_mm_id;
	param.numaid = 0;
	param.start = start;
	param.size = size;
	param.pgoff = pgoff;
	param.type = DAX_MEM;
	printf("fill DAX anon start\n");
	fflush(stdout);
	ret = ioctl(fd, PSEUDO_MM_IOC_SETUP_PT, (void *)(&param));
	printf("fill DAX anon finish: %d\n", ret);
	fflush(stdout);
	return ret;
}

int setup_anon_map_pt_rdma(int fd, int pseudo_mm_id, unsigned long start,
			   unsigned long size, unsigned long pgoff)
{
	struct pseudo_mm_setup_pt_param param;
	int ret;

	param.id = pseudo_mm_id;
	param.start = start;
	param.size = size;
	param.pgoff = pgoff;
	param.type = RDMA_MEM;
	printf("fill RDMA anon start\n");
	fflush(stdout);
	ret = ioctl(fd, PSEUDO_MM_IOC_SETUP_PT, (void *)(&param));
	printf("fill RDMA anon finish: %d\n", ret);
	fflush(stdout);
	return ret;
}

int bring_back_map(int fd, int pseudo_mm_id, unsigned long start,
		   unsigned long size)
{
	struct pseudo_mm_bring_back_param param;
	int ret;

	param.id = pseudo_mm_id;
	param.start = start;
	param.size = size;
	ret = ioctl(fd, PSEUDO_MM_IOC_BRING_BACK, (void *)(&param));
	return ret;
}

int get_pseudo_mm_pf_stat(int fd, pid_t pid, unsigned long *cow_nr,
			  unsigned long *rdma_read_nr)
{
	struct pseudo_mm_pf_stat_param param;
	int ret;

	param.pid = pid;
	ret = ioctl(fd, PSEUDO_MM_IOC_PF_STAT, (void *)(&param));
	if (ret)
		return ret;
	if (cow_nr)
		*cow_nr = param.cow_nr;
	if (rdma_read_nr)
		*rdma_read_nr = param.rdma_read_nr;
	return 0;
}

unsigned long djb_hash(const char *cp)
{
	unsigned long hash = 5381;
	while (*cp)
		hash = 33 * hash ^ (unsigned char)*cp++;
	return hash;
}

static int __create_and_fill_file(const char *path, size_t size, off_t offset)
{
	int i;
	int fd, ret;
	unsigned long seed = djb_hash(path);
	char buf[PAGE_SIZE];

	size = (size + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));
	fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		return fd;
	}

	fill_single_page((void *)buf, seed);

	for (i = 0; i < (size >> PAGE_SHIFT); i++) {
		ret = pwrite(fd, buf, PAGE_SIZE, offset + i * PAGE_SIZE);
		if (ret != PAGE_SIZE) {
			close(fd);
			return -1;
		}
	}
	fsync(fd);
	close(fd);
	return 0;
}

/* return fd of path */
int create_and_fill_file(const char *path, size_t size, off_t offset)
{
	pid_t pid;
	pid = fork();
	if (pid < 0) {
		printf("fork failed\n");
		return -1;
	}
	if (pid == 0) {
		int ret;
		ret = __create_and_fill_file(path, size, offset);
		if (ret) {
			printf("__create_and_fill_file failed\n");
			exit(EXIT_FAILURE);
		}
		printf("child %d exit\n", getpid());
		exit(EXIT_SUCCESS);
	} else {
		int status;

		if (waitpid(pid, &status, 0) != pid) {
			printf("waitpid failed\n");
			return -1;
		}
		if (!WIFEXITED(status)) {
			printf("WIFEXITED failed\n");
			return -1;
		}
		if (WEXITSTATUS(status) != 0) {
			printf("WEXITSTATUS failed\n");
			return -1;
		}
	}

	return open(path, O_RDWR);
}

#endif
