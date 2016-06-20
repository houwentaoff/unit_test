/*
 * test_yuvcap.c
 *
 * History:
 *	2008/05/13 - [Oliver Li] created file
 *	2012/02/09 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
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
#include "datatx_lib.h"
#include <signal.h>


typedef enum {
	CAPTURE_NONE = 255,
	CAPTURE_FIRST_TYPE = 0,
	CAPTURE_PREVIEW_BUFFER = 0,
	CAPTURE_PREVIEW_INFO,
	CAPTURE_MOTION_BUFFER,
	CAPTURE_TYPE_NUM,
	CAPTURE_LAST_TYPE = CAPTURE_TYPE_NUM,
} CAPTURE_TYPE;
//#define MAX_ME1_BUFFER_SIZE		(744*418)		// 1/16 of 2976x1674
#define MAX_ME1_BUFFER_SIZE		(648*486)		// 1/16 of 2592x1944
#define MAX_YUV_BUFFER_SIZE		(1920*1080)		// 1080p
#define PREVIEW_C_BUFFER_ID		(1)
#define PREVIEW_A_BUFFER_ID		(3)

#define YUVCAP_PORT					(2016)

typedef enum {
	YV12_FORMAT = 0,
	NV12_FORMAT = 1,
} YUV420_FORMAT;

int fd_iav;

static int transfer_method = TRANS_METHOD_NFS;
static int port = YUVCAP_PORT;

static int preview_buffer_id = PREVIEW_C_BUFFER_ID;
static int yuv420_format = YV12_FORMAT;
static int capture_select = CAPTURE_PREVIEW_BUFFER;
static int frame_count = 120;
static int quit_yuv_stream = 0;
static int verbose = 0;

const char *default_filename_nfs = "/mnt/media/test.yuv";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";
const char *default_filename;
static char filename[256];


static int map_buffer(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	mem_mapped = 1;
	return 0;
}

static inline int remove_padding_from_pitched_y
	(u8* output_y, const u8* input_y, int pitch, int width, int height)
{
	int row;
	for (row = 0; row < height; row++) { 		//row
		memcpy(output_y, input_y, width);
		input_y = input_y + pitch;
		output_y = output_y + width ;
	}
	return 0;
}

static inline int remove_padding_and_deinterlace_from_pitched_uv
	(u8* output_uv, const u8* input_uv, int pitch, int width, int height)
{
	int row, i;
	u8 * output_u = output_uv;
	u8 * output_v = output_uv + width * height;	//without padding

	for (row = 0; row < height; row++) { 		//row
		for (i = 0; i < width; i++) {
			if (yuv420_format == YV12_FORMAT) {
				// U buffer and V buffer is plane buffer
				*output_u++ = *input_uv++;   	//U Buffer
				*output_v++ =  *input_uv++;	//V buffer
			} else {
				// U buffer and V buffer is interlaced buffer
				*output_u++ = *input_uv++;
				*output_u++ = *input_uv++;
			}
		}
		input_uv += (pitch - width) * 2;		//skip padding
	}
	return 0;
}

static int save_preview_buffer(int fd, u32 count, u32 source_buffer_id)
{
	iav_yuv_buffer_info_ex_t info;
	u8 * uv_buffer_clipped = NULL;
	u8 * y_buffer_clipped = NULL;
	u32 old_pts, new_pts, pts_diff;
	int uv_pitch, uv_width, uv_height;
	int infinite_loop = 0, counter = 0;
	int yuv_format = 0;  // 0 is yuv422, 1 is yuv420
	char yuv_format_string[32];

	if (count == 0)
		infinite_loop = 1;

	y_buffer_clipped = malloc(MAX_YUV_BUFFER_SIZE);
	if (y_buffer_clipped == NULL) {
		printf("Not enough memory for preview capture !\n");
		goto error_exit;
	}
	uv_buffer_clipped = malloc(MAX_YUV_BUFFER_SIZE);
	if (uv_buffer_clipped == NULL) {
		printf("Not enough memory for preivew capture !\n");
		goto error_exit;
	}

	old_pts = new_pts = pts_diff = 0;
	yuv_format = (source_buffer_id == PREVIEW_C_BUFFER_ID) ? 1 : 0;
	while ((infinite_loop ||count --) && (!quit_yuv_stream)) {
		info.source = source_buffer_id;
		if (ioctl(fd_iav, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &info) < 0) {
			if (errno == EINTR) {
				continue;		/* back to for() */
			} else {
				perror("IAV_IOC_READ_YUV_BUFFER_INFO_EX");
				goto error_exit;
			}
		}
		if (info.pitch == info.width) {
			memcpy(y_buffer_clipped, info.y_addr, info.width * info.height);
		} else if (info.pitch > info.width) {
			remove_padding_from_pitched_y(y_buffer_clipped,
				info.y_addr, info.pitch, info.width, info.height);
		} else {
			printf("pitch size smaller than width!\n");
			goto error_exit;
		}

		//convert uv data from interleaved into planar format
		if (yuv_format == 1) {
			uv_pitch = info.pitch / 2;
			uv_width = info.width / 2;
			uv_height = info.height / 2;
		} else {
			uv_pitch = info.pitch / 2;
			uv_width = info.width / 2;
			uv_height = info.height;
		}

		remove_padding_and_deinterlace_from_pitched_uv(uv_buffer_clipped,
			info.uv_addr, uv_pitch, uv_width, uv_height);

		if (amba_transfer_write(fd, y_buffer_clipped,
			info.width* info.height, transfer_method) < 0) {
		 	perror("Failed to save preview data into file !\n");
		 	goto error_exit;
		}

		if (amba_transfer_write(fd, uv_buffer_clipped,
			uv_width * uv_height * 2, transfer_method) < 0) {
		 	perror("Failed to save preview data into file !\n");
		 	goto error_exit;
		}
		new_pts = info.PTS;
		pts_diff = new_pts - old_pts;
		old_pts = new_pts;
		if (verbose) {
//			printf("Y 0x%08x, UV 0x%08x, pitch %d, %dx%d.\n", (u32)info.y_addr,
//				(u32)info.uv_addr, info.pitch, info.width, info.height);
			printf("[%3d]   PTS [%d], diff [%d]\n",
				counter++, new_pts, pts_diff);
		}
	}

	amba_transfer_close(fd, transfer_method);
	free(y_buffer_clipped);
	free(uv_buffer_clipped);
	if (yuv_format == 0) {
		sprintf(yuv_format_string, "YV16");
	} else {
		if (yuv420_format == YV12_FORMAT)
			sprintf(yuv_format_string, "YV12");
		else
			sprintf(yuv_format_string, "NV12");
	}
	printf("save_preview_buffer done\n");
	printf("Output YUV resolution %d x %d in %s format\n",
		info.width, info.height, yuv_format_string);
	return 0;

