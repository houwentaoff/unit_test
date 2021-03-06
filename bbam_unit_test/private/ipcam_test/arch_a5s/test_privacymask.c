/**********************************************************
 * test_privacymask.c
 *
 * History:
 *	2010/05/28 - [Louis Sun] created for A5s
 *	2011/07/04 - [Jian Tang] modified for A5s
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************/
 
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


#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, n)	( ((x)+(n)-1u) & ~((n)-1u) )
#endif


static int fd_privacy_mask;

#define NO_ARG		0
#define HAS_ARG		1
#define OPTIONS_BASE		0

enum numeric_short_options {
	DISABLE_PRIVACY_MASK = OPTIONS_BASE,
};

#define MASK_DEFAULT_Y		16
#define MASK_DEFAULT_U		128
#define MASK_DEFAULT_V		128

#define MAX_MASK_BUF_NUM		(8)

static int num_block_x = 0;
static int num_block_y = 0;
static u8 * pm_start_addr[MAX_MASK_BUF_NUM];
static u8 pm_buffer_id = 0;
static u8 pm_buffer_max_num = 0;
static u32 pm_buffer_size = 0;

//mask color
typedef struct privacy_mask_color_s {
	u8 y;
	u8 u;
	u8 v;
	u8 reserved;
} privacy_mask_color_t;

static privacy_mask_color_t mask_color = {
	.y = MASK_DEFAULT_Y,
	.u = MASK_DEFAULT_U,
	.v = MASK_DEFAULT_V,
};

static int mask_color_set = 0;

//mask rect
typedef struct privacy_mask_rect_s{
	int start_x;
	int start_y;
	int width;
	int height;
} privacy_mask_rect_t;
static privacy_mask_rect_t	mask_rect;
static int mask_rect_set = 0;
static int mask_rect_remove = 0;

//mask disable flag
static int mask_disable  = 0;

//auto-run privacy mask flag
static int auto_run_flag = 0;
static int auto_run_interval = 3;

static struct option long_options[] = {
	{"xstart", HAS_ARG, 0, 'x'},
	{"ystart", HAS_ARG, 0, 'y'},
	{"width", HAS_ARG, 0, 'w'},
	{"height", HAS_ARG, 0, 'h'},
	{"remove", NO_ARG, 0, 'd'},

	{"luma", HAS_ARG, 0, 'Y'},
	{"chroma-u", HAS_ARG, 0, 'U'},
	{"chroma-v", HAS_ARG, 0, 'V'},

	//turn it off
	{"disable", NO_ARG, 0, DISABLE_PRIVACY_MASK},
	{"auto", HAS_ARG, 0, 'r'},

	{0, 0, 0, 0}
};

static const char *short_options = "x:y:w:h:dY:U:V:r:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tset offset x"},
	{"", "\t\tset offset y"},
	{"", "\t\tset mask width"},
	{"", "\t\tset mask height"},
	{"", "\t\tremove mask"},

	{"0~255", "\tset privacy mask luma"},
	{"0~255", "\tset privacy mask chroma U"},
	{"0~255", "\tset privacy mask chroma V"},

	{"", "\t\tclear all masks"},
	{"1~30", "\tauto run privacy mask every N frames, default is 3 frames"},
};

static void usage(void)
{
	int i;

	printf("test_privacymask usage:\n");
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
	printf("\nExamples:");
	printf("\n  Add include a privacy mask:\n    test_privacymask -x100 -y100 -w256 -h256\n");
	printf("\n  Add exclude a privacy mask:\n    test_privacymask -x100 -y100 -w256 -h256 -d\n");
	printf("\n  Clear all privacy masks:\n    test_privacymask --disable\n");
	printf("\n  Auto run privacy masks every 4 frames:\n    test_privacymask -r 4\n");
	printf("\n");
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'x':
			mask_rect.start_x = atoi(optarg);
			mask_rect_set = 1;
			break;
		case 'y':
			mask_rect.start_y = atoi(optarg);
			mask_rect_set = 1;
			break;
		case 'w':
			mask_rect.width = atoi(optarg);
			mask_rect_set = 1;
			break;
		case 'h':
			mask_rect.height = atoi(optarg);
			mask_rect_set = 1;
			break;
		case 'd':
			mask_rect_remove = 1;
			break;
		case 'Y':
			mask_color.y	= atoi(optarg);
			mask_color_set = 1;
			break;
		case 'U':
			mask_color.u	= atoi(optarg);
			mask_color_set = 1;
			break;
		case 'V':
			mask_color.v	= atoi(optarg);
			mask_color_set = 1;
			break;
		case DISABLE_PRIVACY_MASK:
			mask_disable  = 1;
			break;
		case 'r':
			auto_run_flag = 1;
			auto_run_interval = atoi(optarg);
			if (auto_run_interval < 1 || auto_run_interval > 30) {
				printf("Invalid auto run interval value [%d], please choose from 1~30.\n",
					auto_run_interval);
				return -1;
			}
			mask_disable = 0;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}
	return 0;
}

