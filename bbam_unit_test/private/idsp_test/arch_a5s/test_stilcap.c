/**********************************************************************
 *
 * test_stilcap.c
 *
 * History:
 *	2010/08/28 - [Jian Tang] Created this file
 *
 * Copyright (C) 2007 - 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************************/

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
#include <assert.h>
#include <basetypes.h>
#include <signal.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include "datatx_lib.h"

#include "img_struct.h"
#include "img_api.h"

#ifndef FILENAME_LENGTH
#define	FILENAME_LENGTH		(256)
#endif

#ifndef BASE_PORT
#define	BASE_PORT			(2020)
#endif

#ifndef ARR_SIZE
#define ARR_SIZE(x)   ((sizeof (x)) / (sizeof *(x)))
#endif

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg) \
do { \
	if (ioctl(_filp, _cmd, _arg) < 0) {	\
		perror(#_cmd);	\
		return -1;	\
	}	\
}while (0)
#endif

static int capture_jpeg = 0;
static int capture_thumb = 0;
static int capture_raw = 0;
static int keep_AR_flag = 0;
static int quality = 75;

static int capture_num = 1;
//static int capture_num_flag = 0;
static int jpeg_width = 0;
static int jpeg_height = 0;
//static int jpeg_resolution_flag = 0;
static int thumb_width = 0;
static int thumb_height = 0;
//static int thumb_resolution_flag = 0;

int transfer_method = TRANS_METHOD_NFS;
int transfer_port = BASE_PORT;
const char *default_filename_nfs = "/mnt/media/amba";
const char *default_filename_tcp = "media/";
const char *default_host_ip_addr = "10.0.0.1";
const char *default_filename;
static char filename[FILENAME_LENGTH];

int fd_iav;
u8 *bsb_mem;
u32 bsb_size;

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "c:f:jJ:knN:rq:tu";

static struct option long_options[] = {
	{"frames",      HAS_ARG,    0,    'c'},		// specify capture frames
	{"filename",    HAS_ARG,    0,    'f'},		// specify file name
	{"jpeg",        NO_ARG,	    0,	  'j'},		// capture jpeg
	{"jsize",       HAS_ARG,    0,    'J'},		// specify jpeg size
	{"keep-ar",	NO_ARG,	    0,    'k'},		// keep correct aspect ratio
	{"thumbnail",	NO_ARG,	    0,    'n'},		// capture thumbnail
	{"tsize",	HAS_ARG,    0,    'N'},		// specify thumbnail size
	{"raw",         NO_ARG,	    0,    'r'},		// capture raw
	{"quality",	HAS_ARG,    0,    'q'},		// jpeg quality
	{"tcp",	        NO_ARG,     0,    't'},
	{"usb",         NO_ARG,     0,    'u'},

	{0,             0,          0,     0 }
};

static const struct hint_s hint[] = {
	{"",     "\tspecify total capture frames"},
	{"",     "\tfilename to store capture file"},
	{"",     "\tcapture jpeg file"},
	{"",     "\tspecify jpeg resolution (default is VIN resolution)"},
	{"",     "\tkeep correct aspect ratio of jpeg"},
	{"",     "\tcapture thumbnail file"},
	{"",     "\tspecify thumbnail resolution (default is 160x120)"},
	{"",     "\tcapture raw file"},
	{"5~95", "specify jpeg quality"},
	{"",     "\tuse tcp to send data to PC"},
	{"",     "\tuse usb to send data to PC"},
};

static void usage(void)
{
	u32 i;
	printf("test_stilcap usage:\n");
	for (i = 0; i < ARR_SIZE(long_options) - 1; i++) {
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
	printf("Example:\n    test_stilcap -j -f test --tcp\n");
	printf("    test_stilcap -r -j -f test --usb\n\n");
}

//first second value must in format "x~y" if delimiter is '~'
static int get_two_values(const char *name, int * first, int * second, char delimiter)
{
	char tmp_string[16] = {0};
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("range should be like a%cb \n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1,  name + strlen(name) -separator);
	*second = atoi(tmp_string);

//	printf("input string %s,  first value %d, second value %d \n",name, *first, *second);
	return 0;
}

static int init_param(int argc, char **argv)
{
	int ch;
	int first, second;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'c':
			//capture_num_flag = 1;
			capture_num = atoi(optarg);
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'j':
			capture_jpeg = 1;
			break;
		case 'J':
			if (get_two_values(optarg, &first, &second, 'x') < 0) {
				return -1;
			}
			jpeg_width = first;
			jpeg_height = second;
			//jpeg_resolution_flag = 1;
			break;
		case 'k':
			keep_AR_flag = 1;
			break;
		case 'n':
			capture_thumb = 1;;
			break;
		case 'N':
			if (get_two_values(optarg, &first, &second, 'x') < 0) {
				return -1;
			}
			thumb_width = first;
			thumb_height = second;
			//thumb_resolution_flag = 1;
			break;
		case 'r':
			capture_raw = 1;
			break;
		case 'q':
			quality = atoi(optarg);
			break;
		case 't':
			transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			transfer_method = TRANS_METHOD_USB;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			break;
		}
	}

	if (transfer_method == TRANS_METHOD_TCP ||
			transfer_method == TRANS_METHOD_USB)
		default_filename = default_filename_tcp;
	else
		default_filename = default_filename_nfs;

	return 0;
}

static int check_state(int fd_iav)
{
	int state;

	AM_IOCTL(fd_iav, IAV_IOC_GET_STATE, &state);
	if (state != IAV_STATE_PREVIEW) {
		perror("IAV is not in preview state, CANNOT take still capture!\n");
		return -1;
	}

	return 0;
}

static int save_raw_picture(int fd_iav, char *pic_file_name)
{
	char filename[FILENAME_LENGTH] = {0};
	iav_raw_info_t raw_info;
	int fd_raw = -1;

	memset(&raw_info, 0, sizeof(raw_info));
	AM_IOCTL(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info);

	printf("raw_addr = %p.\n", raw_info.raw_addr);
	printf("bit_resolution = %d.\n", raw_info.bit_resolution);
	printf("resolution = %dx%d.\n", raw_info.width, raw_info.height);
	printf("bayer_pattern = %d.\n", raw_info.bayer_pattern);
	printf("sensor_id = %d.\n", raw_info.sensor_id);

	sprintf(filename, "%s.raw", pic_file_name);

	fd_raw = amba_transfer_open(filename, transfer_method, transfer_port);
	if (fd_raw < 0) {
		printf("Error opening file [%s]!\n", filename);
		return -1;
	}
	if (amba_transfer_write(fd_raw, raw_info.raw_addr,
		raw_info.width * raw_info.height * 2, transfer_method) < 0) {
		perror("write(5)");
		return -1;
	}

	printf("Save RAW picture [%s].\n", filename);
	amba_transfer_close(fd_raw, transfer_method);

	return 0;
}

static int save_jpeg_picture(int fd_iav, char *pic_file_name, int pic_num)
{
	int i = 0, fd_jpeg = -1;
	char filename[FILENAME_LENGTH] = {0};
	u32 start_addr, pic_size, bsb_start, bsb_end;
	bs_fifo_info_t bs_info;
	int pic_counter = 0;
	static struct timeval pre_time = {0,0};
	static struct timeval curr_time = {0,0};
	int time_interval_us = 0;

	bsb_start = (u32)bsb_mem;
	bsb_end = bsb_start + bsb_size;
	memset(&bs_info, 0, sizeof(bs_info));

	gettimeofday(&pre_time, NULL);

	/*
	 * DSP always generates thumbnail with JPEG. It's decided in ARM side
	 * to read out thumbnail or not. All even pic numbers are reserved for
	 * JPEGs, and odd number are for thumbnails.
	 */
	for (pic_counter = 0; pic_counter < 2 * pic_num; ++pic_counter) {
		AM_IOCTL(fd_iav, IAV_IOC_READ_BITSTREAM, &bs_info);

		while (bs_info.count == 0) {
			printf("JPEG is not available, wait for a while!\n");
			AM_IOCTL(fd_iav, IAV_IOC_READ_BITSTREAM, &bs_info);
		}

		if ((capture_thumb == 0) && (pic_counter % 2))
			continue;

		for (i = 0; i < bs_info.count; ++i) {
			sprintf(filename, "%s_%d%03d%s.jpeg", pic_file_name, i,
				(pic_counter / 2), (pic_counter % 2) ? "_thumb" : "");
			fd_jpeg = amba_transfer_open(filename,
				transfer_method, transfer_port);
			if (fd_jpeg < 0) {
				printf("Error opening file [%s]!\n", filename);
				return -1;
			}
			pic_size = (bs_info.desc[i].pic_size + 31) & ~31;
			start_addr = bs_info.desc[i].start_addr;

			if (start_addr + pic_size <= bsb_end) {
				if (amba_transfer_write(fd_jpeg, (void *)start_addr,
					pic_size, transfer_method) < 0) {
					perror("Save jpeg file!\n");
					return -1;
				}
			} else {
				u32 size, remain_size;
				size = bsb_end - start_addr;
				remain_size = pic_size - size;
				if (amba_transfer_write(fd_jpeg, (void *)start_addr,
					size, transfer_method) < 0) {
					perror("Save jpeg file first part!\n");
					return -1;
				}
				if (amba_transfer_write(fd_jpeg, (void *)bsb_start,
					remain_size, transfer_method) < 0) {
					perror("Save jpeg file second part!\n");
					return -1;
				}
			}
			amba_transfer_close(fd_jpeg, transfer_method);
		}
		printf("Save JPEG %s #: %d.\n",
			(capture_thumb && (pic_counter % 2)) ? "Thumbnail " : "",
			pic_counter / 2);
	}

	gettimeofday(&curr_time, NULL);
	time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
		curr_time.tv_usec - pre_time.tv_usec;
        printf("total count : %d, total cost time : %d ms, per jpeg cost time: %d ms\n",
		pic_num, time_interval_us/1000, (time_interval_us / pic_num)/1000);

	return 0;
}

int map_bsb(int fd_iav)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	AM_IOCTL(fd_iav, IAV_IOC_MAP_BSB, &info);
	bsb_mem = info.addr;
	bsb_size = info.length;

	AM_IOCTL(fd_iav, IAV_IOC_MAP_DSP, &info);
	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;
	return 0;
}

