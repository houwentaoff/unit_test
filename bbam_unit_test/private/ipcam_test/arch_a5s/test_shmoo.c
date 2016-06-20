/*
 * test_shmoo.c
 *
 * History:
 *	2010/11/30 - [Jian Tang] created file
 *
 * Copyright (C) 2008-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/*******************************************
 * Test Step :
 *  test mode - 0 is 720p 60 encoding
 *              1 is 1080p30 + 480p30 H.264 encoding
 *              2 is 1080p30 + 480p30 MJPEG encoding
 *
 *  dll - DLL 0 & DLL 1 register value
 *
 *  mask is 0. It modifies the byte 0.
 *      DLL 0 : 0x00 0a 0a XX
 *      DLL 1 : 0x00 0a 0a XX
 *
 *  mask is 1. It modifies the byte 1.
 *      DLL 0 : 0x00 0a XX ca
 *      DLL 1 : 0x00 0a XX ca
 *
 *  mask is 2. It modifies the byte 2.
 *      DLL 0 : 0x00 XX 0a ca
 *      DLL 1 : 0x00 XX 0a ca
 *
 *  lowbound - canter to minimal value
 *  highbound  - canter to maximal value
 *
 *  test time - time cost in each loop
 *
 *  step size - each step from lowbound to upbound
 *
 *  Example:
 *      test_shmoo 0 0x000a0aca 0 0x00 0x40 1 4
 *
 *  Test DLL 0 & DLL 1 from 0x000a0a00 to 0x000a0a40
 *
 *******************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

#include <getopt.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>

#include <linux/types.h>
#include <linux/watchdog.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "ambas_vin.h"
#include <amba_debug.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct shmoo_param_s {
	u32 test_mode;
	u32 dll;
	u32 mask;
	u32 lowbound;
	u32 highbound;
	u32 time;
	u32 step;
	u32 save_mode;
} shmoo_param_t;

typedef struct dll_sel_range_s {
	u32 min;
	u32 max;
} dll_sel_range_t;

typedef struct tcp_read_req_s {
	int id;
	int size;
	unsigned long long offset;
} tcp_read_req_t;

typedef struct tcp_ack_s {
	int result;
	int dummy;
	unsigned long long size;
} tcp_ack_t;


static const char * HOST_IP_ADDR = "10.0.0.1";
static const char * DEFAULT_DIRNAME = "shmoo_test";

#define TCP_PORT_BASE		(2016)
#define TCP_PORT_NUM		(16)
#define CREATE_PORT			(TCP_PORT_BASE + 0)
#define WRITE_PORT			(TCP_PORT_BASE + 2)
#define READ_PORT			(TCP_PORT_BASE + 4)
#define CHECK_PORT			(TCP_PORT_BASE + 6)
#define GET_FILE_PORT		(TCP_PORT_BASE + 8)

#define BUF_SIZE				(256)

#define TEST_STEP_SIZE		(8)
#define TEST_TIMES			(1)
#define TEST_SECONDS		(1)

#define PAGE_SHIFT			(12)
#define DLL0_ADDR			(0x70170090)
#define DLL1_ADDR			(0x70170094)
#define DLL_MAP_START		((DLL0_ADDR >> PAGE_SHIFT) << PAGE_SHIFT)
#define DLL_REG_SIZE		(4)

static dll_sel_range_t dll_range[3] = {
	{ .min = 0x00, .max = 0x40 },			// default valid range: 0x00 ~ 0x40
	{ .min = 0x00, .max = 0xA0 },			// default valid range: 0x00 ~ 0xA0
	{ .min = 0x00, .max = 0x40 }			// default valid range: 0x00 ~ 0x40
};
static struct amba_debug_mem_info mem_info;
static char *debug_mem = NULL;

static int digitvalue(char isdigit)
{
	if (isdigit >= '0' && isdigit <= '9')
		return isdigit - '0';
	else
		return -1;
}

static int xdigitvalue(char isdigit)
{
	if (isdigit >= '0' && isdigit <= '9')
		return isdigit - '0';
	if (isdigit >= 'a' && isdigit <= 'f')
		return 10 + isdigit - 'a';
	if (isdigit >= 'A' && isdigit <= 'F')
		return 10 + isdigit - 'A';
	return -1;
}

static int strtou32(const char *str, u32 *value)
{
	int i;
	*value = 0;

	if (strncmp(str, "0x", 2) == 0) {
		/* hexadecimal mode */
		str += 2;
		while (*str != '\0') {
			if ((i = xdigitvalue(*str)) < 0)
				return -1;
			*value = (*value << 4) | (u32)i;
			++str;
		}
	} else {
		/* decimal mode */
		while (*str != '\0') {
			if ((i = digitvalue(*str)) < 0)
				return -1;
			*value = (*value * 10) + (u32)i;
			++str;
		}
	}
	return 0;
}

