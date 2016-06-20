/*
 * test_memcpy.c
 *
 * History:
 *    2010/9/6 - [Louis Sun] create it to test memcpy by GDMA
 *
 * Copyright (C) 2007-2010, Ambarella, Inc. 
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include <signal.h>


int fd;
int block_size = 1024;
int block_count = 1024;

char filename[64] = "test.file";

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "b:n:f:";

static struct option long_options[] = {
	{"test", 		NO_ARG, 0, 't'},
	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\t test only"},
};

static void usage(void)
{
	u32 i;
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
	
	printf("    >test_memcpy  -t  \n" );

}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'b':
			block_size = atoi(optarg);
			break;
		case 'n':
			block_count = atoi(optarg);
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}


int main(int argc, char **argv)
{
	int i;
	char *buf;
//	if (argc < 2) {
//		usage();
//		return -1;
//	}
	
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	buf = malloc(block_size);

	// open the device
	if ((fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_DSYNC, 0777)) < 0) {
		perror("open file error");
		return -1;
	}

	for (i = 0; i < block_count; i++) {
		write(fd, buf, block_size);
	}

	printf("%d MB written.\n", block_size*block_count/1024/1024);
	return 0;
}

