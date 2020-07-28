#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEMO_DEV_NAME "/dev/demo_cdrv"

int main()
{
	char buffer[64] = {"abcdefghijklmn"};
	int fd;
	int ret = 0;

	fd = open(DEMO_DEV_NAME, O_RDWR);
	if (fd < 0)
	{
		ret = fd;
		printf("open device %s failed\n", DEMO_DEV_NAME);
		goto l_out;
	}

	ret = write(fd, buffer, 64);
	if (ret < 0)
	{
		printf("write device %s failed, rc(%d)\n", DEMO_DEV_NAME, ret);
		goto l_out;
	}

	close(fd);

	fd = open(DEMO_DEV_NAME, O_RDWR);
	if (fd < 0)
	{
		ret = fd;
		printf("open device %s failed\n", DEMO_DEV_NAME);
		goto l_out;
	}

	memset(buffer, 0, 64);
	ret = read(fd, buffer, 64);
	if (ret < 0)
	{
		printf("read device %s failed, rc(%d)\n", DEMO_DEV_NAME, ret);
		goto l_out;
	}
	printf("read device %s ok, buff(%s)\n", DEMO_DEV_NAME, buffer);

	close(fd);

l_out:
	return ret;
}