static int hog_cpu(u32 sec)
{
	int counter = 0;
	long start_time, curr_time;

	long long do_timeout = sec * TEST_TIMES;
	long long timeout = 0;

	if ((start_time = time(NULL)) == -1) {
		perror("Failed to acquire current time!\n");
		exit(1);
	}
	srand(time(NULL));
	if (do_timeout) {
		 do {
			++counter;
			sqrt(rand());
			if ((curr_time = time(NULL)) == -1) {
				perror("Failed to acquire current time!\n");
				exit(1);
			}
			timeout = do_timeout - (curr_time - start_time);
		} while (timeout);
	}
	return 0;
}

/* calculate the SQRT of a random number for "sec * TEST_TIMES" seconds */
static int do_arm_test(u32 sec)
{
	int pid, status;
	if ((pid = fork()) == 0) {
		exit(hog_cpu(sec));
	}
	if ((pid = wait(&status)) > 0) {
		printf("ARM CPU test signalled normally!\n");
	} else {
		perror("Waiting for ARM test!\n");
	}
	return 0;
}

static int read_vdsp_int(void)
{
	int fd, retv;
	u32 value;
	char buf[BUF_SIZE], file[BUF_SIZE];
	char *pstart, *pend;

	sprintf(file, "/tmp/vdsp_int");
	sprintf(buf, "grep vdsp /proc/interrupts | awk \'{print $2}\' > %s ", file);
	system(buf);
	if ((fd = open(file, O_RDWR, 0777)) < 0) {
		printf(" ### Error! Can't open file [%s].\n", file);
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	retv = read(fd, buf, BUF_SIZE-1);
	buf[retv] = '\0';
	pstart = buf;
	while (*pstart == ' ') {
		++pstart;
	}
	pend = pstart;
	while ((*pend != ' ' ) && (*pend != '\n')) {
		++pend;
	}
	*pend = '\0';

	strtou32(pstart, &value);
	close(fd);
	return value;
}

static int check_dsp_state(u32 sec)
{
	const int FRAMERATE = 30;
	int i;
	u32 start, stop, delta;

	start = stop = 0;
	if ((start = read_vdsp_int()) < 0)
		return -1;
	for (i = 0; i < sec; ++i) {
		sleep(1);
		if ((stop = read_vdsp_int()) < 0)
			return -1;
		delta = stop - start;
		start = stop;
		if (delta < FRAMERATE) {
			printf(" ### DSP Crash !!!\n");
			return -1;
		}
	}
	return 0;
}

// test_mode = 0, need sensor to support 720P60 mode
static int do_h264_hfps_enc_720p(u32 seconds)
{
	char cmd[BUF_SIZE];

	sprintf(cmd, "/usr/local/bin/test_encode -i720p -F60 -V480p --hdmi -X --bsize 720p -A -h720p -b0 -e \n");
	system(cmd);
	if (check_dsp_state(seconds) < 0)
		return -1;
	sprintf(cmd, "/usr/local/bin/test_encode -A -s \n");
	system(cmd);
	if (check_dsp_state(1) < 0)
		return -1;
	return 0;
}

// test_mode = 1
static int do_h264_dual_stream(u32 seconds)
{
	char cmd[BUF_SIZE];

	sprintf(cmd, "/usr/local/bin/test_encode -i1080p -V480p --hdmi -A -h1080p -b0 -e -B -h480p -b1 -e \n");
	system(cmd);
	if (check_dsp_state(seconds) < 0)
		return -1;
	sprintf(cmd, "/usr/local/bin/test_encode -A -s -B -s \n");
	system(cmd);
	if (check_dsp_state(1) < 0)
		return -1;
	return 0;
}

// test_mode = 2
static int do_mjpeg_dual_stream(u32 seconds)
{
	char cmd[BUF_SIZE];

	sprintf(cmd, "/usr/local/bin/test_encode -i1080p -V480p --hdmi -A -m1080p -b0 -e -B -m480p -b1 -e \n");
	system(cmd);
	if (check_dsp_state(seconds) < 0)
		return -1;
	sprintf(cmd, "/usr/local/bin/test_encode -A -s -B -s \n");
	system(cmd);
	if (check_dsp_state(1) < 0)
		return -1;
	return 0;
}

// test_mode = 3
static int do_nothing(u32 seconds)
{
	return 0;
}

static int recv_buffer(int sock_in, char *buffer, int len)
{
	while (len > 0) {
		int r = recv(sock_in, buffer, len, MSG_WAITALL);
		if (r < 0) {
			return -1;
		}
		if (r == 0) {
			return 0;
		}
		len -= r;
	}
	return len;
}

static int send_buffer(int sock_in, char *buffer, int len)
{
	int retv;
	retv = send(sock_in, buffer, len, MSG_WAITALL);
	if (retv <= 0) {
		perror("send error");
		return -1;
	}
	return 0;
}

static int create_file_tcp(const char *name)
{
	int sock;
	int tcp_port;
	struct sockaddr_in sa;
	char fname[BUF_SIZE];
	const char *host_ip_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Sock error!\n");
		return -1;
	}
	tcp_port = CREATE_PORT;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(tcp_port);
	host_ip_addr = HOST_IP_ADDR;
	sa.sin_addr.s_addr = inet_addr(host_ip_addr);
	if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("Connect error!\n");
		return -1;
	}
	strcpy(fname, name);
	if (send(sock, fname, sizeof(fname), MSG_NOSIGNAL) < 0) {
		perror("Send error!\n");
		return -1;
	}
	return sock;
}