error_exit:
	if (y_buffer_clipped)
		free(y_buffer_clipped);
	if (uv_buffer_clipped)
		free(uv_buffer_clipped);
	return -1;
}

static int save_me1_buffer(int fd_main, int fd_preview_c, u32 count)
{
	iav_me1_buffer_info_ex_t info;
	u8 * uv_buffer_clipped = NULL;
	u8 * y_buffer_clipped = NULL;
	u32 old_pts, new_pts, pts_diff;
	int infinite_loop = 0;

	if (count == 0)
		infinite_loop = 1;

	y_buffer_clipped = malloc(MAX_ME1_BUFFER_SIZE);
	if (y_buffer_clipped == NULL) {
		printf("Not enough memory for ME1 buffer capture !\n");
		goto error_exit;
	}

	//clear UV to be B/W mode, UV data is not useful for motion detection,
	//just fill UV data to make YUV to be YV12 format, so that it can play in YUV player
	uv_buffer_clipped = malloc(MAX_ME1_BUFFER_SIZE);
	if (uv_buffer_clipped == NULL) {
		printf("Not enough memory for ME1 buffer capture !\n");
		goto error_exit;
	}
	memset(uv_buffer_clipped, 0x80, MAX_ME1_BUFFER_SIZE);

	new_pts = old_pts = pts_diff = 0;
	while ((infinite_loop ||count --) && (!quit_yuv_stream)) {
		if (ioctl(fd_iav, IAV_IOC_READ_ME1_BUFFER_INFO_EX, &info) < 0) {
			if (errno == EINTR) {
				continue;		/* back to for() */
			} else {
				perror("IAV_IOC_READ_ME1_BUFFER_INFO_EX");
				goto error_exit;
			}
		}

		// main ME1 buffer dump
		if (info.main_pitch == info.main_width) {
			memcpy(y_buffer_clipped, info.main_addr,
				info.main_width * info.main_height);
		} else if (info.main_pitch > info.main_width) {
			remove_padding_from_pitched_y(y_buffer_clipped,
				info.main_addr, info.main_pitch,
				info.main_width, info.main_height);
		} else {
			printf("pitch size smaller than width!\n");
			goto error_exit;
		}
		if (amba_transfer_write(fd_main, y_buffer_clipped,
			info.main_width* info.main_height, transfer_method) <0) {
			perror("Failed to save ME1 data into file !\n");
			goto error_exit;
		}
		if (amba_transfer_write(fd_main, uv_buffer_clipped,
			info.main_width* info.main_height / 2, transfer_method) <0) {
			perror("Failed to save ME1 data into file !\n");
			goto error_exit;
		}

		// second ME1 buffer dump
		if (info.second_pitch == info.second_width) {
			memcpy(y_buffer_clipped, info.second_addr,
				info.second_width * info.second_height);
		} else if (info.second_pitch > info.second_width) {
			remove_padding_from_pitched_y(y_buffer_clipped,
				info.second_addr, info.second_pitch,
				info.second_width, info.second_height);
		} else {
			printf("pitch size smaller than width!\n");
			goto error_exit;
		}

		if (amba_transfer_write(fd_preview_c, y_buffer_clipped,
			info.second_width* info.second_height, transfer_method) <0) {
			perror("Failed to save ME1 data into file !\n");
			goto error_exit;
		}
		if (amba_transfer_write(fd_preview_c, uv_buffer_clipped,
			info.second_width* info.second_height / 2, transfer_method) <0) {
			perror("Failed to save ME1 data into file !\n");
			goto error_exit;
		}
		new_pts = info.PTS;
		pts_diff = new_pts - old_pts;
		old_pts = new_pts;
		if (verbose) {
			printf("seqnum [%d], PTS [%d], diff [%d].\n",
					info.seqnum, new_pts, pts_diff);
		}
	}

	amba_transfer_close(fd_main, transfer_method);
	amba_transfer_close(fd_preview_c, transfer_method);
	free(y_buffer_clipped);
	free(uv_buffer_clipped);
	printf("save_me1_buffer done\n");
	printf("main resolution %d x %d, preview c resolution %d x %d, in YV12 format\n",
		info.main_width, info.main_height, info.second_width, info.second_height);
	return 0;

error_exit:
	if (y_buffer_clipped)
		free(y_buffer_clipped);
	if (uv_buffer_clipped)
		free(uv_buffer_clipped);
	return -1;
}

