/*
 * test_dptz.c
 * the program can change digital ptz parameters
 *
 * History:
 *	2010/02/20 - [Louis Sun] created file
 *	2011/07/20 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
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
#include <signal.h>

#define	ROUND_UP(x, n)				( ((x)+(n)-1u) & ~((n)-1u) )

#define	MAX_SOURCE_BUFFER_NUM	(4)
#define	MIN_PAN_TILT_STEP			(8)

#define	ZOOM_FACTOR_Y				(4 << 16)
#define	CENTER_OFFSET_Y_MARGIN	(4)

#define	MAX_MASK_BUF_NUM			(8)

// the device file handle
static int fd_iav;

//global variables
static int zoom_x = 1;
static int zoom_y = 1;
static int center_offset_x = 0;
static int center_offset_y = 0;
static int zoom_x_flag = 0;
static int zoom_y_flag = 0;
static int center_offset_x_flag = 0;
static int center_offset_y_flag = 0;

static int default_zoom_times = 1;
static int default_zoom_times_flag = 0;


static int current_source = -1;	// -1 is a invalid source buffer, for initialize data only

// digital zoom type II format
typedef struct dptz_format_s {
	int width;
	int height;
	int resolution_flag;
	int offset_x;
	int offset_y;
	int offset_flag;
} dptz_format_t;

static dptz_format_t dptz_format[MAX_SOURCE_BUFFER_NUM];
static int dptz_format_change_id = 0;

//auto run zoom demo
static int autorun_zoom_I_flag = 0;
static int autorun_zoom_II_flag = 0;

// digital pan format
typedef struct pan_tilt_format_s {
	int offset_start_x;
	int offset_start_y;
	int offset_end_x;
	int offset_end_y;
} pan_tilt_format_t;

static pan_tilt_format_t pt_format[MAX_SOURCE_BUFFER_NUM];
static int pan_format_change_id = 0;

//auto run pan tilt demo
static int pan_tilt_step = MIN_PAN_TILT_STEP;
static int autorun_pan_I_flag = 0;
//static int autorun_pan_II_flag = 0;


// auto privacy mask demo
static iav_mmap_info_t pm_mmap_info;
static int autorun_pm_flag = 0;
static int autorun_pm_count = 0;
static u8 * pm_start_addr[MAX_MASK_BUF_NUM];
static u8 pm_buffer_id = 0;
static u8 pm_buffer_max_num = 0;
static u32 pm_buffer_size = 0;
static int num_block_x = 0;
static int num_block_y = 0;

//mask color
typedef struct privacy_mask_color_s {
	u8 y;
	u8 u;
	u8 v;
	u8 reserved;
} privacy_mask_color_t;
static privacy_mask_color_t mask_color = {
	.y = 210,
	.u = 16,
	.v = 146,
};

//mask rect
typedef struct privacy_mask_rect_s{
	int start_x;
	int start_y;
	int width;
	int height;
} privacy_mask_rect_t;

// options and usage
#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{"zoom-x", HAS_ARG, 0, 'x'},
	{"zoom-y", HAS_ARG, 0, 'y'},
	{"offset-x", HAS_ARG, 0, 'a'},
	{"offset-y", HAS_ARG, 0, 'b'},
	{"default-zoom", HAS_ARG, 0, 'd'},
	{"add-mask", HAS_ARG, 0, 'M'},
	{"pt-x-range", HAS_ARG, 0, 'm'},
	{"pt-y-range", HAS_ARG, 0, 'n'},
	{"pt-step", HAS_ARG, 0, 't'},
	{"auto-pt", NO_ARG, 0, 'p'},
	{"autozoom", NO_ARG, 0, 'r'},
	{"autozoom2", NO_ARG, 0, 'R'},
	{"buffer2", NO_ARG, 0, 'Y'},
	{"buffer3", NO_ARG, 0, 'J'},
	{"buffer4", NO_ARG, 0, 'K'},
	{"zoom-window-size", HAS_ARG, 0, 's'},
	{"zoom-window-offset", HAS_ARG, 0, 'o'},
	{0, 0, 0, 0}
};

static const char *short_options = "x:y:a:b:d:M:m:n:t:prRYJKs:o:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"16.16 format", "zoom factor x for main buffer"},
	{"16.16 format", "zoom factor y for main buffer"},
	{"", "\t\tcenter offset x for main buffer"},
	{"", "\t\tcenter offset y for main buffer"},
	{"1~10", "default zoom times from center, overwrite other settings"},
	{"n", "\tadd privacy mask automatically for times"},
	{"A~B", "\tdefault range is from left to right"},
	{"A~B", "\tdefault range is from top to bottom"},
	{"", "\t\tspecify steps in width and height direction, minimum step is 8"},
	{"", "\t\trun auto pan tilt for type I with specified width and height range"},
	{"", "\t\trun auto zoom type I from 1X to 10X"},
	{"", "\t\trun auto zoom type II"},
	{"", "\t\tDPTZ type II config for second buffer"},
	{"", "\t\tDPTZ type II config for third buffer"},
	{"", "\t\tDPTZ type II config for fourth buffer"},
	{"", "\tDPTZ type II zoom out window size (for 2nd buffer, must be not smaller than buffer size)"},
	{"", "\tDPTZ type II zoom out window offset"},
};

void usage(void)
{
	int i;
	printf("test_dptz usage:\n");
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
	printf("\nExamples:\n  Type I:\n    test_dptz -d 2 -a 10 -b 10\n");
	printf("    test_dptz -d 2 -m -300~300 -n -200~200 -p\n");
	printf("\n  Type I with privacy mask:\n    test_dptz -d 2 -M 4\n");
	printf("\n  Type II:\n    test_dptz -Y -s 720x480 -o 100x200\n");
	printf("\n");
}

// first and second values must be in format of "AxB"
static int get_two_values(const char *name, int *first, int *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("two values should be like A%cB .\n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1,  name + strlen(name) - separator);
	*second = atoi(tmp_string);

//	printf("input string [%s], first value %d, second value %d.\n", name, *first, *second);
	return 0;
}


#define	VERIFY_BUFFERID(x)		do {		\
			if ((x) < 1 || ((x) >= MAX_SOURCE_BUFFER_NUM)) {	\
				printf("Wrong buffer id %d.\n", (x));		\
				return -1;	\
			}		\
		} while (0)


int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int first, second;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		// parameters for DPTZ type I
		case 'x':
			zoom_x = atoi (optarg);
			zoom_x_flag =1;
			break;
		case 'y':
			zoom_y = atoi (optarg);
			zoom_y_flag =1;
			break;
		case 'a':
			center_offset_x = atoi(optarg);
			center_offset_x_flag = 1;
			break;
		case 'b':
			center_offset_y = atoi(optarg);
			center_offset_y_flag = 1;
			break;
		case 'd':
			default_zoom_times = atoi(optarg);
			default_zoom_times_flag = 1;
			break;
		case 'M':
			autorun_pm_count = atoi(optarg);
			autorun_pm_flag = 1;
			break;
		case 'm':
			if (get_two_values(optarg, &first, &second, '~') < 0)
				return -1;
			pt_format[0].offset_start_x = first;
			pt_format[0].offset_end_x = second;
			pan_format_change_id |= (1 << 0);
			break;
		case 'n':
			if (get_two_values(optarg, &first, &second, '~') < 0)
				return -1;
			pt_format[0].offset_start_y = first;
			pt_format[0].offset_end_y = second;
			pan_format_change_id |= (1 << 0);
			break;
		case 'p':
			autorun_pan_I_flag = 1;
			break;
		case 't':
			pan_tilt_step = atoi(optarg);
			break;
		case 'r':
			autorun_zoom_I_flag = 1;
			break;

		// parameters for DPTZ type II
		case 'Y':
			current_source = 1;
			break;
		case 'J':
			current_source = 2;
			break;
		case 'K':
			current_source = 3;
			break;
		case 's':
			VERIFY_BUFFERID(current_source);
			if (get_two_values(optarg, &first, &second, 'x') < 0)
				return -1;
			dptz_format[current_source].width = first;
			dptz_format[current_source].height = second;
			dptz_format[current_source].resolution_flag = 1;
			dptz_format_change_id |= (1 << current_source);
			break;
		case 'o':
			VERIFY_BUFFERID(current_source);
			if (get_two_values(optarg, &first, &second, 'x') < 0)
				return -1;
			dptz_format[current_source].offset_x = first;
			dptz_format[current_source].offset_y = second;
			dptz_format[current_source].offset_flag = 1;
			dptz_format_change_id |= (1 << current_source);
			break;
		case 'R':
			autorun_zoom_II_flag = 1;
			break;

		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}
	return 0;
}

static int get_privacymask_buffer_id(void)
{
	iav_privacy_mask_info_ex_t mask_info;
	if (ioctl(fd_iav, IAV_IOC_GET_PRIVACY_MASK_INFO_EX, &mask_info) < 0) {
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

	if (ioctl(fd_iav, IAV_IOC_MAP_PRIVACY_MASK_EX, &pm_mmap_info) < 0) {
		perror("IAV_IOC_MAP_PRIVACY_MASK_EX");
		return -1;
	}
	if (get_privacymask_buffer_id() < 0) {
		printf("Get wrong privacy mask buffer id.\n");
		return -1;
	}

	memset(pm_mmap_info.addr, 0, pm_mmap_info.length);
	pm_buffer_size = pm_mmap_info.length / pm_buffer_max_num;
	for (i = 0; i < pm_buffer_max_num; ++i) {
		pm_start_addr[i] = pm_mmap_info.addr + i * pm_buffer_size;
	}

	return 0;
}

//calculate privacy mask size for memory filling,  privacy mask take effects on macro block (16x16)
static int calc_privacy_mask_size(void)
{
	iav_source_buffer_format_all_ex_t buf_format;

	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX");
		return -1;
	}
	num_block_x = ROUND_UP(buf_format.main_width, 16) / 16;
	num_block_y = ROUND_UP(buf_format.main_height, 16) / 16;

//	printf("num_block_x %d, num_block_y %d \n", num_block_x, num_block_y);
	return 0;
}

static inline int is_masked(int block_x, int block_y,  privacy_mask_rect_t * block_rect)
{
	return  ((block_x >= block_rect->start_x) &&
		(block_x < block_rect->start_x + block_rect->width) &&
		(block_y >= block_rect->start_y) &&
		(block_y < block_rect->start_y + block_rect->height));
}

//fill memory, use most straightforward way to detect mask, without optimization
static int fill_privacy_mask_mem(privacy_mask_rect_t * rect)
{
	int each_row_bytes, num_rows;
	u32 *wp;
	int i, j, k;
	int row_gap_count;
	int end_x, end_y;
	privacy_mask_rect_t nearest_block_rect;

	if ((num_block_x == 0) || (num_block_y == 0)) {
		printf("privacy mask block number error \n");
		return -1;
	}
	if (get_privacymask_buffer_id() < 0) {
		printf("Get wrong privacy mask buffer id.\n");
		return -1;
	}

	nearest_block_rect.start_x = (rect->start_x) / 16;
	nearest_block_rect.start_y = (rect->start_y) / 16;
	end_x = ROUND_UP((rect->start_x + rect->width), 16);
	end_y = ROUND_UP((rect->start_y + rect->height), 16);

	nearest_block_rect.width = end_x / 16 - nearest_block_rect.start_x;
	nearest_block_rect.height = end_y / 16 - nearest_block_rect.start_y;

	// privacy mask dsp mem uses 4 bytes to represent one block,
	// and width needs to be 32 bytes aligned
	each_row_bytes = ROUND_UP((num_block_x * 4), 32);
	num_rows = num_block_y;
	wp = (u32 *)pm_start_addr[pm_buffer_id];
	row_gap_count = (each_row_bytes - num_block_x * 4) / 4;
	for(i = 0; i < num_rows; i++) {
		for (j = 0; j < num_block_x ; j++) {
			k = is_masked(j, i, &nearest_block_rect);
			*wp = k;
			wp++;
		}
		wp += row_gap_count;
	}

	return 0;
}

// set privacy mask
static int set_privacy_mask(int enable, iav_privacy_mask_setup_ex_t *privacy_mask_setup)
{
	privacy_mask_setup->enable = enable;
	privacy_mask_setup->y = mask_color.y;
	privacy_mask_setup->u = mask_color.u;
	privacy_mask_setup->v = mask_color.v;
	privacy_mask_setup->block_count_x = num_block_x;
	privacy_mask_setup->block_count_y = num_block_y;
	privacy_mask_setup->buffer_id = pm_buffer_id;

	return 0;
}

static int auto_privacy_mask_for_main_buffer(iav_digital_zoom_ex_t * digital_zoom)
{
	int count, offset, offset_delta, sleep_time;
	privacy_mask_rect_t rect;
	iav_digital_zoom_privacy_mask_ex_t dz_pm;

	memset(&dz_pm, 0, sizeof(dz_pm));
	dz_pm.zoom = *digital_zoom;

	rect.start_x = center_offset_x > 0 ? center_offset_x:-center_offset_x;
	rect.start_y = center_offset_y > 0 ? center_offset_y:-center_offset_y;
	rect.width = 320;
	rect.height = 160;

	offset = 100;
	offset_delta = 30;
	sleep_time = 34000;
	for (count = 0; count < autorun_pm_count; count += 2, offset += offset_delta) {
		rect.start_x += offset;
		rect.start_y += offset;
		dz_pm.zoom.center_offset_x += (100 << 16);
		dz_pm.zoom.center_offset_y += (100 << 16);
		fill_privacy_mask_mem(&rect);
		set_privacy_mask(1, &dz_pm.privacy_mask);
		if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX, &dz_pm) < 0) {
			perror("IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX");
			return -1;
		}
		printf("*** [Round %d : sleep %d us] set privacy mask with dptz, buffer id [%d].\n",
			count, sleep_time, pm_buffer_id);
		usleep(sleep_time);

		rect.start_x -= offset;
		rect.start_y -= offset;
		dz_pm.zoom.center_offset_x -= (100 << 16);
		dz_pm.zoom.center_offset_y -= (100 << 16);
		fill_privacy_mask_mem(&rect);
		set_privacy_mask(1, &dz_pm.privacy_mask);
		if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX, &dz_pm) < 0) {
			perror("IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX");
			return -1;
		}
		printf("*** [Round %d : sleep %d us] set privacy mask with dptz, buffer id [%d].\n",
			count + 1, sleep_time, pm_buffer_id);
		usleep(sleep_time);
	}

	// remove all privacy mask automatically
	get_privacymask_buffer_id();
	set_privacy_mask(0, &dz_pm.privacy_mask);
	if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX, &dz_pm) < 0) {
		perror("IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX");
		return -1;
	}
	printf("*** [Round %d] Quit privacy mask with dptz, buffer id [%d].\n\n",
		count++, pm_buffer_id);
	sleep(1);
	memset(pm_mmap_info.addr, 0, pm_mmap_info.length);

	return 0;
}

static int digital_ptz_for_main_buffer(void)
{
	struct amba_video_info video_info;
	iav_source_buffer_format_all_ex_t buf_format;
	iav_digital_zoom_ex_t digital_zoom;
	int cap_w, cap_h;

	if (default_zoom_times_flag) {
		if ((default_zoom_times < 0) || (default_zoom_times > 10)) {
			printf("currently support 0 ~ 10 times Dzoom only\n");
			return -1;
		}
		if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
			perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
			return -1;
		}
		if (default_zoom_times == 0) {
			if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
				perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
				return -1;
			}
			cap_w = video_info.width;
			cap_h = video_info.height;
		} else {
			cap_w = ROUND_UP(buf_format.main_width / default_zoom_times, 4);
			cap_h = ROUND_UP(buf_format.main_height / default_zoom_times, 4);
		}
		digital_zoom.zoom_factor_x = (buf_format.main_width << 16) / cap_w;
		digital_zoom.zoom_factor_y = (buf_format.main_height << 16) / cap_h;
		digital_zoom.center_offset_x = 0;
		digital_zoom.center_offset_y = 0;
	} else {
		if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
			perror("IAV_IOC_GET_DIGITAL_ZOOM_EX");
			return -1;
		}
		if (zoom_x_flag)
			digital_zoom.zoom_factor_x = zoom_x;
		if (zoom_y_flag)
			digital_zoom.zoom_factor_y = zoom_y;
	}
	if (center_offset_x_flag)
		digital_zoom.center_offset_x = center_offset_x * 65536;
	if (center_offset_y_flag)
		digital_zoom.center_offset_y = center_offset_y * 65536;

	printf("(DPTZ Type I) zoomX %d, zoomY %d, X-center offset %d, Y-center offset %d.\n",
			digital_zoom.zoom_factor_x, digital_zoom.zoom_factor_y,
			((int)digital_zoom.center_offset_x / 65536),
			((int)digital_zoom.center_offset_y / 65536));

	if (autorun_pm_flag) {
		//add privacy mask automatically
		if (auto_privacy_mask_for_main_buffer(&digital_zoom) < 0) {
			printf("Failed to add privacy mask automatically!\n");
			return -1;
		}
	} else {
		if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
			perror("IAV_IOC_SET_DIGITAL_ZOOM_EX");
			return -1;
		}
	}

	return 0;
}

static int digital_ptz_for_preview_buffer(void)
{
	iav_digital_zoom2_ex_t digital_zoom;
	int i;

	for (i = 0; i < MAX_SOURCE_BUFFER_NUM; i++) {
		if (dptz_format_change_id & (1 << i)) {
			digital_zoom.source = i;

			if (ioctl(fd_iav, IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
				perror("IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX");
				return -1;
			}
			if (dptz_format[i].resolution_flag) {
				digital_zoom.input_width = dptz_format[i].width;
				digital_zoom.input_height = dptz_format[i].height;
			}
			if (dptz_format[i].offset_flag) {
				digital_zoom.input_offset_x = dptz_format[i].offset_x;
				digital_zoom.input_offset_y = dptz_format[i].offset_y;
			}
			printf("(DPTZ Type II) zoom out window size : %dx%d, offset : %dx%d\n",
				digital_zoom.input_width, digital_zoom.input_height,
				digital_zoom.input_offset_x, digital_zoom.input_offset_y);
			if (ioctl(fd_iav, IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
				perror("IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX");
				return -1;
			}
		}
	}

	return 0;
}

static int check_pan_tilt_param(void)
{
	struct amba_video_info video_info;
	iav_digital_zoom_ex_t digital_zoom;
	iav_source_buffer_format_all_ex_t buf_format;
	int max_offset_x, max_offset_y, cap_w, cap_h, adjust = 0;
	if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
		perror("IAV_IOC_GET_DIGITAL_ZOOM_EX");
		return -1;
	}
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
		return -1;
	}
	cap_w = (buf_format.main_width << 16) / digital_zoom.zoom_factor_x;
	cap_h = (buf_format.main_height << 16) / digital_zoom.zoom_factor_y;
	max_offset_x = (video_info.width - cap_w) / 2;
	max_offset_y = (video_info.height - cap_h) / 2;
	if (digital_zoom.zoom_factor_y >= ZOOM_FACTOR_Y) {
		max_offset_y -= CENTER_OFFSET_Y_MARGIN;
	}
	if (pt_format[0].offset_start_x + max_offset_x < 0) {
		adjust = 1;
		pt_format[0].offset_start_x = -max_offset_x;
	}
	if (pt_format[0].offset_end_x > max_offset_x) {
		adjust = 1;
		pt_format[0].offset_end_x = max_offset_x;
	}
	if (pt_format[0].offset_start_y + max_offset_y < 0) {
		adjust = 1;
		pt_format[0].offset_start_y = -max_offset_y;
	}
	if (pt_format[0].offset_end_y > max_offset_y) {
		adjust = 1;
		pt_format[0].offset_end_y = max_offset_y;
	}
	if (pan_tilt_step < MIN_PAN_TILT_STEP) {
		pan_tilt_step = MIN_PAN_TILT_STEP;
	}
	if (adjust) {
		printf("Adjust auto pan tilt param: center offset x range (%d, %d), center offset y range (%d, %d).\n",
			pt_format[0].offset_start_x, pt_format[0].offset_end_x,
			pt_format[0].offset_start_y, pt_format[0].offset_end_y);
	}
	return 0;
}

static int auto_pan_tilt_for_main(void)
{
	iav_digital_zoom_ex_t digital_zoom;
	int x, y, dx, dy;
	int dir, time;

	if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
		perror("IAV_IOC_GET_DIGITAL_ZOOM_EX");
		return -1;
	}
	check_pan_tilt_param();

	dx = (pt_format[0].offset_end_x - pt_format[0].offset_start_x) / pan_tilt_step;
	dy = (pt_format[0].offset_end_y - pt_format[0].offset_start_y) / pan_tilt_step * 2;
	x = pt_format[0].offset_start_x;
	y = pt_format[0].offset_start_y;
	dir = 1;
	while (1) {
		digital_zoom.center_offset_x = (x << 16);
		digital_zoom.center_offset_y = (y << 16);
//		printf("+++++++++++++++++++ center offset : %dx%d ==> %d x %d.\n", x, y,
//			digital_zoom.center_offset_x, digital_zoom.center_offset_y);
		if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
			perror("IAV_IOC_SET_DIGITAL_ZOOM_EX");
			return -1;
		}
		time = 200 * 1000;
		x += dir * dx;
		if (x > pt_format[0].offset_end_x) {
			x = pt_format[0].offset_end_x;
			dir = -1;
			y += dy;
		}
		if (x < pt_format[0].offset_start_x) {
			x = pt_format[0].offset_start_x;
			dir = 1;
			y += dy;
		}
		if (y > pt_format[0].offset_end_y) {
			x = pt_format[0].offset_start_x;
			y = pt_format[0].offset_start_y;
			dir = 1;
		}
		usleep(time);
	}
	return 0;
}

static int auto_zoom_for_main(void)
{
	//multi step zoom from 1X to 10X and then back to 1X,  in multi steps
	int zoom_factor;
	int i;
	iav_digital_zoom_ex_t digital_zoom;
	iav_source_buffer_format_all_ex_t buf_format;
	int cap_w, cap_h;

	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
		return -1;
	}

	for (i = 0; i <= 360; i++) {
		zoom_factor = 65536 + 65536 * i / 40;
		cap_w = ROUND_UP((buf_format.main_width << 16) / zoom_factor, 4);
		cap_h = ROUND_UP((buf_format.main_height << 16) / zoom_factor, 4);
		digital_zoom.zoom_factor_x = (buf_format.main_width << 16) / cap_w;
		digital_zoom.zoom_factor_y = (buf_format.main_height << 16) / cap_h;
		digital_zoom.center_offset_x  = 0;
		digital_zoom.center_offset_y  = 0;
		if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
			perror("IAV_IOC_SET_DIGITAL_ZOOM_EX");
			return -1;
		}
		usleep(1000*33);	//zoom at about 1 fps
	}

	for (i = 360; i >= 0; i--) {
		zoom_factor = 65536 + 65536 * i / 40;
		cap_w = ROUND_UP((buf_format.main_width << 16) / zoom_factor, 4);
		cap_h = ROUND_UP((buf_format.main_height << 16) / zoom_factor, 4);
		digital_zoom.zoom_factor_x = (buf_format.main_width << 16) / cap_w;
		digital_zoom.zoom_factor_y = (buf_format.main_height << 16) / cap_h;
		digital_zoom.center_offset_x  = 0;
		digital_zoom.center_offset_y  = 0;
		if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
			perror("IAV_IOC_SET_DIGITAL_ZOOM_EX");
			return -1;
		}
		usleep(1000*33);	//zoom at about 1 fps
	}

	return 0;
}

static int digital_zoom_autorun_for_prev(void)
{
	iav_source_buffer_format_all_ex_t buf_format;
	iav_digital_zoom2_ex_t digital_zoom;
	u32 w, h;
	int sleep_time = 2, time;
	int x, y, dir, max_x, max_y, dx, dy;
	int zm_flag, countX, countY, zm_factor;// zm_out_flag

	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
		return -1;
	}
	memset(&digital_zoom, 0, sizeof(digital_zoom));
	digital_zoom.source = 1;
	if (ioctl(fd_iav, IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
		perror("IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX");
		return -1;
	}

	max_x = buf_format.main_width - buf_format.second_width;
	max_y = buf_format.main_height - buf_format.second_height;
	w = buf_format.second_width;
	h = buf_format.second_height;
	x = y = zm_flag = 0;
	countX = 20;
	countY = 10;
	zm_factor = 50;
	dir = 1;
	while (1) {
		digital_zoom.input_width = w & (~0x1);
		digital_zoom.input_height = h & (~0x3);
		digital_zoom.input_offset_x = x & (~0x1);
		digital_zoom.input_offset_y = y & (~0x3);
//		printf("(DPTZ Type II) zoom out window size :  %dx%d, offset : %dx%d\n",
//			digital_zoom.input_width, digital_zoom.input_height,
//			digital_zoom.input_offset_x, digital_zoom.input_offset_y);
		if (ioctl(fd_iav, IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
			perror("IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX");
		}
		if (!zm_flag) {
			time = 200 * 1000;
			x += dir * max_x / countX;
			if (x > max_x) {
				x = max_x;
				dir = -1;
				y += max_y / countY;
				sleep(sleep_time);
			}
			if (x <= 0) {
				x = 0;
				dir = 1;
				y += max_y / countY;
				sleep(sleep_time);
			}
			if (y > max_y) {
				x = max_x / 2;
				y = max_y / 2;
				zm_flag = 1;
				//zm_out_flag = 1;
				sleep(sleep_time);
			}
		} else {
			dy = max_y / zm_factor;
			dx = w * dy / h;
			x -= dx;
			y -= dy;
			w += 2 * dx;
			h += 2 * dy;
			if (h > buf_format.main_height) {
				w = buf_format.second_width;
				h = buf_format.second_height;
				x = y = zm_flag = 0;
				dir = 1;
				sleep(4);
			}
			time = 200 * 1000;
		}
		usleep(time);
	}

	return 0;
}

static void sigstop()
{
	if (autorun_pm_flag) {
		//remove all privacy mask
		iav_privacy_mask_setup_ex_t privacy_mask;
		get_privacymask_buffer_id();
		set_privacy_mask(0, &privacy_mask);
		if (ioctl(fd_iav, IAV_IOC_SET_PRIVACY_MASK_EX, &privacy_mask) < 0) {
			perror("IAV_IOC_SET_PRIVACY_MASK_EX");
		}
		memset(pm_mmap_info.addr, 0, pm_mmap_info.length);
	}
	exit(1);
}

int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	int do_dptz_for_main_buffer = 0;
	int do_dptz_for_preview_buffer = 0;

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (default_zoom_times_flag ||zoom_x_flag || zoom_y_flag
		|| center_offset_x_flag || center_offset_y_flag)
		do_dptz_for_main_buffer = 1;

	if (dptz_format_change_id)
		do_dptz_for_preview_buffer = 1;

	if (autorun_pm_flag) {
		if (map_privacymask() < 0) {
			printf("map privacy mask failed \n");
			return -1;
		}
		if (calc_privacy_mask_size() < 0) {
			printf("calc privacy mask size failed \n");
			return -1;
		}
	}

	if (do_dptz_for_main_buffer) {
		if (digital_ptz_for_main_buffer() < 0)
			return -1;
	}

	if (do_dptz_for_preview_buffer) {
		if (digital_ptz_for_preview_buffer() < 0)
			return -1;
	}

	if (autorun_pan_I_flag) {
		if (auto_pan_tilt_for_main() < 0)
			return -1;
	}

	if (autorun_zoom_I_flag) {
		if (auto_zoom_for_main() < 0)
			return -1;
	}

	if (autorun_zoom_II_flag) {
		if (digital_zoom_autorun_for_prev() < 0)
			return -1;
	}

	close(fd_iav);
	return 0;
}