int main(int argc, char ** argv)
{
	still_cap_info_t cap_info;

	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}
	if ((capture_raw == 0) && (capture_jpeg == 0)) {
		usage();
		return -1;
	}

	// open the IAV device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (check_state(fd_iav) < 0) {
		return -1;
	}

	if (map_bsb(fd_iav) < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	if (amba_transfer_init(transfer_method) < 0) {
		return -1;
	}

	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	memset(&cap_info, 0, sizeof(cap_info));
	cap_info.capture_num = capture_num;
	cap_info.jpeg_w = jpeg_width;
	cap_info.jpeg_h = jpeg_height;
	cap_info.thumb_w = thumb_width;
	cap_info.thumb_h = thumb_height;
	cap_info.need_raw = capture_raw;
	cap_info.keep_AR_flag = keep_AR_flag;

	if (img_init_still_capture(fd_iav, quality) < 0) {
		printf("Failed to init still capture mode!\n");
		return -1;
	}
	if (img_start_still_capture(fd_iav, &cap_info) < 0) {
		printf("Failed to start still capture process!\n");
		return -1;
	}

	if (capture_raw) {
		save_raw_picture(fd_iav, filename);
	}
	if (capture_jpeg || capture_thumb) {
		save_jpeg_picture(fd_iav, filename, cap_info.capture_num);
	}

	if (img_stop_still_capture(fd_iav) < 0) {
		printf("Failed to stop still capture mode!\n");
		return -1;
	}

	AM_IOCTL(fd_iav, IAV_IOC_LEAVE_STILL_CAPTURE, 0);

	if (amba_transfer_deinit(transfer_method) < 0) {
		return -1;
	}

	close(fd_iav);
	return 0;
}