static void run_get_preview_data(void)
{
	iav_preview_info_t info;

	info.interval = 4;
	while (1) {
		if (ioctl(fd_iav, IAV_IOC_READ_PREVIEW_INFO, &info) < 0) {
			perror("IAV_IOC_READ_PREVIEW_INFO");
			return;
		}

		printf("seqnum = %d, y = %p, uv = %p, pts = %d , pitch = %d\n",
			info.seqnum, info.y_addr, info.uv_addr, info.PTS, info.pitch);
	}
}

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "b:mif:F:tuc:v";

static struct option long_options[] = {
	{"buffer",		HAS_ARG, 0, 'b'},
	{"info",		NO_ARG, 0, 'i'},
	{"me1",		NO_ARG, 0, 'm'},		/*capture me1 buffer */
	{"filename",	HAS_ARG, 0, 'f'},		/* specify file name*/
	{"format",	HAS_ARG, 0, 'F'},
	{"tcp", 		NO_ARG, 0, 't'},
	{"usb",		NO_ARG, 0,'u'},
	{"frames",	HAS_ARG,0, 'c'},
	{"verbose",	NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"0|1", "select preview buffer id, 0 for preview C, 1 for preview A"},
	{"",	"\tshow preview yuv buffer info"},
	{"",	"\tcapture me1 buffer"},
	{"?.yuv",	"filename to store output yuv"},
	{"0|1",	"YUV420 format, 0: YV12, 1: NV12. Default is YV12 format"},
	{"",	"\tuse tcp to send data to PC"},
	{"",	"\tuse usb to send data to PC"},
	{"1~n",	"frame counts to capture"
			"\t\t\twidth x height, with pitch padding data clipping"},
	{"",	"\tprint frame pts"},
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
	printf("\n\nThis program captures YUV or ME1 buffer in YUV420 format, and save as YV12 and NV12.\n");
	printf(" YV12 format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUUUUUUUUUUUUUU\n"
 		 "\t\tUUUUUUUUUUUUUU\n"
	 	 "\t\tVVVVVVVVVVVVVV\n"
 	 	 "\t\tVVVVVVVVVVVVVV\n");
	printf("NV12 format (U and V are interleaved buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUVUVUVUVUVUVUV\n"
 		 "\t\tUVUVUVUVUVUVUV\n"
	 	 "\t\tUVUVUVUVUVUVUV\n"
 	 	 "\t\tUVUVUVUVUVUVUV\n");

