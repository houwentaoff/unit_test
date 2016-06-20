/*
 * test_ptz.c
 *
 * History:
 *	2011/7/20 - [George Chen] created file
 *
 * Copyright (C) 2007-2011, GM-Innovation, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of GM-Innovation, Inc.
 *
 */
#include "ptz.h"
#include "basetypes.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <linux/ioctl.h>	 

#include <getopt.h>
#include <sched.h>
#include <time.h>
#include <termios.h> 

#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#define NO_ARG		0
#define HAS_ARG		1

typedef struct hint_s {
	const char *arg;
	const char *str;
} hint_t;


// Init parameter
static struct option long_options[] = {
	{"help", NO_ARG, 0, 'h'},
	{"cfg-file", HAS_ARG, 0, 'c'},
	{"turn up", HAS_ARG, 0, 'u'},
	{"turn down", HAS_ARG, 0, 'd'},
	{"turn left", HAS_ARG, 0, 'l'},
	{"turn left up", HAS_ARG, 0, 'L'},
	{"turn left down", HAS_ARG, 0, 'M'},
	{"turn right", HAS_ARG, 0, 'r'},
	{"turn right up", HAS_ARG, 0, 'R'},
	{"turn right down", HAS_ARG, 0, 'N'},
	{"test eight direction", HAS_ARG, 0, 'E'},
	{"zoom wide", HAS_ARG, 0, 'w'},
	{"zoom tele", HAS_ARG, 0, 't'},
	{"focus near", HAS_ARG, 0, 'n'},
	{"focus far", HAS_ARG, 0, 'f'},
	{"baudrate setting", HAS_ARG, 0, 'b'},
	{"pt speed setting", HAS_ARG, 0, 's'},
	{"zoom/focus speed setting", HAS_ARG, 0, 'S'},
	{"control gpio select", HAS_ARG, 0, 'p'},
	
	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\tshow usage"},
	{"file", "specify configuration file"},
	{"up step", "specify up steps"},
	{"down step", "specify down steps"},
	{"left step", "specify left steps"},
	{"left up step", "specify left up  steps"},
	{"left down step", "specify left down steps"},
	{"right step", "specify right steps"},
	{"right up step", "specify right up  steps"},
	{"right down step", "specify right down steps"},
	{"test eight direction", "test eight direction on PTZ"},
	{"zoom wide", "zoom wide"},
	{"zoom tele", "zoom tele"},
	{"focus near", "focus near"},
	{"focus far", "focus far"},
	{"baudrate", "specify the baudrate"},
	{"pt speed", "specify the pt speed"},
	{"zoom/focus speed", "specify the zoom/focus speed"},
	{"port", "specify the gpio"},
	
	{0, 0},
};

static const char *short_options = "hc:u:d:l:L:M:r:R:N:E:w:t:n:f:b:s:S:p:";
	 

//#define DEBUG_PELCO
#ifdef DEBUG_PELCO
#define dprintf printf
#else
#define dprintf 
#endif


FILE *output;
int fd_uart1;

static int cfg_file_flag = 0;
static char cfg_filename[256];
static char ptzCmd = 0;
static int ptzStep = 0;
static int ptzSpeedPan = 0x3c,ptzSpeedTilt = 0x3c,ptzSpeed = 0;
static int ptzBaudrate = 0;
static int ptzPort = 31;

void test_ptz_up(int times)
{
	PelcoD_cmd(PELCO_CMD_UP,NULL);
	sleep(times);
	PelcoD_cmd(PELCO_CMD_STOP,NULL);	
}

void test_ptz_down(int times)
{
	PelcoD_cmd(PELCO_CMD_DOWN,NULL);	
	sleep(times);
	PelcoD_cmd(PELCO_CMD_STOP,NULL);	
}

void test_ptz_left(int times)
{
	PelcoD_cmd(PELCO_CMD_LEFT,NULL);	
	sleep(times);
	PelcoD_cmd(PELCO_CMD_STOP,NULL);	
}

void test_ptz_left_up(int times)
{
	PelcoD_cmd(PELCO_CMD_UP,NULL);
	PelcoD_cmd(PELCO_CMD_LEFT,NULL);	
	sleep(times);
	PelcoD_cmd(PELCO_CMD_STOP,NULL);	
}

void test_ptz_left_down(int times)
{
	PelcoD_cmd(PELCO_CMD_DOWN,NULL);	
	PelcoD_cmd(PELCO_CMD_LEFT,NULL);	
	sleep(times);
	PelcoD_cmd(PELCO_CMD_STOP,NULL);	
}

void test_ptz_right(int times)
{
	PelcoD_cmd(PELCO_CMD_RIGHT,NULL);	
	sleep(times);
	PelcoD_cmd(PELCO_CMD_STOP,NULL);	
}

