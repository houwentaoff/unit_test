#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

int open_device(void)
{
#define DEV_PATH "/dev/ttyS1"
	int fd = 0;
	struct termios options;
	int ret = 0;

	fd = open(DEV_PATH,O_RDWR, 0);
	if (fd < 0) {
		printf("%s_%d:open %s failed\n", __FILE__, __LINE__, DEV_PATH);
		return -1;
	}

	/* 8bit, stop_bit-1 */
	options.c_cflag |= CS8;
	options.c_cflag &= ~PARENB;
	options.c_iflag &= ~INPCK;
	options.c_cflag &= ~CSTOPB;

	/* set raw mode */
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
	options.c_oflag &= ~OPOST;                          /*Output*/

	/* Don't treat <CR> as <LF>.*/
	options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
	options.c_oflag &= ~(ONLCR | OCRNL);

	tcflush(fd, TCIFLUSH);
	options.c_cc[VTIME] = 20; /* Set timeout 15 seconds*/
	options.c_cc[VMIN] = 0;    /* Update the options and do it NOW */

	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	ret = tcsetattr(fd, TCSANOW, &options);
	if (ret != 0) {
		printf("%s_%d:set options to  %s failed\n", __FILE__, __LINE__, DEV_PATH);
		return -1;
	}

	return fd;
}

void close_device(int fd)
{
	if (fd > 0)
		close(fd);
}

unsigned int check_sum(unsigned char *start, unsigned int len)
{
	int i = 0;
	unsigned int sum = 0;
	
	if (!start) {
		printf("%s_%d:invalid arguments, NULL\n", __FILE__, __LINE__);
		return -1;
	}
	
	for(i = 0; i < len; i++) {
		sum += *(start + i);		
	}
	sum = (sum&0xff);

	return sum;
}

static int  run_cmd = 1;
void *read_trhead(void *arg)
{
	int fd = *((int *)arg);
	char buf[10] = {0};
	int ret = 0;
	int i = 0;

	if (fd <= 0) {
		printf("%s_%d:fd is 0 , so exit read thread\n", __FILE__, __LINE__);
		goto exit;
	}

	while (run_cmd) {
		ret = read(fd, buf, sizeof(buf));
		if (ret < 0) {
			printf("%s_%d:ret < 0, so exit read thread\n", __FILE__, __LINE__);
			goto exit;
		}
		if (!ret) {
			//printf(".........read..........\n");
			continue;		
		}

		printf("read thread buf>>>>>>>>>>>>>>\n");
		for (i = 0; i < ret; i++) {
			printf("%2x	", buf[i]);
		}
		printf("\n");
	}

exit:
	return NULL;
}

int main_loop(int fd, pthread_t pthread_id)
{
	unsigned char cmd[10] = {0xff, 0x01, 0};
	int amount = 0;
	int ret = 0;
	int i = 0;
	int tmp = 0;

	if (fd <= 0) {
		printf("%s_%d:fd is 0 , so exit write thread\n", __FILE__, __LINE__);
		goto exit;
	}
	
	while (1) {
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = 0xff;
		cmd[1] = 0x01;
		printf("Input the amount of number you want to input(<=0 will exit):\n");
		ret = scanf("%d", &amount);
		if (ret != 1) {
			printf("%s_%d:scanf ret is %d , so exit write thread\n", __FILE__, __LINE__, ret);
			goto exit;	
		}
		
		if (amount <= 0) {
			printf("%s_%d:exit write thread\n", __FILE__, __LINE__);
			goto exit;
		}

		printf("input %d numbers:\n", amount);
		for (i = 0; i < amount; i++) {
			scanf("%x", &tmp); 
			cmd[2 + i] = tmp;
		}
		cmd[2+amount] = check_sum(&cmd[1], amount+1);
		printf("\n");
		for (i = 0; i < (3+amount); i++) {
			printf("%2x ", cmd[i]);
		}
		printf("\n");

		ret = write(fd, cmd, 3+amount);
		if (ret != (3+amount)) {
			printf("%s_%d: write failed, ret %d\n", __FILE__, __LINE__, ret);
			continue;	
		}
	}
	
exit:
	run_cmd = 0;
	pthread_join(pthread_id, NULL);
	return 0;
}

int main(int argc, char *argv[])
{
	int fd = 0;
	int ret = 0;
	pthread_t pth_id = 0;

	fd = open_device();
	if (fd < 0) {
		printf("%s_%d: open device failed, fd %d\n", __FILE__, __LINE__, fd);
		return -1;	
	}
	
	ret = pthread_create(&pth_id, NULL, read_trhead, &fd);
	if (ret != 0) {
		printf("%s_%d: create thread failed, ret %d\n", __FILE__, __LINE__, ret);
		goto CLOSE;
	}

	main_loop(fd, pth_id);

CLOSE:
	close_device(fd);
	return 0;
}

