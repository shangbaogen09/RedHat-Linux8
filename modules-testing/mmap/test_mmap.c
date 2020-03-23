#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
 
int main( void )
{
	int fd;
	char *buffer;
	char *mapBuf;
	fd = open("/dev/mmap_driver", O_RDWR);
	if(fd<0)
	{
		printf("open device is error,fd = %d\n",fd);
		return -1;
	}
	printf("before mmap\n");
	sleep(15);
	buffer = (char *)malloc(1024);
	memset(buffer, 0, 1024);
	mapBuf = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	printf("after mmap\n");
	sleep(15);
	
	strcpy(mapBuf, "Driver Test");
	memset(buffer, 0, 1024);
	strcpy(buffer, mapBuf);
	printf("buf = %s\n", buffer);
	
	
	munmap(mapBuf, 1024);
	free(buffer);
	close(fd);
	return 0;
}