	printf("\neg: To get 20 preview frames of YV12 format\n" );
	printf("    > test_yuvcap  -c 20  -f  1.yuv  -F 0 \n\n");
	printf("    To get continous preview as .yuv file of NV12 format\n" );
	printf("    > test_yuvcap -f  1.yuv  -F 1 \n\n");
	printf("    To get continous me1 as .yuv file\n" );
	printf("    > test_yuvcap -m -f 1.yuv \n\n");
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'b':
			value = atoi(optarg);
			if ((value < 0) || (value > 1)) {
				printf("Invalid preview buffer id : %d.\n", value);
				return -1;
			}
			preview_buffer_id = (value == 0) ? PREVIEW_C_BUFFER_ID : PREVIEW_A_BUFFER_ID;
			break;
		case 'c':
			frame_count = atoi(optarg);
			break;
		case 'i':
			capture_select = CAPTURE_PREVIEW_INFO;
			break;
		case 'm':
			capture_select = CAPTURE_MOTION_BUFFER;
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'F':
			value = atoi(optarg);
			if ((value != YV12_FORMAT) && (value != NV12_FORMAT)) {
				printf("Invalid yuv420 file format : %d.\n", value);
				return -1;
			}
			yuv420_format = value;
			break;
		case 't':
			transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			transfer_method = TRANS_METHOD_USB;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	if (transfer_method == TRANS_METHOD_TCP ||
			transfer_method == TRANS_METHOD_USB)
		default_filename = default_filename_tcp;
	else
		default_filename = default_filename_nfs;

	return 0;
}

static void sigstop(int a)
{
	quit_yuv_stream = 1;
}

static int check_state(void)
{
	int state;
	if (ioctl(fd_iav, IAV_IOC_GET_STATE, &state) < 0) {
		perror("IAV_IOC_GET_STATE");
		exit(2);
	}

	//if it is encoding, then stop encoding
	if ((state != IAV_STATE_PREVIEW) && state != IAV_STATE_ENCODING) {
		printf("IAV is not in preview / encoding state, cannot get yuv buf!\n");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int fd_main = -1;
	int fd_preview_c = -1;
	char filename_preview_c[256];
	char filename_main_me1[256];
	char filename_preview_c_me1[256];

	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (map_buffer() < 0)
		return -1;

	//check iav state
	if (check_state() < 0)
		return -1;

	if (amba_transfer_init(transfer_method) < 0) {
		return -1;
	}

	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	switch (capture_select) {
	case CAPTURE_PREVIEW_BUFFER:
		sprintf(filename_preview_c, "%s_preview_%c.yuv", filename,
			(preview_buffer_id == PREVIEW_C_BUFFER_ID) ? 'c' : 'a');
		fd_preview_c = amba_transfer_open(filename_preview_c,
			transfer_method, port++);
		if (fd_preview_c < 0) {
			printf("Cannot open file [%s].\n", filename_preview_c);
			return -1;
		}
		save_preview_buffer(fd_preview_c, frame_count, preview_buffer_id);
		break;

	case CAPTURE_PREVIEW_INFO:
		run_get_preview_data();
		break;

	case CAPTURE_MOTION_BUFFER:
		sprintf(filename_main_me1, "%s_main_me1.yuv", filename);
		fd_main = amba_transfer_open(filename_main_me1,
			transfer_method, port++);
		if (fd_main < 0) {
			printf("Cannot open file [%s].\n", filename_main_me1);
			return -1;
		}
		sprintf(filename_preview_c_me1, "%s_preview_c_me1.yuv", filename);
		fd_preview_c = amba_transfer_open(filename_preview_c_me1,
			transfer_method, port++);
		if (fd_preview_c < 0) {
			printf("Cannot open file [%s].\n", filename_preview_c_me1);
			return -1;
		}
		save_me1_buffer(fd_main, fd_preview_c, frame_count);
		break;

	default:
		break;
	}

	if (amba_transfer_deinit(transfer_method) < 0) {
		return -1;
	}

	close(fd_preview_c);
	close(fd_main);

	return 0;
}