static int get_file_tcp(const char *srcname, const char *dstname)
{
	int sock, fd;
	int tcp_port;
	struct sockaddr_in sa;
	char fname[BUF_SIZE];
	const char *host_ip_addr;
	tcp_read_req_t req;
	tcp_ack_t ack;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("sock error!\n");
		return -1;
	}

	tcp_port = GET_FILE_PORT;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(tcp_port);
	host_ip_addr = HOST_IP_ADDR;
	sa.sin_addr.s_addr = inet_addr(host_ip_addr);

	if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("connect error!\n");
		return -1;
	}
	if ((fd = open(dstname, O_RDWR |O_CREAT |O_TRUNC, 0)) < 0) {
		perror("file open() error!\n");
		return -1;
	}
	/* transfer the file name */
	sprintf(fname,"%s:r", srcname);
	if (send(sock, fname, sizeof(fname), MSG_NOSIGNAL) < 0) {
		perror("send error!\n");
		return -1;
	}
	/* get the file size from server */
	req.offset = 0;
	req.size = 0;
	if (send_buffer(sock, (char *)&req, sizeof(req)) < 0)
		return -1;
	if (recv_buffer(sock, (char *)&ack, sizeof(ack)) < 0)
		return -1;
	/* get the file from server */
	while (ack.size > 0) {
		req.size = ack.size;
		if (req.size > BUF_SIZE)
			req.size = BUF_SIZE;
		ack.size -= req.size;
		if (send_buffer(sock, (char *)&req, sizeof(req)) < 0)
			return -1;
		if (recv_buffer(sock, fname, req.size) < 0)
			return -1;
		write(fd, fname, req.size);
		req.offset += req.size;
	}
	close(fd);
	close(sock);

	return 0;
}
static int check_file_exist_tcp(const char *name)
{
	int sock;
	int file_exist_flag;
	static int tcp_port = CHECK_PORT;
	struct sockaddr_in sa;
	char fname[BUF_SIZE];
	const char * host_ip_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("sock error!\n");
		return -1;
	}
	tcp_port = (CHECK_PORT == tcp_port) ? (tcp_port + 1) : (tcp_port - 1);
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(tcp_port);
	host_ip_addr = HOST_IP_ADDR;
	sa.sin_addr.s_addr = inet_addr(host_ip_addr);
	if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("connect error!\n");
		return -1;
	}
	sprintf(fname, "%s:e", name);
	if (send(sock, fname, sizeof(fname), MSG_NOSIGNAL) < 0) {
		perror("send error!\n");
		return -1;
	}
	if (recv_buffer(sock, (char *)&file_exist_flag, sizeof(file_exist_flag)) < 0) {
		perror("recv error!\n");
		return -1;
	}
	close(sock);
	return file_exist_flag;
}

