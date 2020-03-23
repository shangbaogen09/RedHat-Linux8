#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PAGE_SIZE 4096


int main(int argc, char**argv) {
    int fd;
    int i;
    unsigned char *p_map;

    if (argv[1] == NULL) {
        printf("command line is empty!\n");
        exit(1);
    }

    fd = open("test.dat", O_RDWR | O_APPEND | O_CREAT);
    if (fd<0) {
        printf("open failed!\n");
        exit(1);
    }

    p_map = (unsigned char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p_map == MAP_FAILED) {
        printf("mmap failed!\n");
        goto end;
    }

    // 读取有效数据
    for (i=0;i<PAGE_SIZE;i++) {
        if (p_map[i] == 0) {
            break;
        }
    }

    printf("length: [%d]\n%s\n", i, p_map);

    char * line = argv[1];

    printf("line length: %lu\n", strlen(line));

    // 复制数据
    memcpy(p_map, line, strlen(line));
    msync(p_map, strlen(line), MS_SYNC);

    close(fd);

end:
    munmap(p_map, PAGE_SIZE);
    return 0;
}
