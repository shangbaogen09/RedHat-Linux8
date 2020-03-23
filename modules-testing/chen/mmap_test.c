#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define	MAP_SIZE 4096
#define	USTR_DEF "String changed from the user space"

int main(int argc, char *argv[])
{
	int fd;
	char *pdata;

	if(argc <= 1)
	{
		printf("USAGE: main devfile pamapped\n");
		return 0;
	}

	fd = open(argv[1], O_RDWR | O_NDELAY);
	if(fd  >= 0)
	{
		pdata	= (char *)mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, strtoul(argv[2], 0, 16));
		printf("USERAddr = %p, DATA from kernel %s\n",pdata, pdata);
		printf("Writing a string to the kernel space...\n");
		strcpy(pdata, USTR_DEF);
		printf("Done\n");
		munmap(pdata, MAP_SIZE);
		close(fd);
	}

	return 0;
}
