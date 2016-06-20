/*
 * test_mem.c
 *
 * History:
 *	2010/12/24 - [Louis Sun] created file
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
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "basetypes.h"

#define NO_ARG				0
#define HAS_ARG				1

static struct option long_options[] = {
	{"lib_memcpy", NO_ARG, 0, 'l'},
	{"c_memcpy", NO_ARG, 0, 'c'},
	{"lib_memset", NO_ARG, 0, 's'},
	{"c_memset", NO_ARG, 0, 'S'},
	{"time", NO_ARG, 0, 't'},
	{"allocate",HAS_ARG, 0, 'a'},
	{"noaccess", NO_ARG, 0, 'n'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "lcsSta:n";
static int test_clib = 0;
static int test_clang = 0;

static int test_clib_memset = 0;
static int test_c_memset = 0;
static int test_time = 0;
static int test_allocate = 0;
static int allocate_size = 0; //in bytes

static int test_noaccess = 0;

static const struct hint_s hint[] = {
	{"", "\ttest memcpy by C run time library"},
	{"", "\ttest memcpy by C language"},
	{"", "\ttest memset by C run time library"},
	{"", "\ttest memset by C language"},
	{"", "\ttest time to see if timer is correct"},
	{"1~n","\tallocate how many MB memory"},
	{"", "\tdo not read/write in the allocate test"},
};


void usage(void)
{
	int i;

	printf("\ntest_mem usage:\n");
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
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
        case 'l':
			test_clib = 1;
			break;

		case 'c':
			test_clang = 1;
			break;

		case 's':
			test_clib_memset = 1;
			break;

		case 'S':
			test_c_memset = 1;
			break;

		case 't':
			test_time = 1;
			break;

		case 'a':
			test_allocate = 1;
			allocate_size =  atoi(optarg) *1024*1024;
			break;

		case 'n':
			test_noaccess = 1;
			break;			
				
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

int test_clibrary()
{
	struct timeval time_point1, time_point2;
	char * buf1, *buf2;
	const int test_length = 32*1024*1024;
	const int interation_times = 50;
	int i;
	unsigned int time_interval_us;
	
	buf1 = (char *)malloc(test_length); 
	buf2 = (char *)malloc(test_length);	
	if (!buf1 || !buf2) {
		printf("buf allocation error\n");
		return -1;
	}

	memset(buf2, 0, test_length);
	gettimeofday(&time_point1, NULL);

	for (i = interation_times; i > 0; i--)
	memcpy(buf1, buf2, test_length);

	gettimeofday(&time_point2, NULL);

	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C lib, memcpy speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));
	
	free(buf1);
	free(buf2);

	return 0;
}

static inline void m__memcpy(u32 * dst, u32 * src,  int count)
{
	int i;
	for(i = count >> 4; i > 0; i--) {
		*dst = *src; dst++;	src++;
		*dst = *src; dst++;	src++;
		*dst = *src; dst++;	src++;
		*dst = *src; dst++;	src++;		
	}	
}



int test_clanguage()
{
	struct timeval time_point1, time_point2;
	char * buf1, *buf2;
	const int test_length = 32*1024*1024;
	const int interation_times = 50;
	int i;
	unsigned int time_interval_us;
	
	buf1 = (char *)malloc(test_length); 
	buf2 = (char *)malloc(test_length);	
	if (!buf1 || !buf2) {
		printf("buf allocation error\n");
		return -1;
	}

	memset(buf2, 0, test_length);
	gettimeofday(&time_point1, NULL);

	for (i = interation_times; i > 0; i--)
	m__memcpy((u32*)buf1, (u32*)buf2, test_length);

	gettimeofday(&time_point2, NULL);

	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C Language, memcpy speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));

	free(buf1);
	free(buf2);
		
	return 0;
}

int clib_memset()
{
	char * buf1;
	struct timeval time_point1, time_point2;
	unsigned int time_interval_us;
	const int test_length = 32*1024*1024;
	const int interation_times = 50;
	buf1 = (char *)malloc(test_length); 
	int i;

	gettimeofday(&time_point1, NULL);
	for (i = interation_times; i > 0 ; i --) {
		memset(buf1, 0, test_length);
	}
	gettimeofday(&time_point2, NULL);

	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C Lib, memset speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));

	
	free(buf1);
	return 0;
}

int c_memset()
{
	char * buf1;
	struct timeval time_point1, time_point2;
	unsigned int time_interval_us;
	const int test_length = 32*1024*1024;
	const int interation_times = 50;
	buf1 = (char *)malloc(test_length); 
	int i, j;
	unsigned int * pwrite;

	gettimeofday(&time_point1, NULL);
	for (i = interation_times; i > 0 ; i --) {
		pwrite = (unsigned int *)buf1;
		for (j = 0; j < test_length/4; j++)
			*pwrite++ = 0;					
	}
	gettimeofday(&time_point2, NULL);
	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C Language, memset speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));

	free(buf1);
	return 0;


}

int time_test()
{
	struct timeval time_point1, time_point2;
	unsigned int time_interval_us;
	gettimeofday(&time_point1, NULL);
	sleep(10);
	gettimeofday(&time_point2, NULL);
	
	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Sleep 10 seconds, get time of day %d useconds \n", 	time_interval_us);
	return 0;
}

int allocate_test(int size)
{
	u32 * buf;
	u32  test;
	int i;

	if (size <= 0) {
		printf("allocate size is wrong %d \n", size);
		return -1;
	}


	printf("prepare to allocate %d bytes of mem \n", size);
	buf = (u32 *)malloc(size);
	if (!buf) {
		printf("unable to allocate %d bytes of mem \n", size);
		return -1;
	}
	printf("malloc OK, now do memset \n");
	memset(buf, 0, size);

	printf("memset OK, Now start sequential access to the mem so that it's not easily swapped out\n");


	while (1) {
		if (test_noaccess) {
			usleep(1000000);
		}
		else {
			for (i = 0; i < size/sizeof(u32); i+= 1024) {
				test = buf[i];
				buf[i] = test + 1;
				usleep(1000);
			}
		}
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)	
		return -1;


	if (test_clib)
		test_clibrary();

	if (test_clang)
		test_clanguage();

	if (test_clib_memset)
		clib_memset();

	if (test_c_memset)
		c_memset();

	if (test_time)
		time_test();

	if (test_allocate)
		allocate_test(allocate_size);
	
	return 0;
}