void test_ptz_right_up(int times)
{
	PelcoD_cmd(PELCO_CMD_UP,NULL);
	PelcoD_cmd(PELCO_CMD_RIGHT,NULL);	
	sleep(times);
	PelcoD_cmd(PELCO_CMD_STOP,NULL);	
}

void test_ptz_right_down(int times)
{
	PelcoD_cmd(PELCO_CMD_DOWN,NULL);	
	PelcoD_cmd(PELCO_CMD_RIGHT,NULL);	
	sleep(times);
	PelcoD_cmd(PELCO_CMD_STOP,NULL);	
}

void test_ptz_eight_directions(int times)
{
	printf("Test Ptz Left\n");
	test_ptz_left(times);
	usleep(100000);
	printf("Test Ptz Right\n");
	test_ptz_right(times);
	usleep(200000);
	printf("Test Ptz UP\n");
	test_ptz_up(times);
	usleep(100000);
	printf("Test Ptz Down\n");
	test_ptz_down(times);
#if 0
	printf("Test Ptz Left Up\n");
	test_ptz_left_up(times);
	printf("Test Ptz Right Down\n");
	test_ptz_right_down(times);
	printf("Test Ptz Right Up\n");
	test_ptz_right_up(times);
	printf("Test Ptz Left Down\n");
	test_ptz_left_down(times);
#endif
}

void test_ptz_speed()
{
	PelcoD_set_speed(ptzSpeedPan,ptzSpeedTilt);
}

void usage(void)
{
	int i;
	printf("test_ptz usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	ptzSpeed = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'h':
			usage();
			exit(0);
		case 'f':
			strcpy(cfg_filename, optarg);
			cfg_file_flag = 1;
			break;
		case 'u':
		case 'd':
		case 'l':
		case 'L':
		case 'M':
		case 'r':
		case 'R':
		case 'N':
		case 'E':
			ptzCmd = ch;
			ptzStep = atoi(optarg);
			break;
		case 'b':
			ptzBaudrate = atoi(optarg);
			break;
		case 's':
			ptzSpeed = 1;
			ptzSpeedPan = atoi(optarg);
			ptzSpeedTilt = ptzSpeedPan;
			break;
		case 'p':
			ptzPort = atoi(optarg);
			break;
		default:
			printf("Unknown option found : %c\n", ch);
			return -1;
		}
	}
	printf("cmd:test_ptz -b %d -s %d -p %d -%c %d\n",ptzBaudrate,ptzSpeedPan,ptzPort,ptzCmd,ptzStep);
	return 0;
}

static void sigstop(int signo)
{
	exit(1);
}

int main(int argc, char **argv)
{
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	if (ptzBaudrate == 0){
		perror("should input the baudrate >0 ");
		return -1;
	}
	if (ptzStep == 0){
		perror("should input the step > 0");
		return -1;
	}
	
	fd_uart1 = open("/dev/ttyS1", O_RDWR, 0);		
	if (fd_uart1 == -1) {
		perror("open uart port: Unable to open /dev/ttyS1 - ");
		return -1;
	} else {
		fcntl(fd_uart1, F_SETFL, 0);
	}
	
	PelcoD_init(fd_uart1,1,ptzBaudrate,ptzPort,1);// gpio31

	if (ptzSpeed){
		test_ptz_speed();
	}

	sleep(1);
	
	int j;
	switch(ptzCmd){
		case 'u':
			for(j = 0; j < ptzStep; j++){
				test_ptz_up(1);
				sleep(1);
			}
			break;
		case 'd':
			for(j = 0; j < ptzStep; j++){
				test_ptz_down(1);
				sleep(1);
			}
			break;
		case 'l':
			for(j = 0; j < ptzStep; j++){
				test_ptz_left(1);
				sleep(1);
			}
			break;
		case 'L':
			for(j = 0; j < ptzStep; j++){
				test_ptz_left_up(1);
				sleep(1);
			}
			break;
		case 'M':
			for(j = 0; j < ptzStep; j++){
				test_ptz_left_down(1);
				sleep(1);
			}
			break;
		case 'r':
			for(j = 0; j < ptzStep; j++){
				test_ptz_right(1);
				sleep(1);
			}
			break;
		case 'R':
			for(j = 0; j < ptzStep; j++){
				test_ptz_right_up(1);
				sleep(1);
			}
			break;
		case 'N':
			for(j = 0; j < ptzStep; j++){
				test_ptz_right_down(1);
				sleep(1);
			}
			break;
		case 'E':
			for(j = 0; j < ptzStep; j++){
				test_ptz_eight_directions(2);
				//sleep(1);
			}
			break;
	}
	close(fd_uart1);
	printf("Done!\n");

	return 0;
}