static int create_file_local(const char *name)
{
	int fd;
	if ((fd = open(name, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(name);
		return -1;
	}
	close(fd); //make sure exists
	if ((fd = open(name, O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(name);
		return -1;
	}
	return fd;

}

static int open_file_local(const char *name)
{
	int fd;
	if ((fd = open(name, O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(name);
		return -1;
	}
	return fd;
}

static int check_file_exist_local(const char *name)
{
	int fd;
	if ((fd = open(name, O_RDONLY, 0)) < 0)
	{
		printf("check: %s ....not found \n",name);
		return -1;
	}
	close(fd);
	return 1;
}

static void usage(void)
{
	printf("test_shmoo step: \n Usage: [test mode] [dll] [mask] [lowbound] [highbound] [seconds] [step] [save mode]\n");
	printf("\ttest mode is 0 - H.264 720p60 encoding\n"
		"\t             1 - H.264 Dual H.264 1080p30 + 480p30 encoding %d seconds in default\n"
		"\t             2 - H.264 Dual MJPEG 1080p30 + 480p30 encoding %d seconds in default\n"
		"\t             else - Nothing\n"
		"\tdll - DLL register value\n"
		"\tmask is 0 - mask dll low adj\n"
		"\t        1 - mask dll middle adj\n"
		"\t        2 - mask dll high adj\n"
		"\tlowbound - canter to minimal value\n"
		"\thighbound - canter to maximal value\n"
		"\tseconds - testing seconds in each loop, minimal value is %d\n"
		"\tstep - step size from lowbound to highbound\n"
		"\tsave_mode - 0 by network, 1 save to nand:/root\n"
		" Example:\n"
		"\ttest_shmoo 0 0x000a0aca 0 0x40 0xff %d %d 0\n"
		"\tTest DLL 0 and DLL 1 from 0x000a0a40 to 0x000a0aff in step of %d\n",
		TEST_SECONDS, TEST_SECONDS, TEST_SECONDS,
		TEST_SECONDS, TEST_STEP_SIZE, TEST_STEP_SIZE);

	printf("\ntest_shmoo default: \n Usage: --default\n"
		"\t\n");
}

static int init_param(int argc, char **argv, shmoo_param_t *ts)
{
	if (9 == argc) {
		printf("======++=== test_shmoo %s %s %s %s %s %s %s %s\n",
			argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
		strtou32(argv[1], &ts->test_mode);
		strtou32(argv[2], &ts->dll);
		strtou32(argv[3], &ts->mask);
		strtou32(argv[4], &ts->lowbound);
		strtou32(argv[5], &ts->highbound);
		strtou32(argv[6], &ts->time);
		strtou32(argv[7], &ts->step);
		strtou32(argv[8], &ts->save_mode);
		if (ts->lowbound < dll_range[ts->mask].min)
			ts->lowbound = dll_range[ts->mask].min;
		if (ts->lowbound > dll_range[ts->mask].max)
			ts->lowbound = dll_range[ts->mask].max - 2;
		if (ts->highbound > dll_range[ts->mask].max)
			ts->highbound = dll_range[ts->mask].max;
		if (ts->highbound < ts->lowbound)
			ts->highbound = ts->lowbound;
		return ts->mask;
	} else {
		return -1;
	}
}

static int fill_data(char *pmem, u32 start, u32 size_limit,
				u32 address, u32 size, u32 data)
{
	u32 offset, i;
	if (address & 0x03) {
		printf("address[0x%08x] will be alined to 0x%08x!\n", address, (address & 0xfffffffc));
		address &= 0xfffffffc;
	}
	offset = address - start;
	if (offset >= size_limit) {
		printf("Offset is too big! 0x%x < 0x%x.\n", size_limit, offset);
		return -1;
	}
	if ((offset + size) >= size_limit) {
		printf("Size is too big! 0x%x < 0x%x.\n", size_limit, (offset + size));
		size = size_limit - offset;
	}
	pmem += offset;
	for (i = 0; i < size; i += 4) {
		*(u32 *)(pmem + i) = data;
	}
	return 0;
}

static int dram_dll_map(void)
{
	int fd;
	int debug_flag, map_size = 0;

	printf("============= DRAM_DLL_MAP =========== \n");
	if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		return -1;
	}
	if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
		perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
		return -1;
	}
	if (ioctl(fd, AMBA_DEBUG_IOC_GET_MEM_INFO, &mem_info) < 0) {
		perror("AMBA_DEBUG_IOC_GET_MEM_INFO");
		return -1;
	}
	map_size = DLL0_ADDR - DLL_MAP_START + 2 * DLL_REG_SIZE;
	debug_mem = (char *)mmap(NULL, map_size,
  		PROT_READ | PROT_WRITE, MAP_SHARED, fd, DLL_MAP_START);
	if ((int)debug_mem < 0) {
		perror("dram dll map !\n");
		return -1;
	}
	close(fd);
	return 0;
}

void dram_dll_set(u32 dll)
{
	printf("============= Set DLL : 0x%08x_0x%08x. \n",dll, dll);
	sleep(1);
	fill_data(debug_mem, DLL_MAP_START, 0xffffffff,
			DLL0_ADDR, 2 * DLL_REG_SIZE, dll);
}

static void shmoo_test_step(shmoo_param_t *ts, char *dir)
{
	int sockfd;
	int retv, remap_flag = 0;

	u32 i, j;
	char full_name[BUF_SIZE], file_name[BUF_SIZE];
	u32 dll = ts->dll;

	printf("---------------- Shmooing Test Step %d Begin ---------------- \n",
		ts->mask);
	/* start test */
	printf("show dir:%s \n",dir);
	for (i = ts->lowbound; i <= ts->highbound; i += ts->step) {
		switch (ts->mask) {
		case 0:
			dll &= 0xffffff00;
			dll |= (i & 0xff);
			break;
		case 1:
			dll &= 0xffff00ff;
			dll |= ((i & 0xff) << 8);
			break;
		case 2:
			dll &= 0xff00ffff;
			dll |= ((i & 0xff) << 16);
			break;
		}
		sprintf(full_name, "%s/0x%08x_0x%08x", dir, dll, dll);
		printf("show full_name:%s \n",full_name);

		/* check file exist in server ? */
		if (ts->save_mode == 1)
			retv = check_file_exist_local(full_name);
		else
			retv = check_file_exist_tcp(full_name);
		if (retv < 0) {
			if (ts->save_mode == 1)
				sockfd = create_file_local(full_name);
			else
				sockfd = create_file_tcp(full_name);
			if (sockfd < 0) {
				printf("Cannot create file %s in server! Net Error!\n", full_name);
			} else {
				close(sockfd);
			}
		} else {
			continue;
		}

		if (remap_flag == 0) {
			/* remap the dll value */
			retv = dram_dll_map();
			if (retv < 0)
				exit(1);
			remap_flag = 1;
		}

		/* reset the dll value */
		dram_dll_set(dll);

		/* run ARM testing */
		retv = do_arm_test(ts->time);
		if (retv < 0) {
			printf("ARM test failed!\n");
			goto done;
		} else {
			printf("<+>");
		}
		/* create arm success file in server. */
		sprintf(file_name, "%s_%d_arm_success", full_name, ts->mask);
		if (ts->save_mode == 1)
			sockfd = create_file_local(file_name);
		else
			sockfd = create_file_tcp(file_name);
		if (sockfd < 0) {
			printf("Cannot write file %s in server!\n", file_name);
		} else {
			close(sockfd);
		}

		for (j = 0; j < TEST_TIMES; ++j) {
			switch (ts->test_mode) {
			case 0:
				retv = do_h264_hfps_enc_720p(ts->time);
				break;
			case 1:
				retv = do_h264_dual_stream(ts->time);
				break;
			case 2:
				retv = do_mjpeg_dual_stream(ts->time);
				break;
			default:
				retv = do_nothing(ts->time);
				break;
			}
			if (retv < 0) {
				printf("DSP test failed!\n");
				system("reboot");
				goto done;
			} else {
				printf("+");
			}
		}
		printf("...success!\n");

		/* create dsp success file in server */
		sprintf(file_name, "%s_%d_dsp_success", full_name, ts->mask);
		if (ts->save_mode == 1 )
			sockfd =create_file_local(file_name);
		else
			sockfd = create_file_tcp(file_name);
		if (sockfd < 0) {
			printf("Cannot write file %s in server!\n", file_name);
		} else {
			close(sockfd);
		}

done:
		/* finish test */
		do { } while (0);
	}
}

static int shmoo_test_analyze(shmoo_param_t *ts, char *dir, char *file_name)
{
	u32 dll, next_dll, i;
	u32 min_su, max_su, result, next_low, next_high;
	int sockfd, retv, retv1;
	char full_name[BUF_SIZE], input[BUF_SIZE];
	min_su = max_su = result = 0;
	dll = ts->dll;

	printf("---------------- Shmooing Test Step %d Analyze ---------------- \n",
		ts->mask);
	sprintf(full_name, "%s/%s_%d_analyze.txt", DEFAULT_DIRNAME,
		file_name, ts->mask);
	/* create analyze file in server. */
	if (ts->save_mode == 1 )
	{
		sockfd = create_file_local(full_name);
		close(sockfd);
	}
	else
		sockfd = create_file_tcp(full_name);
	if (sockfd < 0) {
		printf("Cannot write file %s in server!\n", full_name);
	}

	for (i = ts->lowbound; i <= ts->highbound; i += ts->step) {
		switch (ts->mask) {
		case 0:
			dll &= 0xffffff00;
			dll |= (i & 0xff);
			break;
		case 1:
			dll &= 0xffff00ff;
			dll |= ((i & 0xff) << 8);
			break;
		case 2:
			dll &= 0xff00ffff;
			dll |= ((i & 0xff) << 16);
			break;
		}

		sprintf(full_name, "%s/0x%08x_0x%08x_%d_arm_success",dir, dll, dll, ts->mask);
		if (ts->save_mode == 1)
			retv = check_file_exist_local(full_name);
		else
			retv = check_file_exist_tcp(full_name);
		if (retv < 0) {
			sprintf(input, "dll0: 0x%08x, dll1: 0x%08x...............", dll, dll);
		} else {
			sprintf(input, "dll0: 0x%08x, dll1: 0x%08x....arm_success", dll, dll);
		}

		if ( ts->save_mode == 1 )
		{
			sprintf(full_name, "%s/%s_%d_analyze.txt", DEFAULT_DIRNAME,file_name, ts->mask);
			sockfd = open_file_local(full_name);
			if (sockfd < 0)
			{
				printf("Cannot write file %s in server!\n", full_name);
				return -1;
			}
		}
		if ((write(sockfd, input, strlen(input))) != strlen(input)) {
			printf("Cannot write file %s in server!\n", full_name);
		}
		if ( ts->save_mode == 1)
			close(sockfd);

		sprintf(full_name, "%s/0x%08x_0x%08x_%d_dsp_success", dir, dll, dll, ts->mask);
		if (ts->save_mode == 1)
			retv = check_file_exist_local(full_name);
		else
			retv = check_file_exist_tcp(full_name);
		if (retv < 0) {
			sprintf(input, "..............\r\n");
		} else {
			sprintf(input, "...dsp_success\r\n");
		}

		if (ts->save_mode == 1)
		{
			sprintf(full_name, "%s/%s_%d_analyze.txt", DEFAULT_DIRNAME,file_name, ts->mask);
			sockfd = open_file_local(full_name);
			if (sockfd < 0) {
				printf("Cannot write file %s in server!\n", full_name);
				return -1;
			}
		}
		if ((write(sockfd, input, strlen(input))) != strlen(input)) {
			printf("Cannot write file %s in server!\n", full_name);
		}
		if (ts->save_mode == 1)
			close(sockfd);
	}

	if (ts->save_mode == 0)
		close(sockfd);
	/* create next step bash file */
	sprintf(full_name, "%s/step%d.txt", DEFAULT_DIRNAME, ts->mask + 1);
	if (ts->save_mode == 1)
		sockfd = create_file_local(full_name);
	else
		sockfd = create_file_tcp(full_name);
	if (sockfd < 0) {
		printf("Cannot write file %s in server!\n", full_name);
		return -1;
	}
	else if (ts->save_mode == 1)
		close(sockfd);
	for (i = ts->lowbound; i <= ts->highbound; i += ts->step) {
		switch (ts->mask) {
		case 0:
			dll &= 0xffffff00;
			dll |= (i & 0xff);
			next_dll = dll | ((i + ts->step) & 0xff);
			break;
		case 1:
			dll &= 0xffff00ff;
			dll |= ((i & 0xff) << 8);
			next_dll = dll | (((i + ts->step) & 0xff) << 8);
			break;
		case 2:
			dll &= 0xff00ffff;
			dll |= ((i & 0xff) << 16);
			next_dll = dll | (((i + ts->step) & 0xff) << 16);
			break;
		default:
			next_dll = 0;
			break;
		}
		sprintf(full_name, "%s/0x%08x_0x%08x_%d_dsp_success", dir, dll, dll, ts->mask);
		if (ts->save_mode == 1)
			retv = check_file_exist_local(full_name);
		else
			retv = check_file_exist_tcp(full_name);
		sprintf(full_name, "%s/0x%08x_0x%08x_%d_dsp_success", dir, next_dll, next_dll, ts->mask);
		if (ts->save_mode == 1)
			retv1 = check_file_exist_local(full_name);
		else
			retv1 = check_file_exist_tcp(full_name);
		if (retv > 0) {
			if (min_su == 0)
				max_su = min_su = i;
			if (retv1 > 0)
				max_su = i;
		}
	}
	if ((max_su - min_su) < 0) {
		printf("\n !!! Shmoo Test Failed !!!\n");
		if (ts->save_mode == 0)
			close(sockfd);
		exit(1);
	} else {
		result = min_su + (max_su - min_su) / 2;
		switch (ts->mask) {
		case 0:
			dll &= 0xffffff00;
			dll |= (result & 0xff);
			next_low = dll_range[1].min;
			next_high = dll_range[1].max;
			break;
		case 1:
			dll &= 0xffff00ff;
			dll |= ((result & 0xff) << 8);
			next_low = dll_range[2].min;
			next_high = dll_range[2].max;
			break;
		case 2:
			dll &= 0xff00ffff;
			dll |= ((result & 0xff) << 16);
			next_low = dll_range[0].min;
			next_high = dll_range[0].max;
			break;
		default:
			next_low = 0;
			next_high = 0;
			break;
		}
	}
	if (ts->save_mode == 1)
	{
		sprintf(full_name, "%s/step%d.txt", DEFAULT_DIRNAME, ts->mask + 1);
		sockfd = open_file_local(full_name);
		if (sockfd < 0) {
			printf("Cannot write file %s in server!\n", full_name);
			return -1;
		}
	}
	sprintf(input,
		"#########################\r\n"
		"# ! /bin/sh  \r\n"
		"#########################\r\n");
	write(sockfd, input, strlen(input));
	sprintf(input,
		"/usr/local/bin/test_shmoo %d 0x%08x %d 0x%x 0x%x %d %d %d \n",
		ts->test_mode, dll, ts->mask+1,
		next_low, next_high, ts->time, ts->step, ts->save_mode);
	write(sockfd, input, strlen(input));
	close(sockfd);
	printf("---------------- Shmooing Test Step %d End ---------------- \n", ts->mask);
	system(input);
	return 0;
}

int main(int argc, char **argv)
{
	int sockfd,fd;
	int init = 0;
	int retv;
	char full_name[BUF_SIZE], file_name[BUF_SIZE], input[BUF_SIZE], dir[BUF_SIZE];
	u32 dll;
	shmoo_param_t ts = {0};
//	char tmppath[BUF_SIZE];

	if (2 == argc) {
		sprintf(input, "--default");
		sprintf(file_name, "test_shmoo.txt");
		sprintf(full_name, "/tmp/%s/step0.sh", DEFAULT_DIRNAME);

		retv = chdir("/tmp");
		retv = mkdir(DEFAULT_DIRNAME, O_RDWR);
		sprintf(dir, "%s/shmoo", DEFAULT_DIRNAME);
		retv = mkdir(dir, O_RDWR);

		if (strcmp(argv[1], input) == 0) {
			if (get_file_tcp(file_name, full_name) < 0) {
				printf("Error! Cannot get test file [%s] from tcp! Shmooing test failed!\n", file_name);
				return -1;
			}
			if ((fd = open(full_name, O_RDONLY, 0)) < 0) {
				printf("Cannot open test file [%s]! Shmooing test failed!\n", full_name);
				return -1;
			}
			read(fd, input, BUF_SIZE);
			close(fd);
			ts.save_mode = 0;
			sprintf(input, "%s %d",input, ts.save_mode);
			printf("shmoo: %s \n",input);
			system(input);
		}
		exit(1);
	} else {
		sprintf(file_name, "shmoo_test_step");
		init = init_param(argc, argv, &ts);

		if (ts.save_mode == 1)
		{
			retv = chdir("/root");
			retv = mkdir(DEFAULT_DIRNAME, O_RDWR);
			sprintf(dir, "%s/shmoo", DEFAULT_DIRNAME);
			retv = mkdir(dir, O_RDWR);
		}
		else
		{
			retv = chdir("/tmp");
			retv = mkdir(DEFAULT_DIRNAME, O_RDWR);
			sprintf(dir, "%s/shmoo", DEFAULT_DIRNAME);
			retv = mkdir(dir, O_RDWR);
		}
	}

	dll = ts.dll;
	if (init < 0) {
		usage();
	} else if (init < 3) {
		printf("shmoo: shmoo_test_step %d \n",dll);
		shmoo_test_step(&ts, dir);
		shmoo_test_analyze(&ts, dir, file_name);
	} else {
		/* set the best dll value */
		dram_dll_map();
		dram_dll_set(dll);
		sprintf(full_name, "%s/0x%08x_final_result.txt", DEFAULT_DIRNAME, dll);
		if (ts.save_mode == 1)
			sockfd = create_file_local(full_name);
		else
			sockfd = create_file_tcp(full_name);
		if (sockfd < 0) {
			printf("Cannot write file %s in server!\n", full_name);
		}
		retv = write(sockfd, full_name, strlen(full_name));
		if (retv != strlen(full_name)) {
			printf("Cannot write file %s in server!\n", full_name);
		}
		close(sockfd);
	}

	return retv;
}


#define __END_OF_FILE__