static int get_privacymask_buffer_id(void)
{
	iav_privacy_mask_info_ex_t mask_info;
	if (ioctl(fd_privacy_mask, IAV_IOC_GET_PRIVACY_MASK_INFO_EX, &mask_info) < 0) {
		perror("IAV_IOC_GET_PRIVACY_MASK_INFO_EX");
		return -1;
	}
	pm_buffer_id = mask_info.next_buffer_id;
	pm_buffer_max_num = mask_info.total_buffer_num;
	return 0;
}

static int map_privacymask(void)
{
	int i;
	iav_mmap_info_t mmap_info;

	if (ioctl(fd_privacy_mask, IAV_IOC_MAP_PRIVACY_MASK_EX, &mmap_info) < 0) {
		perror("IAV_IOC_MAP_PRIVACY_MASK_EX");
		return -1;
	}
	if (get_privacymask_buffer_id() < 0) {
		printf("Get wrong privacy mask buffer id.\n");
		return -1;
	}

	pm_buffer_size = mmap_info.length / pm_buffer_max_num;
	for (i = 0; i < pm_buffer_max_num; ++i) {
		pm_start_addr[i] = mmap_info.addr + i * pm_buffer_size;
	}

	return 0;
}

static int check_for_privacymask(privacy_mask_rect_t *input_mask_pixel_rect)
{
	struct amba_video_info video_info;
	iav_state_info_t info;
	iav_source_buffer_format_all_ex_t buf_format;

	if (ioctl(fd_privacy_mask, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return -1;
	}

	if ((info.state != IAV_STATE_PREVIEW) &&
		(info.state != IAV_STATE_ENCODING)) {
		printf("privacymask need iav to be in preview or encoding, cur state is %d\n", info.state);
		return -1;
	}

	if (ioctl(fd_privacy_mask, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}
	if (ioctl(fd_privacy_mask, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
		return -1;
	}
	if ((video_info.format == AMBA_VIDEO_FORMAT_INTERLACE) &&
		(buf_format.main_deintlc_for_intlc_vin != DEINTLC_BOB_MODE)) {
		input_mask_pixel_rect->start_y /= 2;
		input_mask_pixel_rect->height /= 2;
	}
	printf("start_x : %d, start_y : %d, width : %d, height : %d\n",
		input_mask_pixel_rect->start_x, input_mask_pixel_rect->start_y,
		input_mask_pixel_rect->width, input_mask_pixel_rect->height);

	//check for input mask pixel rect
	if ((input_mask_pixel_rect->start_x + input_mask_pixel_rect->width > num_block_x *16) ||
		(input_mask_pixel_rect->start_y + input_mask_pixel_rect->height > num_block_y *16)) {
		printf("input mask rect error, start_x %d, start_y %d, width %d, height %d \n",
			input_mask_pixel_rect->start_x, input_mask_pixel_rect->start_y,
			input_mask_pixel_rect->width, input_mask_pixel_rect->height);
		return -1;
	}

	if ((input_mask_pixel_rect->width <= 0) ||
		(input_mask_pixel_rect->height <= 0)) {
		printf("input mask rect size error \n");
		return -1;
	}

	return 0;
}

//calculate privacy mask size for memory filling,  privacy mask take effects on macro block (16x16)
static int calc_privacy_mask_size(void)
{
	iav_source_buffer_format_all_ex_t buf_format;

	if (ioctl(fd_privacy_mask, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX");
		return -1;
	}
	num_block_x = ROUND_UP(buf_format.main_width, 16) / 16;
	num_block_y = ROUND_UP(buf_format.main_height, 16) / 16;

//	printf("num_block_x %d, num_block_y %d \n", num_block_x, num_block_y);
	return 0;
}


//block_rect's every field uses unit of block, not pixels
//try to mask more but not less when input pixels are not on block boundary
static int find_nearest_block_rect(privacy_mask_rect_t *block_rect , privacy_mask_rect_t *input_rect)
{
	int end_x, end_y;
	block_rect->start_x = (input_rect->start_x)/16;
	block_rect->start_y = (input_rect->start_y)/16;
	end_x = ROUND_UP((input_rect->start_x + input_rect->width), 16);
	end_y = ROUND_UP((input_rect->start_y + input_rect->height), 16);

	block_rect->width = end_x/16 - block_rect->start_x;
	block_rect->height= end_y/16 - block_rect->start_y;
	return 0;
}

static inline int is_masked(int block_x, int block_y,  privacy_mask_rect_t * block_rect)
{
	return  ((block_x >= block_rect->start_x) &&
		(block_x < block_rect->start_x + block_rect->width) &&
		(block_y >= block_rect->start_y) &&
		(block_y < block_rect->start_y + block_rect->height));
}

//fill memory,  use most straightforward way to detect mask, without optimization
static int fill_privacy_mask_mem(privacy_mask_rect_t * rect)
{
	int each_row_bytes, num_rows;
	u32 *wp;
	int i, j, k;
	int row_gap_count;
	privacy_mask_rect_t nearest_block_rect;

	if ((num_block_x == 0) || (num_block_y == 0)) {
		printf("privacy mask block number error \n");
		return -1;
	}
	if (get_privacymask_buffer_id() < 0) {
		printf("Get wrong privacy mask buffer id.\n");
		return -1;
	}

	if(find_nearest_block_rect(&nearest_block_rect, rect) < 0) {
		printf("input rect error \n");
		return -1;
	}

	// privacy mask dsp mem uses 4 bytes to represent one block,
	// and width needs to be 32 bytes aligned
	each_row_bytes = ROUND_UP((num_block_x * 4), 32);
	num_rows = num_block_y;
	wp = (u32 *)pm_start_addr[pm_buffer_id];
	row_gap_count = (each_row_bytes - num_block_x*4)/4;

	for(i = 0; i < num_rows; i++) {
		for (j = 0; j < num_block_x ; j++) {
			k = is_masked(j, i, &nearest_block_rect);
			if (mask_rect_remove)
				*wp &= ~k;
			else
				*wp |= k;
			wp++;
		}
		wp += row_gap_count;
	}

	return 0;
}

//call IOCTL to set privacy mask
static int set_privacy_mask(int enable)
{
	iav_privacy_mask_setup_ex_t privacy_mask_setup;
	int curr_buffer_id;
	memset(&privacy_mask_setup, 0, sizeof(privacy_mask_setup));

	privacy_mask_setup.enable = enable;
	privacy_mask_setup.y = mask_color.y;
	privacy_mask_setup.u = mask_color.u;
	privacy_mask_setup.v = mask_color.v;
	privacy_mask_setup.block_count_x = num_block_x;
	privacy_mask_setup.block_count_y = num_block_y;
	privacy_mask_setup.buffer_id = pm_buffer_id;

	if (ioctl(fd_privacy_mask, IAV_IOC_SET_PRIVACY_MASK_EX, &privacy_mask_setup) < 0) {
		perror("IAV_IOC_SET_PRIVACY_MASK_EX");
		return -1;
	}

	// copy data in multi buffers
	curr_buffer_id = pm_buffer_id;
	if (get_privacymask_buffer_id() < 0) {
		printf("Get wrong privacy mask buffer id.\n");
		return -1;
	}
	memcpy(pm_start_addr[pm_buffer_id],
		pm_start_addr[curr_buffer_id], pm_buffer_size);

	return 0;
}

static int auto_run_privacy_mask(int interval)
{
	u32 sleep_time, vin_frame_time;
	privacy_mask_rect_t rect_left, rect_right;

	if (ioctl(fd_privacy_mask, IAV_IOC_VIN_SRC_GET_FRAME_RATE, &vin_frame_time) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}
	sleep_time = interval * 1000000 / (DIV_ROUND(512000000, vin_frame_time));

	rect_left.start_x = rect_left.start_y = 100;
	rect_left.width = 640;
	rect_left.height = 400;
	mask_rect_remove = 0;
	memset(pm_start_addr[pm_buffer_id], 0, pm_buffer_size);
	fill_privacy_mask_mem(&rect_left);
	rect_left.start_x = rect_left.start_y = 200;
	rect_left.width = rect_left.height = 96;
	mask_rect_remove = 1;
	fill_privacy_mask_mem(&rect_left);
	set_privacy_mask(1);

	usleep(sleep_time);

	rect_right.start_x = 600;
	rect_right.start_y = 200;
	rect_right.width = 640;
	rect_right.height = 400;
	memset(pm_start_addr[pm_buffer_id], 0, pm_buffer_size);
	mask_rect_remove = 0;
	fill_privacy_mask_mem(&rect_right);
	rect_right.start_x = 700;
	rect_right.start_y = 300;
	rect_right.width= rect_right.height = 96;
	mask_rect_remove = 1;
	fill_privacy_mask_mem(&rect_right);
	set_privacy_mask(1);

	usleep(sleep_time);

	return 0;
}

static void sigstop()
{
	set_privacy_mask(0);
	exit(1);
}

int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if ((fd_privacy_mask = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0)	{
		printf("init param failed \n");
		return -1;
	}

	if (map_privacymask() < 0) {
		printf("map privacy mask failed \n");
		return -1;
	}

	if (calc_privacy_mask_size() < 0) {
		printf("calc privacy mask size failed \n");
		return -1;
	}

	if (auto_run_flag == 0) {
		//when not to disable mask
		if (!mask_disable) {
			if (check_for_privacymask(&mask_rect) < 0) {
				return -1;
			}

			if (fill_privacy_mask_mem(&mask_rect) < 0) {
				printf("fill privacy mask mem failed \n");
				return -1;
			}
		}

		if (set_privacy_mask(!mask_disable) < 0) {
			perror("set privacy mask");
			return -1;
		}
	} else {
		while (1) {
			if (auto_run_privacy_mask(auto_run_interval) < 0) {
				printf("Failed to run auto privacy mask!\n");
				return -1;
			}
		}
	}

	return 0;
}

