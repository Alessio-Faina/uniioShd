#define __USE_LINUX_IOCTL_DEFS

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sys/ioctl.h>

#include <errno.h>
 
#define NIOCCONFIG	_IO('i', 0x950) /* for ext. modules */

int main()
{
	int fd = 5;
	void* test;
	char c;
	void* mem;
	int result=0;
	int status = 0;
	int i=0;
	void* res = (void*)malloc(4096);
	
	printf("PID: %i\n",getpid());
	
	fd = open("/proc/sys/DosDevices/Global/netmap", O_RDWR);
	if (fd>-1)
	{
		printf("OK, opened: %i\n",fd);
	}else{
		printf("NEIN\n");
		return;
	}
	
	/*printf("Press c to continue\n");
	while (c != 'c') {
	c = getchar();
    }
	c='n';*/
	
	/*printf("Trying MMAP...\n");
	mem = mmap(0, 1024, PROT_WRITE | PROT_READ, MAP_SHARED,fd, 0);
	
	printf("Mem addr 0x%p, errno: %i \n", mem, errno);*/
	
	/*printf("Trying IOCTL...\n");
	printf("Press c to continue\n");
	while (c != 'c') {
	c = getchar();
    }
	c='n';*/
	
	/*for (i=0; i<1000; i++)
	{
		printf("%i,",i);
		result = ioctl(fd, i);
		if (result>-1)
		{
			printf("IOCTL ok: %i... \n", i);
		}else{
			if (errno!=22)
			{
				printf("Errno: (%i);", errno);
			}
		}
	}
	*/
	result = ioctl(fd, NIOCCONFIG, NULL);
	printf("IOCTL result: %i... errno: %i \n", result, errno);
	
	result = read(fd, &result, 1);
	printf("read result: %i... errno: %i \n", result, errno);
	
	result = write(fd, &result, 1);
	printf("write result: %i... errno: %i \n", result, errno);
	
	/*printf("Press c to continue\n");
	while (c != 'c') {
	c = getchar();
    }
	c='n';*/
	
	printf("Closing...\n");
	close(fd);
}