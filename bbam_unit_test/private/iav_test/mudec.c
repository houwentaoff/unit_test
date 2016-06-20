
/*
 * mudec.c
 *
 * History:
 *	2012/06/21 - [Zhi He] created file
 *
 * Description:
 *    This file is delicated for NVR(M UDEC) H264's test case
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <pthread.h>
#include <semaphore.h>

#include "types.h"
#include "ambas_common.h"
#include "ambas_vout.h"
#include "iav_drv.h"

#ifndef KB
#define KB	(1*1024)
#endif

#ifndef MB
#define MB	(1*1024*1024)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_array)	(sizeof(_array)/sizeof(_array[0]))
#endif

#include "fbtest.c"

static int vout_id[] = {0, 1};
static int vout_type[] = {AMBA_VOUT_SINK_TYPE_DIGITAL, AMBA_VOUT_SINK_TYPE_HDMI};
static int vout_index = 1;

#define NUM_VOUT	ARRAY_SIZE(vout_id)

static int vout_width[NUM_VOUT] = {1280};
static int vout_height[NUM_VOUT] = {720};

static int max_vout_width;
static int max_vout_height;

static int verbose = 0;

//static int udec_id = 0;

int mdec_pindex = 0;

#define u_assert(expr) do { \
    if (!(expr)) { \
        u_printf("assertion failed: %s\n\tAt %s : %d\n", #expr, __FILE__, __LINE__); \
    } \
} while (0)

#define u_printf_error(format,args...)  do { \
    u_printf("[Error] at %s:%d: ", __FILE__, __LINE__); \
    u_printf(format,##args); \
} while (0)

#define DATA_PARSE_MIN_SIZE 128*1024
#define MAX_DUMP_FILE_NAME_LEN 256
static unsigned char _h264_eos[8] = {0x00, 0x00, 0x00, 0x01, 0x0A, 0x0, 0x0, 0x0};

#define HEADER_SIZE 128

//some debug options
static int test_decode_one_trunk = 0;
static int test_dump_total = 0;
static int test_dump_separate = 0;
static char test_dump_total_filename[MAX_DUMP_FILE_NAME_LEN] = "./tot_dump/es_%d";
static char test_dump_separate_filename[MAX_DUMP_FILE_NAME_LEN] = "./sep_dump/es_%d_%d";
static int test_feed_background = 0;
static int feeding_sds = 1;
static int feeding_hds = 0;

//debug only
static int get_pts_from_dsp = 1;

#define MAX_FILES	8

typedef char FileName[256];
FileName file_list[MAX_FILES];
static int current_file_index = 0;
static unsigned int file_codec[MAX_FILES];
static int file_video_width[MAX_FILES];
static int file_video_height[MAX_FILES];
static int is_hd[MAX_FILES];
static int first_show_full_screen = 0;
static int first_show_hd_stream = 0;

static int mdec_loop = 1;
static unsigned int mdec_running = 1;
static int stop_background_udec = 0;

//static int pic_width;
//static int pic_height;

//udec mode configuration
static int tiled_mode = 0;
static int ppmode = 2;	// 0: no pp; 1: decpp; 2: voutpp
static int deint = 0;

static int dec_types = 0x3F;
static int max_frm_num = 8;
static int bits_fifo_size = 2*MB;
static int ref_cache_size = 0;
static int pjpeg_buf_size = 4*MB;

static int npp = 5;
//static int interlaced_out = 0;
//static int packed_out = 0;	// 0: planar yuv; 1: packed yuyv

static int add_useq_upes_header = 1;

#define _no_log(format, args...)	(void)0
//debug log
#if 1
#define u_printf_binary _no_log
#define u_printf_binary_index _no_log
#else
#define u_printf_binary u_printf
#define u_printf_binary_index u_printf_index
#endif


#define M_MSG_KILL	0
#define M_MSG_ECHO	1
#define M_MSG_START	2
#define M_MSG_RESTART	3
#define M_MSG_PAUSE	4
#define M_MSG_RESUME	5

enum {
	ACTION_NONE = 0,
	ACTION_QUIT,
	ACTION_START,
	ACTION_RESTART,
	ACTION_PAUSE,
	ACTION_RESUME,
};

enum {
	UDEC_TRICKPLAY_PAUSE = 0,
	UDEC_TRICKPLAY_RESUME = 1,
	UDEC_TRICKPLAY_STEP = 2,
};

typedef struct display_window_s {
	u8 win_id;
	u8 set_by_user;
	u8 reserved[2];

	u16 input_offset_x;
	u16 input_offset_y;
	u16 input_width;
	u16 input_height;

	u16 target_win_offset_x;
	u16 target_win_offset_y;
	u16 target_win_width;
	u16 target_win_height;
} display_window_t;

static display_window_t display_window[MAX_FILES];

//utils functions
static long long get_tick(void)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	static struct timeval start_tv;
	static int started;
	struct timeval tv;
	long long rval;

	pthread_mutex_lock(&mutex);

	if (!started) {
		started = 1;
		gettimeofday(&start_tv, NULL);
		rval = 0;
	} else {
		gettimeofday(&tv, NULL);
		rval = (1000000LL * (tv.tv_sec - start_tv.tv_sec) + (tv.tv_usec - start_tv.tv_usec)) / 1000;
	}

	pthread_mutex_unlock(&mutex);

	return rval;
}

int u_printf(const char *fmt, ...)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	va_list args;
	int rval;
	long long ticks;

	ticks = get_tick();

	pthread_mutex_lock(&mutex);

	printf("%lld  ", ticks);

	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);

	pthread_mutex_unlock(&mutex);

	return rval;
}

int u_printf_index(unsigned index, const char *fmt, ...)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	va_list args;
	int rval;
	long long ticks;

	ticks = get_tick();

	pthread_mutex_lock(&mutex);

	printf("%lld [%d] ", ticks, index);

	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);

	pthread_mutex_unlock(&mutex);

	return rval;
}

int v_printf(const char *fmt, ...)
{
	va_list args;
	int rval;
	if (verbose == 0)
		return 0;
	va_start(args, fmt);
	rval = vprintf(fmt, args);
	va_end(args);
	return rval;
}

#define Assert(_expr) \
	do { \
		if (!(_expr)) { \
			printf("Assertion failed: %s\n", #_expr); \
			printf("At line %d\n", __LINE__); \
			exit(-1); \
		} \
	} while (0)

//todo, hard code here, fix me
#define MAX_NUM_UDEC 5

#define MAX_DECODE_FRAMES	20
#define MAX_INPUT_FRAMES	24

//some utils code for USEQ/UPES Header
#define UDEC_SEQ_HEADER_LENGTH 20
#define UDEC_SEQ_HEADER_EX_LENGTH 24
#define UDEC_PES_HEADER_LENGTH 24

#define UDEC_SEQ_STARTCODE_H264 0x7D
#define UDEC_SEQ_STARTCODE_MPEG4 0xC4
#define UDEC_SEQ_STARTCODE_VC1WMV3 0x71

#define UDEC_PES_STARTCODE_H264 0x7B
#define UDEC_PES_STARTCODE_MPEG4 0xC5
#define UDEC_PES_STARTCODE_VC1WMV3 0x72

#define UDEC_PES_STARTCODE_MPEG12 0xE0

typedef struct bitswriter_t {
    unsigned char* p_start, *p_cur;
    unsigned int size;
    unsigned int left_bits;
    unsigned int left_bytes;
    unsigned int full;
} bitswriter_t;

static bitswriter_t* create_bitswriter(unsigned char* p_start, unsigned int size)
{
    bitswriter_t* p_writer = (bitswriter_t*) malloc(sizeof(bitswriter_t));

    //memset(p_start, 0x0, size);
    p_writer->p_start = p_start;
    p_writer->p_cur = p_start;
    p_writer->size = size;
    p_writer->left_bits = 8;
    p_writer->left_bytes = size;
    p_writer->full = 0;

    return p_writer;
}

static void delete_bitswriter(bitswriter_t* p_writer)
{
    free(p_writer);
}

static int write_bits(bitswriter_t* p_writer, unsigned int value, unsigned int bits)
{
    u_assert(!p_writer->full);
    u_assert(p_writer->size == (p_writer->p_cur - p_writer->p_start + p_writer->left_bytes));
    u_assert(p_writer->left_bits <= 8);

    if (p_writer->full) {
        u_assert(0);
        return -1;
    }

    while (bits > 0 && !p_writer->full) {
        if (bits <= p_writer->left_bits) {
            u_assert(p_writer->left_bits <= 8);
            *p_writer->p_cur |= (value << (32 - bits)) >> (32 - p_writer->left_bits);
            p_writer->left_bits -= bits;
            //value >>= bits;

            if (p_writer->left_bits == 0) {
                if (p_writer->left_bytes == 0) {
                    p_writer->full = 1;
                    return 0;
                }
                p_writer->left_bits = 8;
                p_writer->left_bytes --;
                p_writer->p_cur ++;
            }
            return 0;
        } else {
            u_assert(p_writer->left_bits <= 8);
            *p_writer->p_cur |= (value << (32 - bits)) >> (32 - p_writer->left_bits);
            value <<= 32 - bits + p_writer->left_bits;
            value >>= 32 - bits + p_writer->left_bits;
            bits -= p_writer->left_bits;

            if (p_writer->left_bytes == 0) {
                p_writer->full = 1;
                return 0;
            }
            p_writer->left_bits = 8;
            p_writer->left_bytes --;
            p_writer->p_cur ++;
        }

    }
    return -2;
}

static unsigned int fill_useq_header(unsigned char* p_header, unsigned int v_format, unsigned int time_scale, unsigned int frame_tick, unsigned int is_mp4s_flag, int vid_container_width, int vid_container_height)
{
    if (UDEC_MP12 == v_format) {
        return 0;
    }

    u_assert(p_header);
    u_assert(v_format >= UDEC_H264);
    u_assert(v_format <= UDEC_VC1);
    int ret = 0;

    bitswriter_t* p_writer = create_bitswriter(p_header, is_mp4s_flag?UDEC_SEQ_HEADER_EX_LENGTH:UDEC_SEQ_HEADER_LENGTH);
    u_assert(p_writer);
    memset(p_header, 0x0, is_mp4s_flag?UDEC_SEQ_HEADER_EX_LENGTH:UDEC_SEQ_HEADER_LENGTH);

    //start code prefix
    ret |= write_bits(p_writer, 0, 16);
    ret |= write_bits(p_writer, 1, 8);

    //start code
    if (v_format == UDEC_H264) {
        ret |= write_bits(p_writer, UDEC_SEQ_STARTCODE_H264, 8);
    } else if (v_format == UDEC_MP4H) {
        ret |= write_bits(p_writer, UDEC_SEQ_STARTCODE_MPEG4, 8);
    } else if (v_format == UDEC_VC1) {
        ret |= write_bits(p_writer, UDEC_SEQ_STARTCODE_VC1WMV3, 8);
    }

    //add Signature Code 0x2449504F "$IPO"
    ret |= write_bits(p_writer, 0x2449504F, 32);

    //video format
    ret |= write_bits(p_writer, v_format, 8);

    //Time Scale low
    ret |= write_bits(p_writer, (time_scale & 0xff00)>>8, 8);
    ret |= write_bits(p_writer, time_scale & 0xff, 8);

    //markbit
    ret |= write_bits(p_writer, 1, 1);

    //Time Scale high
    ret |= write_bits(p_writer, (time_scale & 0xff000000) >> 24, 8);
    ret |= write_bits(p_writer, (time_scale &0x00ff0000) >> 16, 8);

    //markbit
    ret |= write_bits(p_writer, 1, 1);

    //num units tick low
    ret |= write_bits(p_writer, (frame_tick & 0xff00)>>8, 8);
    ret |= write_bits(p_writer, frame_tick & 0xff, 8);

    //markbit
    ret |= write_bits(p_writer, 1, 1);

    //num units tick high
    ret |= write_bits(p_writer, (frame_tick & 0xff000000) >> 24, 8);
    ret |= write_bits(p_writer, (frame_tick &0x00ff0000)>> 16, 8);

    //resolution info flag(0:no res info, 1:has res info)
    ret |= write_bits(p_writer, (is_mp4s_flag &0x1), 1);

    if(is_mp4s_flag)
    {
        //pic width
        ret |= write_bits(p_writer, (vid_container_width & 0x7f00)>>8, 7);
        ret |= write_bits(p_writer, vid_container_width & 0xff, 8);

        //markbit
        ret |= write_bits(p_writer, 3, 2);

        //pic height
        ret |= write_bits(p_writer, (vid_container_height & 0x7f00)>>8, 7);
        ret |= write_bits(p_writer, vid_container_height & 0xff, 8);
    }

    //padding
    ret |= write_bits(p_writer, 0xffffffff, 20);

    u_assert(p_writer->left_bits == 0 || p_writer->left_bits == 8);
    u_assert(p_writer->left_bytes == 0);
    u_assert(!p_writer->full);
    u_assert(!ret);

    delete_bitswriter(p_writer);

    return is_mp4s_flag?UDEC_SEQ_HEADER_EX_LENGTH:UDEC_SEQ_HEADER_LENGTH;
}

static void init_upes_header(unsigned char* p_header, unsigned int v_format)
{
    u_assert(p_header);
    u_assert(v_format >= UDEC_H264);
    u_assert(v_format <= UDEC_VC1);

    memset(p_header, 0x0, UDEC_PES_HEADER_LENGTH);

    //start code prefix
    p_header[2] = 0x1;

    //start code
    if (UDEC_H264 == v_format) {
        p_header[3] = UDEC_PES_STARTCODE_H264;
    } else if (UDEC_MP4H == v_format) {
        p_header[3] = UDEC_PES_STARTCODE_MPEG4;
    } else if (UDEC_VC1 == v_format) {
        p_header[3] = UDEC_PES_STARTCODE_VC1WMV3;
    } else if(UDEC_MP12 == v_format) {
        p_header[3] = UDEC_PES_STARTCODE_MPEG12;
    } else {
        u_assert(0);
    }

    //add Signature Code 0x2449504F "$IPO"
    p_header[4] = 0x24;
    p_header[5] = 0x49;
    p_header[6] = 0x50;
    p_header[7] = 0x4F;
}

static unsigned int fill_upes_header(unsigned char* p_header, unsigned int pts_low, unsigned int pts_high, unsigned int au_datalen, unsigned int has_pts, unsigned int pts_disc)
{
    u_assert(p_header);

    int ret = 0;

    bitswriter_t* p_writer = create_bitswriter(p_header + 8, UDEC_PES_HEADER_LENGTH - 8);
    u_assert(p_writer);
    memset(p_header + 8, 0x0, UDEC_PES_HEADER_LENGTH - 8);

    ret |= write_bits(p_writer, has_pts, 1);

    if (has_pts) {
        //pts 0
        ret |= write_bits(p_writer, (pts_low & 0xff00)>>8, 8);
        ret |= write_bits(p_writer, pts_low & 0xff, 8);
        //marker bit
        ret |= write_bits(p_writer, 1, 1);
        //pts 1
        ret |= write_bits(p_writer, (pts_low & 0xff000000) >> 24, 8);
        ret |= write_bits(p_writer, (pts_low & 0xff0000) >> 16, 8);
        //marker bit
        ret |= write_bits(p_writer, 1, 1);
        //pts 2
        ret |= write_bits(p_writer, (pts_high & 0xff00)>>8, 8);
        ret |= write_bits(p_writer, pts_high & 0xff, 8);

        //marker bit
        ret |= write_bits(p_writer, 1, 1);
        //pts 3
        ret |= write_bits(p_writer, (pts_high & 0xff000000) >> 24, 8);
        ret |= write_bits(p_writer, (pts_high & 0xff0000) >> 16, 8);

        //marker bit
        ret |= write_bits(p_writer, 1, 1);

        //pts_disc
        ret |= write_bits(p_writer, pts_disc, 1);

        //padding
        ret |= write_bits(p_writer, 0xffffffff, 27);
    }

    //au_data_length low
    //AM_PRINTF("au_datalen %d, 0x%x.\n", au_datalen, au_datalen);

    ret |= write_bits(p_writer, (au_datalen & 0xff00)>>8, 8);
    ret |= write_bits(p_writer, (au_datalen & 0xff), 8);
    //marker bit
    ret |= write_bits(p_writer, 1, 1);
    //au_data_length high
    ret |= write_bits(p_writer, (au_datalen & 0x07000000) >> 24, 3);
    ret |= write_bits(p_writer, (au_datalen &0x00ff0000)>> 16, 8);
    //pading
    ret |= write_bits(p_writer, 0xf, 3);

    u_assert(p_writer->left_bits == 0 || p_writer->left_bits == 8);
    u_assert(!ret);

    ret = p_writer->size - p_writer->left_bytes + 8;

    delete_bitswriter(p_writer);
    return ret;

}

static u32 get_vout_mask(void)
{
	if (vout_index == NUM_VOUT) {
		return (1 << NUM_VOUT) - 1;
	} else {
		return 1 << vout_id[vout_index];
	}
}

static int vout_get_sink_id(int chan, int sink_type, int iav_fd)
{
	int					num;
	int					i;
	struct amba_vout_sink_info		sink_info;
	u32					sink_id = -1;

	num = 0;
	if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_NUM");
		return -1;
	}
	if (num < 1) {
		u_printf("Please load vout driver!\n");
		return -1;
	}

	for (i = num - 1; i >= 0; i--) {
		sink_info.id = i;
		if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
			perror("IAV_IOC_VOUT_GET_SINK_INFO");
			return -1;
		}

		//u_printf("sink %d is %s\n", sink_info.id, sink_info.name);

		if ((sink_info.sink_type == sink_type) &&
			(sink_info.source_id == chan))
			sink_id = sink_info.id;
	}

	//u_printf("%s: %d %d, return %d\n", __func__, chan, sink_type, sink_id);

	return sink_id;
}

static int get_single_vout_info(int index, int *width, int *height, int iav_fd)
{
	struct amba_vout_sink_info info;

	memset(&info, 0, sizeof(info));
	info.id = vout_get_sink_id(vout_id[index], vout_type[index], iav_fd);
	if (info.id < 0)
		return -1;

	u_printf("vout id: %d\n", info.id);

	if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &info) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_INFO");
		return -1;
	}

	*width = info.sink_mode.video_size.vout_width;
	*height = info.sink_mode.video_size.vout_height;

	/*u_printf("info.sink_mode.format: %d\n", info.sink_mode.format);
	if (info.sink_mode.format == AMBA_VIDEO_FORMAT_INTERLACE) {
		vout_height *= 2;
	}*/

	u_printf("vout size: %d * %d\n", *width, *height);
	if (*width == 0 || *height == 0) {
		*width = 1280;
		*height = 720;
	}

	return 0;
}

static int ioctl_enter_idle(int iav_fd)
{
	if (ioctl(iav_fd, IAV_IOC_ENTER_IDLE, 0) < 0) {
		perror("IAV_IOC_ENTER_IDLE");
		return -1;
	}
	u_printf("enter idle done\n");
	return 0;
}

void init_udec_mode_config(iav_udec_mode_config_t *udec_mode)
{
	memset(udec_mode, 0, sizeof(*udec_mode));

	udec_mode->postp_mode = ppmode;
	udec_mode->enable_deint = deint;

	udec_mode->pp_chroma_fmt_max = 2; // 4:2:2
	udec_mode->pp_max_frm_width = max_vout_width;	// vout_width;
	udec_mode->pp_max_frm_height = max_vout_height;	// vout_height;

	udec_mode->pp_max_frm_num = npp;

	udec_mode->vout_mask = get_vout_mask();
}

static void init_udec_configs(iav_udec_config_t *udec_config, int nr, u16 max_width, u16 max_height)
{
	int i;

	memset(udec_config, 0, sizeof(iav_udec_config_t) * nr);

	for (i = 0; i < nr; i++, udec_config++) {
		udec_config->tiled_mode_allowed = tiled_mode;
		udec_config->frm_chroma_fmt_max = 1;	// 4:2:0
		udec_config->dec_types = dec_types;
		udec_config->max_frm_num = max_frm_num; // MAX_DECODE_FRAMES - todo
		udec_config->max_frm_width = max_width;	// todo
		udec_config->max_frm_height = max_height;	// todo
		udec_config->max_fifo_size = bits_fifo_size;
	}
}

static void init_udec_windows(udec_window_t *win, int start_index, int nr, int src_width, int src_height)
{
	//int voutA_width = 0;
	//int voutA_height = 0;
	int voutB_width = 0;
	int voutB_height = 0;

	int display_width = 0;
	int display_height = 0;

	u32 vout_mask = get_vout_mask();
	int rows;
	int cols;
	int rindex;
	int cindex;
	int i;
	u32 win_id;

	udec_window_t *window = win + start_index;

	//LCD not used yet
#if 0
	if (vout_mask & 1) {
		voutA_width = vout_width[0];
		voutA_height = vout_height[0];
		u_printf("Error: M UDEC not support LCD now!!!!\n");
	}
#endif

	if (vout_mask & 2) {
		voutB_width = vout_width[1];
		voutB_height = vout_height[1];
	} else {
		u_printf("Error: M UDEC must have HDMI configured!!!!\n");
	}

	if (nr == 1) {
		rows = 1;
		cols = 1;
	} else if (nr <= 4) {
		rows = 2;
		cols = 2;
	} else if (nr <= 9) {
		rows = 3;
		cols = 3;
	} else {
		rows = 4;
		cols = 4;
	}

	memset(window, 0, sizeof(udec_window_t) * nr);

	rindex = 0;
	cindex = 0;

	display_width = voutB_width;
	display_height = voutB_height;

	if (!display_width || !display_height) {
		u_printf("why vout width %d, vout height %d are zero?\n");
		display_width = 1280;
		display_height = 720;
	}

	for (i = 0; i < nr; i++, window++) {
		win_id = i + start_index;
		window->win_config_id = win_id;

		u_assert(win_id < MAX_FILES);
		if (win_id < MAX_FILES) {
			if (display_window[win_id].set_by_user) {
				u_printf("%d'th window set by user, off %d,%d, size %d,%d.\n", win_id, display_window[win_id].target_win_offset_x, display_window[win_id].target_win_offset_y, display_window[win_id].target_win_width, display_window[win_id].target_win_height);
				window->target_win_offset_x = display_window[win_id].target_win_offset_x;
				window->target_win_offset_y = display_window[win_id].target_win_offset_y;
				window->target_win_width = display_window[win_id].target_win_width;
				window->target_win_height = display_window[win_id].target_win_height;
			} else {
				window->target_win_offset_x = cindex * display_width / cols;
				window->target_win_offset_y = rindex * display_height / rows;
				window->target_win_width = display_width / cols;
				window->target_win_height = display_height / rows;
				display_window[win_id].win_id = win_id;
				display_window[win_id].target_win_offset_x = window->target_win_offset_x;
				display_window[win_id].target_win_offset_y = window->target_win_offset_y;
				display_window[win_id].target_win_width = window->target_win_width;
				display_window[win_id].target_win_height = window->target_win_height;
			}
		}

		window->input_offset_x = 0;
		window->input_offset_y = 0;
		window->input_width = src_width;
		window->input_height = src_height;

#if 0
		window->target_win_offset_x = cindex * display_width / cols;
		window->target_win_offset_y = rindex * display_height / rows;
		window->target_win_width = display_width / cols;
		window->target_win_height = display_height / rows;
#endif

		if (++cindex == cols) {
			cindex = 0;
			rindex++;
		}
	}
}

static void init_udec_renders(udec_render_t *render, int nr)
{
	int i;
	memset(render, 0, sizeof(udec_render_t) * nr);

	for (i = 0; i < nr; i++, render++) {
		render->render_id = i;
		render->win_config_id = i;
		render->win_config_id_2nd = 0xff;//hard code
		render->udec_id = i;
	}
}

static void init_udec_renders_single(udec_render_t *render, u8 render_id, u8 win_id, u8 sec_win_id, u8 udec_id)
{
	memset(render, 0, sizeof(udec_render_t));

	render->render_id = render_id;
	render->win_config_id = win_id;
	render->win_config_id_2nd = sec_win_id;
	render->udec_id = udec_id;
}

static int init_mdec_params(int argc, char **argv)
{
	int i = 0;
	int ret = 0;
	int started_scan_filename = 0;
	u32 param0, param1, param2, param3, param4;

	//parse options
	for (i=1; i<argc; i++) {
		if (!strcmp("--loop", argv[i])) {
			mdec_loop = 1;
			u_printf("[input argument] --loop, enable loop feeding data.\n");
		} else if (!strcmp("--noloop", argv[i])) {
			mdec_loop = 0;
			u_printf("[input argument] --noloop, disable loop feeding data.\n");
		} else if (!strcmp("--tilemode", argv[i])) {
			tiled_mode = 1;
			u_printf("[input argument] --tilemode.\n");
		} else if (!strcmp("--stopbackground", argv[i])) {
			stop_background_udec = 1;
			u_printf("[input argument] --stopbackground, stop background udec instance.\n");
		} else if (!strcmp("--maxbuffernumber", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				if (ret < 4 || ret  > 20) {
					u_printf_error("[input argument error]: '--maxbuffernumber', number(%d) should not less than 4, or greate than 20.\n", ret);
				} else {
					max_frm_num = ret;
					u_printf("[input argument]: '--maxbuffernumber', number is (%d).\n", ret);
				}
			} else {
				u_printf_error("[input argument error]: '--maxbuffernumber', should follow with integer, argc %d, i %d.\n", argc, i);
			}
			i ++;
		} else if (!strcmp("--fifosize", argv[i])) {
			if (((i + 1) < argc) && (1 != sscanf(argv[i + 1], "%d", &ret))) {
				if (ret < 1 || ret  > 4) {
					u_printf_error("[input argument error]: '--fifosize', fifosize(%d MB) should not less than 1MB, or greate than 4MB.\n", ret);
				} else {
					bits_fifo_size = ret * MB;
					u_printf("[input argument]: '--fifosize', bits fifo size is (%d MB).\n", ret);
				}
			} else {
				u_printf_error("[input argument error]: '--fifosize', should follow with integer.\n");
			}
			i ++;
		} else if (!strcmp("--nopts", argv[i])) {
			get_pts_from_dsp = 0;
			u_printf("[input argument] --nopts, disable get pts from driver.\n");
		} else if (!strcmp("--pts", argv[i])) {
			get_pts_from_dsp = 1;
			u_printf("[input argument] --pts, enable get pts from driver.\n");
		} else if (!strcmp("--dumpt", argv[i])) {
			test_dump_total = 1;
			u_printf("[input argument] --dumpt, dump to tatal file, default to ./tot_dump/.\n");
		} else if (!strcmp("--dumps", argv[i])) {
			test_dump_separate = 1;
			u_printf("[input argument] --dumps, dump to separate file, default to ./sep_dump/.\n");
		} else if (!strcmp("--oneshot", argv[i])) {
			test_decode_one_trunk = 1;
			u_printf("[input argument] --oneshot, only decode one data block(not greater than 8M).\n");
		} else if (!strcmp("--feedbackground", argv[i])) {
			test_feed_background = 1;
			u_printf("[input argument] --feedbackground, will feed all udec instance.\n");
		} else if (!strcmp("--usequpes", argv[i])) {
			add_useq_upes_header = 1;
			u_printf("[input argument] --usequpes, will add USEQ/UPES header.\n");
		} else if (!strcmp("--nousequpes", argv[i])) {
			add_useq_upes_header = 0;
			u_printf("[input argument] --nousequpes, will not add USEQ/UPES header.\n");
		} else if(!strcmp("-f", argv[i])) {
			if (started_scan_filename) {
				current_file_index ++;
			}
			if (current_file_index >= MAX_FILES) {
				u_printf("max file number(current index %d, max value %d).\n", current_file_index, MAX_FILES);
				return (-1);
			}
			memcpy(&file_list[current_file_index][0], argv[i+1], sizeof(file_list[current_file_index]) - 1);
			file_list[current_file_index][sizeof(file_list[current_file_index]) - 1] = 0x0;
			//set default values
			file_codec[current_file_index] = UDEC_H264;
			file_video_width[current_file_index] = 720;
			file_video_height[current_file_index] = 480;
			is_hd[current_file_index] = 0;
			started_scan_filename = 1;
			i ++;
			u_printf("[input argument] -f[%d]: %s.\n", current_file_index, file_list[current_file_index]);
		} else if (!strcmp("-c", argv[i])) {
			if (!strcmp("h264", argv[i + 1])) {
				file_codec[current_file_index] = UDEC_H264;
				u_printf("[input argument] -c[%d]: H264.\n", current_file_index);
			} else if (!strcmp("mpeg12", argv[i + 1])) {
				file_codec[current_file_index] = UDEC_MP12;
				u_printf("[input argument] -c[%d]: MPEG12.\n", current_file_index);
			} else if (!strcmp("mpeg4", argv[i + 1])) {
				file_codec[current_file_index] = UDEC_MP4H;
				u_printf("[input argument] -c[%d]: MPEG4.\n", current_file_index);
			} else if (!strcmp("vc1", argv[i + 1])) {
				file_codec[current_file_index] = UDEC_VC1;
				u_printf("[input argument] -c[%d]: VC-1.\n", current_file_index);
			} else {
				u_printf("bad codec string %s, -c should followed by 'h264' 'mpeg12' 'mpeg4' or 'vc1' .\n", argv[i + 1]);
				return (-2);
			}
			i ++;
		} else if (!strcmp("-s", argv[i])) {
			ret = sscanf(argv[i+1], "%dx%d", &file_video_width[current_file_index], &file_video_height[current_file_index]);
			if (2 != ret) {
				u_printf_error("[input argument error], '-s' should be followed by video width x height.\n");
				return (-3);
			}
			u_printf("[input argument] -s[%d]: %dx%d.\n", current_file_index, file_video_width[current_file_index], file_video_height[current_file_index]);
			i ++;
			if ((720 == file_video_width[current_file_index]) && (480 == file_video_height[current_file_index])) {
				//sd
				is_hd[current_file_index] = 0;
			} else if ((800 == file_video_width[current_file_index]) && (480 == file_video_height[current_file_index])) {
				//sd
				is_hd[current_file_index] = 0;
			} else if ((960 == file_video_width[current_file_index]) && (540 == file_video_height[current_file_index])) {
				//sd
				is_hd[current_file_index] = 0;
			} else if ((1280 == file_video_width[current_file_index]) && (720 == file_video_height[current_file_index])) {
				//hd
				is_hd[current_file_index] = 1;
			} else if ((1920 == file_video_width[current_file_index]) && (1080 == file_video_height[current_file_index])) {
				//full hd
				is_hd[current_file_index] = 1;
			} else if ((1600 == file_video_width[current_file_index]) && (1200 == file_video_height[current_file_index])) {
				//full hd
				is_hd[current_file_index] = 1;
			} else {
				u_printf("!!!NOT supported resolution, yet.\n");
			}
		} else if (!strcmp("-d", argv[i])) {
			ret = sscanf(argv[i+1], "%d:%d,%d,%d,%d", &param0, &param1, &param2, &param3, &param4);
			if (5 != ret) {
				u_printf_error("[input argument error], '-d' should be followed by 'win_id:off_x,off_y,size_x,size_y'.\n");
				return (-3);
			}
			u_printf("[input argument] -d %d:%d,%d,%d,%d.\n", param0, param1, param2, param3, param4);
			if (param0 >= MAX_FILES) {
				u_printf_error("[input argument error], '-d' win_id(%d) should less than %d.\n", param0, MAX_FILES);
			} else {
				if (((param0 + param2) > vout_width[0]) || ((param1 + param3) > vout_height[0])) {
					u_printf_error("[input argument error], '-d' win argument invalid, off_x(%d) + size_x(%d) = %d, width %d, off_y(%d) + size_y(%d) = %d, height %d.\n", param0, param2, param0 + param2, vout_width[1], param1, param3, param1 + param3, vout_height[1]);
				}
			}
			display_window[param0].set_by_user = 1;
			display_window[param0].win_id = param0;
			display_window[param0].target_win_offset_x = param1;
			display_window[param0].target_win_offset_y = param2;
			display_window[param0].target_win_width = param3;
			display_window[param0].target_win_height = param4;
			i ++;
		} else {
			u_printf("NOT processed option(%s).\n", argv[i]);
		}
	}
	return 0;
}

static int do_test_mdec(int iav_fd);

static int open_iav(void)
{
	int iav_fd = -1;
	if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	return iav_fd;
}

////////////////////////////////////////////////////////////////////////////////////

typedef struct msg_s {
	int	cmd;
	void	*ptr;
	u8 arg1, arg2, arg3, arg4;
} msg_t;

#define MAX_MSGS	8

typedef struct msg_queue_s {
	msg_t	msgs[MAX_MSGS];
	int	read_index;
	int	write_index;
	int	num_msgs;
	int	num_readers;
	int	num_writers;
	pthread_mutex_t mutex;
	pthread_cond_t cond_read;
	pthread_cond_t cond_write;
	sem_t	echo_event;
} msg_queue_t;

msg_queue_t renderer_queue;
pthread_t renderer_thread_id;

int msg_queue_init(msg_queue_t *q)
{
	q->read_index = 0;
	q->write_index = 0;
	q->num_msgs = 0;
	q->num_readers = 0;
	q->num_writers = 0;
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->cond_read, NULL);
	pthread_cond_init(&q->cond_write, NULL);
	sem_init(&q->echo_event, 0, 0);
	return 0;
}

void msg_queue_get(msg_queue_t *q, msg_t *msg)
{
	pthread_mutex_lock(&q->mutex);
	while (1) {
		if (q->num_msgs > 0) {
			*msg = q->msgs[q->read_index];

			if (++q->read_index == MAX_MSGS)
				q->read_index = 0;

			q->num_msgs--;

			if (q->num_writers > 0) {
				q->num_writers--;
				pthread_cond_signal(&q->cond_write);
			}

			pthread_mutex_unlock(&q->mutex);
			return;
		}

		q->num_readers++;
		pthread_cond_wait(&q->cond_read, &q->mutex);
	}
}

int msg_queue_peek(msg_queue_t *q, msg_t *msg)
{
	int ret = 0;
	pthread_mutex_lock(&q->mutex);

	if (q->num_msgs > 0) {
		*msg = q->msgs[q->read_index];

		if (++q->read_index == MAX_MSGS)
			q->read_index = 0;

		q->num_msgs--;

		if (q->num_writers > 0) {
			q->num_writers--;
			pthread_cond_signal(&q->cond_write);
		}

		ret = 1;
	}

	pthread_mutex_unlock(&q->mutex);
	return ret;
}


void msg_queue_put(msg_queue_t *q, msg_t *msg)
{
	pthread_mutex_lock(&q->mutex);
	while (1) {
		if (q->num_msgs < MAX_MSGS) {
			q->msgs[q->write_index] = *msg;

			if (++q->write_index == MAX_MSGS)
				q->write_index = 0;

			q->num_msgs++;

			if (q->num_readers > 0) {
				q->num_readers--;
				pthread_cond_signal(&q->cond_read);
			}

			pthread_mutex_unlock(&q->mutex);
			return;
		}

		q->num_writers++;
		pthread_cond_wait(&q->cond_write, &q->mutex);
	}
}

void msg_queue_ack(msg_queue_t *q)
{
	sem_post(&q->echo_event);
}

void msg_queue_wait(msg_queue_t *q)
{
	sem_wait(&q->echo_event);
}

//for thread safe, members in this struct cannot be used as inter-thread communication.
//it's better only the udec_instance thread access this struct after it spawn.
typedef struct udec_instance_param_s
{
	unsigned int	udec_index;
	int	iav_fd;
	unsigned int	udec_type;
	int	pic_width, pic_height;
	unsigned int	request_bsb_size;
	FILE*	file_fd[2];
	unsigned char	loop;
	unsigned char	wait_cmd_begin;
	unsigned char	wait_cmd_exit;

	iav_udec_vout_config_t* p_vout_config;
	unsigned int	num_vout;

	msg_queue_t	cmd_queue;

	//pts related
	unsigned long long cur_feeding_pts;
	unsigned long long last_display_pts;
	unsigned int frame_duration;

	unsigned int frame_tick;
	unsigned int time_scale;

	unsigned char useq_buffer[UDEC_SEQ_HEADER_EX_LENGTH + 4];// + 4 for safe
	unsigned char upes_buffer[UDEC_PES_HEADER_LENGTH + 4];// + 4 for safe

	unsigned int useq_header_len;

	unsigned char seq_header_sent;
	unsigned char last_pts_from_dsp_valid;
	unsigned char paused;
	unsigned char trickplay_mode;
} udec_instance_param_t;

static int ioctl_init_udec_instance(iav_udec_info_ex_t* info, int iav_fd, iav_udec_vout_config_t* vout_config, unsigned int num_vout, int udec_index, unsigned int request_bsb_size, int udec_type, int deint, int pic_width, int pic_height)
{
	u_printf("init udec %d, type %d\n", udec_index, udec_type);
	u_assert(UDEC_H264 == udec_type);

	info->udec_id = udec_index;
	info->udec_type = udec_type;
	info->enable_pp = 1;
	info->enable_deint = deint;
	info->interlaced_out = 0;
	info->packed_out = 0;

	info->vout_configs.num_vout = num_vout;
	info->vout_configs.vout_config = vout_config;

	info->vout_configs.first_pts_low = 0;
	info->vout_configs.first_pts_high = 0;

	info->vout_configs.input_center_x = pic_width / 2;
	info->vout_configs.input_center_y = pic_height / 2;

	info->bits_fifo_size = request_bsb_size;
	info->ref_cache_size = 0;

	switch (udec_type) {
	case UDEC_H264:
		info->u.h264.pjpeg_buf_size = pjpeg_buf_size;//hard code here
		break;

	default:
		u_printf("udec type %d not implemented\n", udec_type);
		return -1;
	}

	if (ioctl(iav_fd, IAV_IOC_INIT_UDEC, info) < 0) {
		perror("IAV_IOC_INIT_UDEC");
		return -1;
	}

	return 0;
}

static unsigned char* copy_to_bsb(unsigned char *p_bsb_cur, unsigned char *buffer, unsigned int size, unsigned char* p_bsb_start, unsigned char* p_bsb_end)
{
	//u_printf("copy to bsb %d, %x.\n", size, *buffer);
	if (p_bsb_cur + size <= p_bsb_end) {
		memcpy(p_bsb_cur, buffer, size);
		return p_bsb_cur + size;
	} else {
		//u_printf("-------wrap happened--------\n");
		int room = p_bsb_end - p_bsb_cur;
		unsigned char *ptr2;
		memcpy(p_bsb_cur, buffer, room);
		ptr2 = buffer + room;
		size -= room;
		memcpy(p_bsb_start, ptr2, size);
		return p_bsb_start + size;
	}
}

static unsigned int get_next_start_code(unsigned char* pstart, unsigned char* pend, unsigned int esType)
{
	unsigned int size = 0;
	unsigned char* pcur = pstart;
	unsigned int state = 0;

	int	parse_by_amba = 0;
	unsigned char	nal_type;

	//amba dsp defined wrapper
	unsigned char code1, code2;

	//spec defined
	unsigned char code1o0 = 0xff, code2o0 = 0xff;
	unsigned char code1o1 = 0xff, code2o1 = 0xff;
	unsigned int cnt = 2;//find start code cnt

	switch (esType) {
		case 1://UDEC_H264:
			code1 = 0x7D;
			code2 = 0x7B;
			break;
		case 2://UDEC_MP12:
			code1 = 0xB3;
			code2 = 0x00;
			code1o0 = 0xB3;
			code1o1 = 0xB8;
			code2o0 = 0x0;
			code2o1 = 0x0;
			break;
		case 3://UDEC_MP4H:
			//code1 = 0xB8;
			//code2 = 0xB7;
			code1 = 0xC4;
			code2 = 0xC5;
			code1o0 = 0xB6;
			code1o1 = 0xB6;
			code2o0 = 0xB6;
			code2o1 = 0xB6;
			break;
		case 5://UDEC_VC1:
			code1 = 0x71;
			code2 = 0x72;
			code1o0 = 0x0F;
			code1o1 = 0x0E;
			code2o0 = 0x0C;
			code2o1 = 0x0D;
			break;
		default:
			u_printf("not supported es type.\n");
			return pend - pstart;
	}

	while (pcur <= pend) {
		switch (state) {
			case 0:
				if (*pcur++ == 0x0)
					state = 1;
				break;
			case 1://0
				if (*pcur++ == 0x0)
					state = 2;
				else
					state = 0;
				break;
			case 2://0 0
				if (*pcur == 0x1)
					state = 3;
				else if (*pcur != 0x0)
					state = 0;
				pcur ++;
				break;
			case 3://0 0 1
				if (*pcur == code2) { //pic header
					parse_by_amba = 1;
					if (cnt == 1) {
						size = pcur - 3 - pstart;
						return size;
					}
					cnt --;
					state = 0;
				} else if (*pcur == code1) { //seq header
					parse_by_amba = 1;
					if (cnt == 1) {
						size = pcur - 3 - pstart;
						return size;
					}
					state = 0;
				} else if ((!parse_by_amba) && (UDEC_H264 == esType)) { //nal uint type
					nal_type = (*pcur) & 0x1F;
					if (nal_type >= 1 && nal_type <= 5) {
						if (--cnt == 0)
							return pcur - 3 - pstart;
						else
							state = 0;
					} else if (cnt == 1 && nal_type >= 6 && nal_type <= 9) {
						// left SPS and PPS be prefix
						return pcur - 3 - pstart;
					} else if (*pstart == 0x00) {
						state = 1;
					} else {
						state = 0;
					}
				} else if ((!parse_by_amba) && (*pcur == code2o0 || *pcur == code2o1)) { //original pic header
					if (cnt == 1) {
						size = pcur - 3 - pstart;
						return size;
					}
					cnt --;
					state = 0;
				} else if ((!parse_by_amba) && (*pcur == code1o0 || *pcur == code1o1)) { //original seq header
					if (cnt == 1) {
						size = pcur - 3 - pstart;
						return size;
					}
					state = 0;
				} else if (*pcur){
					state = 0;
				} else {
					state = 1;
				}
				pcur ++;
				break;
		}
	}
	u_printf("cannot find start code, return 0, bit-stream end?\n");
	//size = pend - pstart;
	return 0;
}

static unsigned int total_frame_count(unsigned char * start, unsigned char * end, unsigned int udec_type)
{
	unsigned int totsize = 0;
	unsigned int size = 0;
	unsigned int frames_cnt = 0;
	unsigned char* p_cur = start;

	while (p_cur < end) {
		//u_printf("while loop 1 p_cur %p, end %p.\n", p_cur, end);
		size = get_next_start_code(p_cur, end, udec_type);
		if (!size) {
			frames_cnt ++;
			break;
		}
		totsize += size;
		p_cur = start + totsize;
		frames_cnt ++;
	}
	return frames_cnt;
}

static int request_bits_fifo(int iav_fd, unsigned int udec_index, unsigned int size, unsigned char* p_bsb_cur)
{
	iav_wait_decoder_t wait;
	int ret;
	wait.emptiness.room = size + 256;//for safe
	wait.emptiness.start_addr = p_bsb_cur;

	wait.flags = IAV_WAIT_BITS_FIFO;
	wait.decoder_id = udec_index;

	if ((ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
		u_printf("[error]: IAV_IOC_WAIT_DECODER fail, ret %d.\n", ret);
		return ret;
	}

	return 0;
}

static int process_cmd(msg_queue_t* queue, msg_t* msg)
{
	int ret = ACTION_NONE;
	switch (msg->cmd) {
		case M_MSG_KILL:
			ret = ACTION_QUIT;
			break;
		case M_MSG_ECHO:
			msg_queue_ack(queue);
			break;
		case M_MSG_START:
			ret = ACTION_START;
			break;
		case M_MSG_RESTART:
			ret = ACTION_RESTART;
			break;
		case M_MSG_RESUME:
			ret = ACTION_RESUME;
			break;
		case M_MSG_PAUSE:
			ret = ACTION_PAUSE;
			break;
		default:
			u_printf_error("Bad msg.cmd %d.\n", msg->cmd);
			break;
	}
	return ret;
}

static void _write_data_ring(FILE* file, unsigned char* p_buf_start, unsigned char* p_buf_end, unsigned char* p_start, unsigned char* p_end)
{
	if (p_end > p_start) {
		fwrite(p_start, 1, p_end - p_start, file);
	} else {
		if (p_buf_end > p_start) {
			fwrite(p_start, 1, p_buf_end - p_start, file);
		}
		if (p_end > p_buf_start) {
			fwrite(p_buf_start, 1, p_end - p_buf_start, file);
		}
	}
}

static void dump_binary_new_file(char* filename, unsigned int udec_index, unsigned int file_index, unsigned char* p_buf_start, unsigned char* p_buf_end, unsigned char* p_start, unsigned char* p_end)
{
	char* file_name = (char*)malloc(strlen(filename) + 32 + 32);
	if (!file_name) {
		u_printf("malloc fail.\n");
		return;
	}
	sprintf(file_name, "%s_%d_%d", filename, udec_index, file_index);
	FILE* file = fopen(file_name, "wb");

	if (file) {
		_write_data_ring(file, p_buf_start, p_buf_end, p_start, p_end);
		fclose(file);
	} else {
		u_printf("open file fail, %s.\n", file_name);
	}
	free(file_name);
}

//obsolete
#if 0
static void udec_get_last_display_pts(int iav_fd, udec_instance_param_t* udec_param, iav_udec_status_t* status)
{
	int ret;
	if (!get_pts_from_dsp) {
		return;
	}

	ret = ioctl(iav_fd, IAV_IOC_WAIT_UDEC_STATUS, status);
	if (ret < 0) {
		perror("IAV_IOC_WAIT_UDEC_STATUS");
		u_printf_error("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", ret);
		if ((-EPERM) == ret) {
			//to do
		} else {
			//to do
		}
		return;
	}

	if (status->only_query_current_pts) {
		udec_param->last_pts_from_dsp_valid = 1;
		udec_param->last_display_pts = ((unsigned long long)status->pts_high << 32) | (unsigned long long)status->pts_low;
	} else {
		//restore query flag
		status->only_query_current_pts = 1;
	}
}
#endif

static int query_udec_last_display_pts(int iav_fd, unsigned int udec_index, unsigned long long* pts)
{
	int ret;
	iav_udec_status_t status;

	memset(&status, 0x0, sizeof(status));

	status.decoder_id = udec_index;
	status.only_query_current_pts = 1;

	ret = ioctl(iav_fd, IAV_IOC_WAIT_UDEC_STATUS, &status);
	if (ret < 0) {
		perror("IAV_IOC_WAIT_UDEC_STATUS");
		u_printf_error("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", ret);
		return ret;
	}

	if (status.only_query_current_pts) {
		*pts = ((unsigned long long)status.pts_high << 32) | (unsigned long long)status.pts_low;
		return 0;
	}

	return (-1);
}

static int ioctl_udec_trickplay(int iav_fd, unsigned int udec_index, unsigned int trickplay_mode)
{
	iav_udec_trickplay_t trickplay;
	int ret;

	trickplay.decoder_id = udec_index;
	trickplay.mode = trickplay_mode;
	u_printf("[flow cmd]: before IAV_IOC_UDEC_TRICKPLAY, udec(%d), trickplay_mode(%d).\n", udec_index, trickplay_mode);
	ret = ioctl(iav_fd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
	if (ret < 0) {
		perror("IAV_IOC_UDEC_TRICKPLAY");
		u_printf_error("!!!!!IAV_IOC_UDEC_TRICKPLAY error, ret %d.\n", ret);
		if ((-EPERM) == ret) {
			//to do
		} else {
			//to do
		}
		return ret;
	}
	u_printf("[flow cmd]: IAV_IOC_UDEC_TRICKPLAY done, udec(%d), trickplay_mode(%d).\n", udec_index, trickplay_mode);
	return 0;
}

static void udec_trickplay(udec_instance_param_t* udec_param, unsigned int trickplay_mode)
{
	int ret;

	if ((UDEC_TRICKPLAY_PAUSE == trickplay_mode) || (UDEC_TRICKPLAY_RESUME == trickplay_mode)) {
		if (udec_param->trickplay_mode == trickplay_mode) {
			u_printf("[warnning]: udec(%d) is already paused/resumed(%d).\n", udec_param->udec_index, trickplay_mode);
		} else {
			ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, trickplay_mode);
			if (!ret) {
				udec_param->trickplay_mode = trickplay_mode;
			}
		}
	} else if (UDEC_TRICKPLAY_STEP == trickplay_mode) {
		ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, trickplay_mode);
		if (!ret) {
			udec_param->trickplay_mode = trickplay_mode;
		}
	} else {
		u_printf_error("BAD trickplay cmd for udec(%d). trickplay_mode %d.\n", udec_param->udec_index, trickplay_mode);
	}

}

static int ioctl_udec_stop(int iav_fd, unsigned int udec_index, unsigned int stop_flag)
{
	int ret;
	unsigned int stop_code = (stop_flag << 24) | udec_index;

	u_printf("[flow cmd]: before IAV_IOC_UDEC_STOP, udec(%d), stop_flag(%d).\n", udec_index, stop_flag);
	ret = ioctl(iav_fd, IAV_IOC_UDEC_STOP, stop_code);
	if (ret < 0) {
		perror("IAV_IOC_UDEC_STOP");
		u_printf_error("!!!!!IAV_IOC_UDEC_STOP error, ret %d.\n", ret);
		return ret;
	}
	u_printf("[flow cmd]: IAV_IOC_UDEC_STOP done, udec(%d), stop_flag(%d).\n", udec_index, stop_flag);

	return 0;
}

static void udec_pause_resume(udec_instance_param_t* udec_param)
{
	int ret;

	if (UDEC_TRICKPLAY_PAUSE == udec_param->trickplay_mode) {
		//resume
		ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, UDEC_TRICKPLAY_RESUME);
		if (!ret) {
			udec_param->trickplay_mode = UDEC_TRICKPLAY_RESUME;
		}
	} else if (UDEC_TRICKPLAY_RESUME == udec_param->trickplay_mode) {
		//pause
		ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, UDEC_TRICKPLAY_PAUSE);
		if (!ret) {
			udec_param->trickplay_mode = UDEC_TRICKPLAY_PAUSE;
		}
	} else if (UDEC_TRICKPLAY_STEP == udec_param->trickplay_mode) {
		//resume
		ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, UDEC_TRICKPLAY_RESUME);
		if (!ret) {
			udec_param->trickplay_mode = UDEC_TRICKPLAY_RESUME;
		}
	} else {
		u_printf_error("BAD trickplay mode for udec(%d). trickplay_mode %d.\n", udec_param->udec_index, udec_param->trickplay_mode);
	}
}

static void udec_step(udec_instance_param_t* udec_param)
{
	int ret;
	ret = ioctl_udec_trickplay(udec_param->iav_fd, udec_param->udec_index, UDEC_TRICKPLAY_STEP);
	if (!ret) {
		udec_param->trickplay_mode = UDEC_TRICKPLAY_STEP;
	}
}

static int udec_instance_decode_es_file(unsigned int udec_index, int iav_fd, FILE* file_fd, unsigned int udec_type, unsigned char* p_bsb_start, unsigned char* p_bsb_end, unsigned char* p_bsb_cur, msg_queue_t *cmd_queue, udec_instance_param_t* udec_param)
{
	int ret = 0;

	FILE* p_dump_file = NULL;
	char dump_file_name[MAX_DUMP_FILE_NAME_LEN + 64];
	unsigned int dump_separate_file_index = 0;

	unsigned char *p_frame_start;
	iav_udec_decode_t dec;

	//read es file
	unsigned char* p_es = NULL, *p_es_end;
	unsigned char* p_cur = NULL;
	unsigned int size = 0;
	unsigned int totsize = 0;
	unsigned int bytes_left_in_file = 0;
	unsigned int sendsize = 0;
	unsigned int pes_header_len = 0;

	unsigned int mem_size;
	FILE* pFile = file_fd;
	msg_t msg;

	//iav_udec_status_t status;

	if (!pFile) {
		u_printf("NULL input file.\n");
		return (-9);
	}

	if (test_dump_total) {
		snprintf(dump_file_name, MAX_DUMP_FILE_NAME_LEN + 60, test_dump_total_filename, udec_index);
		p_dump_file = fopen(dump_file_name, "wb");
	}

	fseek(pFile, 0L, SEEK_END);
	totsize = ftell(pFile);
	u_printf_index(udec_index, "	file total size %d.\n", totsize);

	if (totsize > 8*1024*1024) {
		mem_size = 8*1024*1024;
	} else {
		mem_size = totsize;
	}

	u_assert(!p_es);
	p_es = (unsigned char*)malloc(mem_size);

	if (!p_es) {
		u_printf_index(udec_index, "[error]: cannot alloc buffer.\n");
		if (p_dump_file) {
			fclose(p_dump_file);
		}
		return (-1);
	}

repeat_feeding:

	//memset(&status, 0x0, sizeof(status));
	//status.decoder_id = udec_index;
	//status.only_query_current_pts = 1;

	bytes_left_in_file = totsize;
	fseek(pFile, 0L, SEEK_SET);

	u_assert(p_es);

	size = mem_size;
	fread(p_es, 1, size, pFile);
	bytes_left_in_file -= size;

	//send data
	p_es_end = p_es + size;
	p_cur = p_es;

	while (1) {

		while (size > (DATA_PARSE_MIN_SIZE)) {

			while (msg_queue_peek(cmd_queue, &msg)) {
				ret = process_cmd(cmd_queue, &msg);

				if (ACTION_QUIT == ret) {
					u_printf("recieve quit cmd, return.\n");
					if (p_dump_file) {
						fclose(p_dump_file);
					}
					free(p_es);
					return (-5);
				} else if (ACTION_RESUME  == ret) {
					udec_param->paused = 0;
					//udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
				} else if (ACTION_PAUSE  == ret) {
					udec_param->paused = 1;
					//udec_trickplay(udec_param, UDEC_TRICKPLAY_PAUSE);

					//wait resume, ugly code..
					while (udec_param->paused) {
						u_printf("thread %d paused at 1...\n", udec_index);
						msg_queue_get(cmd_queue, &msg);
						ret = process_cmd(cmd_queue, &msg);
						if (ACTION_QUIT == ret) {
							u_printf("recieve quit cmd, return.\n");
							if (p_dump_file) {
								fclose(p_dump_file);
							}
							free(p_es);
							return (-5);
						} else if (ACTION_RESUME == ret) {
							u_printf("thread %d resumed\n", udec_index);
							udec_param->paused = 0;
							//udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
							break;
						} else if (ACTION_PAUSE == ret) {
							//udec_param->paused = 1;
						} else {
							u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
						}
					}

				} else {
					u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
				}

			}

			//u_printf("**send start size %d.\n", size);
			//send_frames = 0;
			//sendsize = next_es_packet_size(p_cur, p_es_end, &send_frames, udec_type);

			sendsize = get_next_start_code(p_cur, p_es_end, udec_type);
			u_printf_binary_index(udec_index, " left size %d, send size %d.\n", size, sendsize);
			if (!sendsize) {
				break;
			}

			//udec_get_last_display_pts(iav_fd, udec_param, &status);
			ret = request_bits_fifo(iav_fd, udec_index, sendsize + HEADER_SIZE, p_bsb_cur);

			p_frame_start = p_bsb_cur;

			//feed USEQ/UPES header
			if (add_useq_upes_header) {
				if (!udec_param->seq_header_sent) {
					p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->useq_buffer, udec_param->useq_header_len, p_bsb_start, p_bsb_end);
					udec_param->seq_header_sent = 1;
				}
				pes_header_len = fill_upes_header(udec_param->upes_buffer, udec_param->cur_feeding_pts & 0xffffffff, udec_param->cur_feeding_pts >> 32, sendsize, 1, 0);
				p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->upes_buffer, pes_header_len, p_bsb_start, p_bsb_end);

				udec_param->cur_feeding_pts += udec_param->frame_duration;
			}

			p_bsb_cur = copy_to_bsb(p_bsb_cur, p_cur, sendsize, p_bsb_start, p_bsb_end);

			if (test_dump_separate) {
				dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			if (test_dump_total && p_dump_file) {
				_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			memset(&dec, 0, sizeof(dec));
			dec.udec_type = udec_type;
			dec.decoder_id = udec_index;
			dec.u.fifo.start_addr = p_frame_start;
			dec.u.fifo.end_addr = p_bsb_cur;
			dec.num_pics = 1;

			//u_printf("decoding size %d.\n", sendsize);
			//u_printf("p_frame_start %p, p_bsb_cur %p, diff %p.\n", p_frame_start, p_bsb_cur, (unsigned char*)(p_bsb_cur + mSpace - p_frame_start));
			u_printf_binary_index(udec_index, "a) %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_frame_start[0], p_frame_start[1], p_frame_start[2], p_frame_start[3], p_frame_start[4], p_frame_start[5], p_frame_start[6], p_frame_start[7]);
			u_printf_binary_index(udec_index, "(a %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_cur[sendsize], p_cur[sendsize + 1], p_cur[sendsize + 2], p_cur[sendsize + 3], p_cur[sendsize + 4]);
			if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
				u_printf_index(udec_index, "[0error]: ----IAV_IOC_UDEC_DECODE----");
				free(p_es);
				if (p_dump_file) {
					fclose(p_dump_file);
				}
				return (-2);
			}

			p_cur += sendsize;
			size -= sendsize;

			//u_printf("**send end size %d.\n", size);
		}

		if ((test_decode_one_trunk) || (!size && !bytes_left_in_file)) {
			if (test_decode_one_trunk) {
				u_printf_index(udec_index, " one shot done.\n");
			}

			//simple loop, not feed eos
			if (mdec_loop) {
				//u_printf("[flow %d]: loop, return to beginning.\n", udec_index);
				goto repeat_feeding;
			}
			//fill done
			u_printf_index(udec_index, " fill es done, 1.\n");
			//udec_get_last_display_pts(iav_fd, udec_param, &status);
			ret = request_bits_fifo(iav_fd, udec_index, 8, p_bsb_cur);

			p_frame_start = p_bsb_cur;
			p_bsb_cur = copy_to_bsb(p_bsb_cur, &_h264_eos[0], 5, p_bsb_start, p_bsb_end);

			if (test_dump_separate) {
				dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			if (test_dump_total && p_dump_file) {
				_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			memset(&dec, 0, sizeof(dec));
			dec.udec_type = udec_type;
			dec.decoder_id = udec_index;
			dec.u.fifo.start_addr = p_frame_start;
			dec.u.fifo.end_addr = p_bsb_cur;
			dec.num_pics = 0;

			//u_printf("decoding size %d.\n", sendsize);
			//u_printf("p_frame_start %p, p_bsb_cur %p, diff %p.\n", p_frame_start, p_bsb_cur, (unsigned char*)(p_bsb_cur + mSpace - p_frame_start));
			u_printf_binary_index(udec_index, "e) %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_frame_start[0], p_frame_start[1], p_frame_start[2], p_frame_start[3], p_frame_start[4], p_frame_start[5], p_frame_start[6], p_frame_start[7]);
			if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
				u_printf_index(udec_index, "[1error]: ----IAV_IOC_UDEC_DECODE----");
				free(p_es);
				if (p_dump_file) {
					fclose(p_dump_file);
				}
				return (-2);
			}

			break;
		}

		while (msg_queue_peek(cmd_queue, &msg)) {
			ret = process_cmd(cmd_queue, &msg);

			if (ACTION_QUIT == ret) {
				u_printf("recieve quit cmd, return.\n");
				if (p_dump_file) {
					fclose(p_dump_file);
				}
				free(p_es);
				return (-5);
			} else if (ACTION_RESUME  == ret) {
				udec_param->paused = 0;
				//udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
			} else if (ACTION_PAUSE  == ret) {
				udec_param->paused = 1;
				//udec_trickplay(udec_param, UDEC_TRICKPLAY_PAUSE);

				//wait resume, ugly code..
				while (udec_param->paused) {
					u_printf("thread %d paused at 2...\n", udec_index);
					msg_queue_get(cmd_queue, &msg);
					ret = process_cmd(cmd_queue, &msg);
					if (ACTION_QUIT == ret) {
						u_printf("recieve quit cmd, return.\n");
						if (p_dump_file) {
							fclose(p_dump_file);
						}
						free(p_es);
						return (-5);
					} else if (ACTION_RESUME == ret) {
						u_printf("thread %d resumed\n", udec_index);
						udec_param->paused = 0;
						//udec_trickplay(udec_param, UDEC_TRICKPLAY_RESUME);
						break;
					} else if (ACTION_PAUSE == ret) {
						//udec_param->paused = 1;
					} else {
						u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
					}
				}

			} else {
				u_printf_error("un-processed msg %d, ret %d.\n", msg.cmd, ret);
			}

		}

		//copy left bytes
		if (size) {
			memcpy(p_es, p_cur, size);
		}
		p_cur = p_es + size;

		if (bytes_left_in_file) {
			if ((mem_size - size) >= bytes_left_in_file) {
				fread(p_cur, 1, bytes_left_in_file, pFile);
				size += bytes_left_in_file;
				bytes_left_in_file = 0;
				p_cur = p_es;

				if (size <= (DATA_PARSE_MIN_SIZE)) {
					//simple loop, discard some last frames, not feed eos
					if (mdec_loop) {
						//u_printf("[flow %d]: loop, return to beginning.\n", udec_index);
						goto repeat_feeding;
					}
					//last
					//udec_get_last_display_pts(iav_fd, udec_param, &status);
					ret = request_bits_fifo(iav_fd, udec_index, size + HEADER_SIZE, p_bsb_cur);
					p_frame_start = p_bsb_cur;

					//feed USEQ/UPES header
					if (add_useq_upes_header) {
						if (!udec_param->seq_header_sent) {
							p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->useq_buffer, udec_param->useq_header_len, p_bsb_start, p_bsb_end);
							udec_param->seq_header_sent = 1;
						}
						pes_header_len = fill_upes_header(udec_param->upes_buffer, udec_param->cur_feeding_pts & 0xffffffff, udec_param->cur_feeding_pts >> 32, size, 1, 0);
						p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->upes_buffer, pes_header_len, p_bsb_start, p_bsb_end);

						udec_param->cur_feeding_pts += udec_param->frame_duration;
					}

					p_bsb_cur = copy_to_bsb(p_bsb_cur, p_es, size, p_bsb_start, p_bsb_end);

					//add eos
					p_bsb_cur = copy_to_bsb(p_bsb_cur, &_h264_eos[0], 5, p_bsb_start, p_bsb_end);

					if (test_dump_separate) {
						dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
					}

					if (test_dump_total && p_dump_file) {
						_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
					}

					memset(&dec, 0, sizeof(dec));
					dec.udec_type = udec_type;
					dec.decoder_id = udec_index;
					dec.u.fifo.start_addr = p_frame_start;
					dec.u.fifo.end_addr = p_bsb_cur;
					dec.num_pics = total_frame_count(p_es, p_es + size, udec_type);

					u_printf_binary_index(udec_index, " fill es done, 2, last size %d, frame count %d.\n", size, dec.num_pics);
					u_printf_binary_index(udec_index, "m) %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_frame_start[0], p_frame_start[1], p_frame_start[2], p_frame_start[3], p_frame_start[4], p_frame_start[5], p_frame_start[6], p_frame_start[7]);
					//u_printf("p_frame_start %p, p_bsb_cur %p, diff %p.\n", p_frame_start, p_bsb_cur, (unsigned char*)(p_bsb_cur + mSpace - p_frame_start));
					if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
						u_printf_index(udec_index, "[3error]: ----IAV_IOC_UDEC_DECODE----");
						free(p_es);
						if (p_dump_file) {
							fclose(p_dump_file);
						}
						return (-2);
					}

					break;//done
				}
			} else {
				fread(p_cur, 1, mem_size - size, pFile);
				bytes_left_in_file -= (mem_size - size);
				size = mem_size;
				p_cur = p_es;
			}
		} else {

			//simple loop, discard some last frames, not feed eos
			if (mdec_loop) {
				//u_printf("[flow %d]: loop, return to beginning.\n", udec_index);
				goto repeat_feeding;
			}

			//last
			//udec_get_last_display_pts(iav_fd, udec_param, &status);
			ret = request_bits_fifo(iav_fd, udec_index, size + HEADER_SIZE, p_bsb_cur);
			p_frame_start = p_bsb_cur;

			//feed USEQ/UPES header
			if (add_useq_upes_header) {
				if (!udec_param->seq_header_sent) {
					p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->useq_buffer, udec_param->useq_header_len, p_bsb_start, p_bsb_end);
					udec_param->seq_header_sent = 1;
				}
				pes_header_len = fill_upes_header(udec_param->upes_buffer, udec_param->cur_feeding_pts & 0xffffffff, udec_param->cur_feeding_pts >> 32, size, 1, 0);
				p_bsb_cur = copy_to_bsb(p_bsb_cur, udec_param->upes_buffer, pes_header_len, p_bsb_start, p_bsb_end);

				udec_param->cur_feeding_pts += udec_param->frame_duration;
			}

			p_bsb_cur = copy_to_bsb(p_bsb_cur, p_es, size, p_bsb_start, p_bsb_end);

			//add eos
			p_bsb_cur = copy_to_bsb(p_bsb_cur, &_h264_eos[0], 5, p_bsb_start, p_bsb_end);

			if (test_dump_separate) {
				dump_binary_new_file(test_dump_separate_filename, udec_index, dump_separate_file_index++, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			if (test_dump_total && p_dump_file) {
				_write_data_ring(p_dump_file, p_bsb_start, p_bsb_end, p_frame_start, p_bsb_cur);
			}

			memset(&dec, 0, sizeof(dec));
			dec.udec_type = udec_type;
			dec.decoder_id = udec_index;
			dec.u.fifo.start_addr = p_frame_start;
			dec.u.fifo.end_addr = p_bsb_cur;
			dec.num_pics = total_frame_count(p_es, p_es + size, udec_type);

			u_printf_binary_index(udec_index, " fill es done, 3, last size %d, frame count %d, [index %d].\n", size, dec.num_pics);
			u_printf_binary_index(udec_index, "c) %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n", p_frame_start[0], p_frame_start[1], p_frame_start[2], p_frame_start[3], p_frame_start[4], p_frame_start[5], p_frame_start[6], p_frame_start[7]);
			//u_printf("p_frame_start %p, p_bsb_cur %p, diff %p.\n", p_frame_start, p_bsb_cur, (unsigned char*)(p_bsb_cur + mSpace - p_frame_start));
			if ((ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
				u_printf_index(udec_index, "[3error]: ----IAV_IOC_UDEC_DECODE----");
				free(p_es);
				if (p_dump_file) {
					fclose(p_dump_file);
				}
				return (-2);
			}

			break;//done
		}
	}

	u_printf_index(udec_index, "[flow]: send es data done.\n");
	free(p_es);
	if (p_dump_file) {
		fclose(p_dump_file);
	}
	return 0;
}

static void* udec_instance(void* param)
{
	int ret;
	//int cur_index = 0;
	iav_udec_info_ex_t info;
	//int num_vout = 2;
	msg_t msg;
	int exit_flag = 0;

	udec_instance_param_t* udec_param = (udec_instance_param_t*)param;
	if (!param) {
		u_printf("NULL params in udec_instance.\n");
		return NULL;
	}

	//init udec instance
	memset(&info, 0x0, sizeof(info));
	ret = ioctl_init_udec_instance(&info, udec_param->iav_fd, udec_param->p_vout_config, udec_param->num_vout, udec_param->udec_index, udec_param->request_bsb_size, udec_param->udec_type, 0, udec_param->pic_width, udec_param->pic_height);
	if (ret < 0) {
		u_printf("[error]: ioctl_init_udec_instance fail.\n");
		return NULL;
	}

	u_printf("[flow (%d)]: type(%d) begin loop.\n", udec_param->udec_index, udec_param->udec_type);
	u_printf("	[params %d]: iav_fd(%d), udec_type(%d), bsb_start(%p), loop(%d), file fd0(%p), fd1(%p).\n", udec_param->udec_index, udec_param->iav_fd, udec_param->udec_type, info.bits_fifo_start, udec_param->loop, udec_param->file_fd[0], udec_param->file_fd[1]);
	u_printf("	[params %d]: wait_cmd_begin(%d), wait_cmd_exit(%d).\n", udec_param->udec_index, udec_param->wait_cmd_begin, udec_param->wait_cmd_exit);

udec_instance_loop_begin:
	//wait begin if needed
	while (udec_param->wait_cmd_begin) {
		msg_queue_get(&udec_param->cmd_queue, &msg);
		ret = process_cmd(&udec_param->cmd_queue, &msg);
		if (ACTION_QUIT == ret) {
			exit_flag = 1;
			goto udec_instance_loop_end;
		} else if (ACTION_START == ret) {
			break;
		} else if (ACTION_RESUME == ret) {
			break;
		} else if (ACTION_PAUSE == ret) {
			//
		} else {
			u_printf_error("un-processed cmd %d, ret %d.\n", msg.cmd, ret);
		}
	}

	u_printf("[flow (%d)]: udec instance index, before udec_instance_decode_es_file.\n", udec_param->udec_index);
	ret = udec_instance_decode_es_file(udec_param->udec_index, udec_param->iav_fd, udec_param->file_fd[0], udec_param->udec_type, info.bits_fifo_start, info.bits_fifo_start + udec_param->request_bsb_size, info.bits_fifo_start, &udec_param->cmd_queue, udec_param);
	if (ret < 0) {
		//u_printf("udec_instance(%d)_decode_es_file ret %d, exit thread.\n", udec_param->udec_index, ret);
		exit_flag = 1;
	}
	u_printf("[flow (%d)]: udec instance index, after udec_instance_decode_es_file, ret %d.\n", udec_param->udec_index, ret);

udec_instance_loop_end:
	if (!exit_flag) {
		while (udec_param->wait_cmd_exit) {
			msg_queue_get(&udec_param->cmd_queue, &msg);
			ret = process_cmd(&udec_param->cmd_queue, &msg);
			if (ACTION_QUIT == ret) {
				break;
			} else if ((ACTION_RESTART == ret) || (ACTION_RESUME == ret)) {
				u_printf("[flow]: resume cmd comes\n");
				udec_param->wait_cmd_begin = 0;
				goto udec_instance_loop_begin;
			} else if (ACTION_RESUME == ret) {
				break;
			} else if (ACTION_PAUSE == ret) {
				//
			} else {
				u_printf_error("un-processed cmd %d, ret %d.\n", msg.cmd, ret);
			}
		}
	}

	//stop udec
	u_printf("[flow]: before IAV_IOC_UDEC_STOP(%d).\n", udec_param->udec_index);
	if (ioctl(udec_param->iav_fd, IAV_IOC_UDEC_STOP, udec_param->udec_index) < 0) {
		perror("IAV_IOC_UDEC_STOP");
		u_printf_error("stop udec instance(%d) fail.\n");
	}

	//release udec
	u_printf("[flow]: before IAV_IOC_RELEASE_UDEC(%d).\n", udec_param->udec_index);
	if (ioctl(udec_param->iav_fd, IAV_IOC_RELEASE_UDEC, udec_param->udec_index) < 0) {
		perror("IAV_IOC_RELEASE_UDEC");
		u_printf_error("release udec instance(%d) fail.\n");
	}

	u_printf("[flow]: udec instance(%d) exit loop.\n", udec_param->udec_index);

	return NULL;
}

static void sig_stop(int a)
{
	mdec_running = 0;
}

static void _get_stream_number(int* sds, int* hds, int total_cnt)
{
	int i = 0;
	int sd = 0, hd = 0;
	*sds = 0;
	*hds = 0;
	for (i = 0; i < total_cnt; i++) {
		if (is_hd[i]) {
			hd ++;
		} else {
			sd ++;
		}
	}

	u_printf("found hd stream number %d, sd stream number %d.\n", hd, sd);
	*sds = sd;
	*hds = hd;
}

static void _print_udec_info(int iav_fd, u8 udec_index)
{
	int ret;
	iav_udec_state_t state;

	memset(&state, 0, sizeof(state));
	state.decoder_id = udec_index;
	state.flags = IAV_UDEC_STATE_DSP_READ_POINTER | IAV_UDEC_STATE_BTIS_FIFO_ROOM | IAV_UDEC_STATE_ARM_WRITE_POINTER;

	ret = ioctl(iav_fd, IAV_IOC_GET_UDEC_STATE, &state);
	if (ret) {
		perror("IAV_IOC_GET_UDEC_STATE");
		u_printf("IAV_IOC_GET_UDEC_STATE %d fail.\n", udec_index);
		return;
	}

	u_printf("print udec(%d) info:\n", udec_index);
	u_printf("     udec_state(%d), vout_state(%d), error_code(0x%08x):\n", state.udec_state, state.vout_state, state.error_code);
	u_printf("     bsb info: phys start addr 0x%08x, total size %u(0x%08x), free space %u:\n", state.bits_fifo_phys_start, state.bits_fifo_total_size, state.bits_fifo_total_size, state.bits_fifo_free_size);

	u_printf("     dsp read pointer from msg: (phys) 0x%08x, diff from start 0x%08x, map to usr space 0x%08x.\n", state.dsp_current_read_bitsfifo_addr_phys, state.dsp_current_read_bitsfifo_addr_phys - state.bits_fifo_phys_start, state.dsp_current_read_bitsfifo_addr);
	u_printf("     last arm write pointer: (phys) 0x%08x, diff from start 0x%08x, map to usr space 0x%08x.\n", state.arm_last_write_bitsfifo_addr_phys, state.arm_last_write_bitsfifo_addr_phys - state.bits_fifo_phys_start, state.arm_last_write_bitsfifo_addr);

	u_printf("     tot send decode cmd %d, tot frame count %d.\n", state.tot_decode_cmd_cnt, state.tot_decode_frame_cnt);

	return;
}

static int ioctl_stream_switch_cmd(int iav_fd, int render_id, int new_udec_index)
{
	int ret;
	iav_postp_stream_switch_t stream_switch;
	memset(&stream_switch, 0x0, sizeof(stream_switch));

	stream_switch.num_config = 1;
	stream_switch.switch_config[0].render_id = render_id;
	stream_switch.switch_config[0].new_udec_id = new_udec_index;

	u_printf("[cmd flow]: before IAV_IOC_POSTP_STREAM_SWITCH, render_id %d, new_udec_index %d.\n", render_id, new_udec_index);
	if ((ret = ioctl(iav_fd, IAV_IOC_POSTP_STREAM_SWITCH, &stream_switch)) < 0) {
		perror("IAV_IOC_POSTP_STREAM_SWITCH");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_POSTP_STREAM_SWITCH.\n");

	return 0;
}

static int ioctl_wait_stream_switch_cmd(int iav_fd, int render_id)
{
	int ret;
	iav_wait_stream_switch_msg_t wait_stream_switch;
	memset(&wait_stream_switch, 0x0, sizeof(wait_stream_switch));

	wait_stream_switch.render_id = render_id;
	wait_stream_switch.wait_flags = 0;//blocked

	u_printf("[cmd flow]: before IAV_IOC_WAIT_STREAM_SWITCH_MSG, render_id %d.\n", render_id);
	if ((ret = ioctl(iav_fd, IAV_IOC_WAIT_STREAM_SWITCH_MSG, &wait_stream_switch)) < 0) {
		perror("IAV_IOC_WAIT_STREAM_SWITCH_MSG");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_WAIT_STREAM_SWITCH_MSG, status %d.\n", wait_stream_switch.switch_status);

	return 0;
}

static int ioctl_render_cmd(int iav_fd, int number_of_render, int number_of_window, udec_render_t* p_render)
{
	int ret;
	iav_postp_render_config_t render;

	if (number_of_render <= 0 || number_of_window<= 0) {
		u_printf_error("internal error, inavlid number_of_render %d, number_of_window %d.\n", number_of_render, number_of_window);
		return -2;
	}

	memset(&render, 0x0, sizeof(render));

	render.total_num_windows_to_render = number_of_window;
	render.num_configs = number_of_render;
	u_assert(number_of_render < 6);
	if (number_of_render < 6) {
		for (ret = 0; ret < number_of_render; ret ++) {
			render.configs[ret] = p_render[ret];
			u_printf("    [render %d]: win %d, win_2rd %d, udec_id %d.\n", render.configs[ret].render_id, render.configs[ret].win_config_id, render.configs[ret].win_config_id_2nd, render.configs[ret].udec_id);
		}
	}

	u_printf("[cmd flow]: before IAV_IOC_POSTP_RENDER_CONFIG.\n");
	if ((ret = ioctl(iav_fd, IAV_IOC_POSTP_RENDER_CONFIG, &render)) < 0) {
		perror("IAV_IOC_POSTP_RENDER_CONFIG");
		return -1;
	}
	u_printf("[cmd flow]: after IAV_IOC_POSTP_RENDER_CONFIG.\n");

	return 0;
}

static void resume_pause_feeding(udec_instance_param_t* params, int thread_start_index, int thread_end_index, int total_feeding_thread_number, int pause)
{
	int i = 0;
	msg_t msg;

	if ((thread_start_index >= total_feeding_thread_number) || (thread_start_index < 0)) {
		u_assert(0);
		return;
	}
	if ((thread_end_index >= total_feeding_thread_number) || (thread_end_index < 0)) {
		u_assert(0);
		return;
	}

	if (pause) {
		msg.cmd = M_MSG_PAUSE;
	} else {
		msg.cmd = M_MSG_RESUME;
	}
	msg.ptr = NULL;

	for (i = thread_start_index; i <= thread_end_index; i++) {
		u_printf("[cmd]: pause/resume(%d) udec(%d), msg.cmd %d.\n", pause, i, msg.cmd);
		msg_queue_put(&params[i].cmd_queue, &msg);
	}
}

//simulate real time streaming's case, adjust pts
#if 0
static void adjust_start_pts(udec_instance_param_t* params, int thread_from_index, int thread_to_index, int total_feeding_thread_number)
{
	unsigned long long to_pts;
	if ((thread_from_index >= total_feeding_thread_number) || (thread_to_index < 0)) {
		u_assert(0);
		return;
	}
	if ((thread_to_index >= total_feeding_thread_number) || (thread_to_index < 0)) {
		u_assert(0);
		return;
	}

	if (!params[thread_to_index].last_pts_from_dsp_valid) {
		u_printf("[warnning]: udec(%d)'s pts from dsp is not avaiable, use feeding pts instead.\n", thread_to_index);
		to_pts = params[thread_to_index].cur_feeding_pts;
	} else {
		to_pts = params[thread_to_index].last_display_pts;
	}
	u_printf("try to adjust pts from %llu to %llu, align %d to %d.\n", params[thread_from_index].cur_feeding_pts, to_pts, thread_from_index, thread_to_index);
	params[thread_from_index].cur_feeding_pts = to_pts;
}
#endif

static void adjust_start_pts_ex(udec_instance_param_t* params, int thread_from_index, int thread_to_index, int total_feeding_thread_number)
{
	int ret;
	unsigned long long to_pts;
	if ((thread_from_index >= total_feeding_thread_number) || (thread_to_index < 0)) {
		u_assert(0);
		return;
	}
	if ((thread_to_index >= total_feeding_thread_number) || (thread_to_index < 0)) {
		u_assert(0);
		return;
	}

	ret = query_udec_last_display_pts(params->iav_fd, thread_to_index, &to_pts);
	if (0 != ret) {
		u_printf("[warnning]: there's no pts from dsp, this stream does not start feeding? use feeding pts instead.\n");
		to_pts = params[thread_to_index].cur_feeding_pts;
	}
	u_printf("try to adjust pts from %llu to %llu, align %d to %d.\n", params[thread_from_index].cur_feeding_pts, to_pts, thread_from_index, thread_to_index);
	params[thread_from_index].cur_feeding_pts = to_pts;
}

static int do_test_mdec(int iav_fd)
{
	unsigned int i;
	pthread_t thread_id[MAX_NUM_UDEC];
	udec_instance_param_t params[MAX_NUM_UDEC];
	iav_udec_vout_config_t vout_cfg[NUM_VOUT];
	int vout_start_index = -1;
	int num_of_vout = 0;
	msg_t msg;
	//unsigned int tot_number_udec;
	int sds, hds;
	int total_num;

	char buffer_old[128] = {0};
	char buffer[128];
	char* p_buffer = buffer;
	int flag_stdin = 0;

	int cur_display = 0;// 0: 4xwindow, 1: 1xwindow

	signal(SIGINT, sig_stop);
	signal(SIGQUIT, sig_stop);
	signal(SIGTERM, sig_stop);

	dec_types = 0x17;	// no RV40, no hybrid MPEG4

	ppmode = 3;

	//if (tot_number_udec == 0 || tot_number_udec > MAX_NUM_UDEC) {
	//	u_printf_error(" bad number %d\n", tot_number_udec);
	//	return -1;
	//}

	//enter mdec mode config
	iav_mdec_mode_config_t mdec_mode;
	iav_udec_mode_config_t *udec_mode = &mdec_mode.super;
	iav_udec_config_t *udec_configs;
	udec_window_t *windows;
	udec_render_t *renders, *tmp_renders;

	total_num = current_file_index + 1;
	_get_stream_number(&sds, &hds, total_num);

	if ((1 == total_num) || (0 == sds)) {
		if (!first_show_full_screen) {
			first_show_full_screen = 1;
			u_printf("[change settings]: first use full screen show one stream playback.\n");
		}
	}

	if (0 == sds) {
		if (!first_show_hd_stream) {
			first_show_hd_stream = 1;
			u_printf("[change settings]: first show hd stream.\n");
		}
	}

	u_printf("[flow]: before malloc some memory, current_file_index %d, total_num, sds %d, hds %d, first_show_full_screen %d, first_show_hd_stream %d\n", current_file_index, total_num, sds, hds, first_show_full_screen, first_show_hd_stream);

	if (sds > 4 && 0 == hds) {
		u_printf("[debug]: make 5'th stream as hd, for debug.\n");
		hds = 1;
		sds = 4;
	}

	if ((udec_configs = malloc(total_num * sizeof(iav_udec_config_t))) == NULL) {
		u_printf_error(" no memory\n");
		return -1;
	}

	if ((windows = malloc(total_num * sizeof(udec_window_t))) == NULL) {
		u_printf_error(" no memory\n");
		return -1;
	}

	if ((renders = malloc(total_num * sizeof(udec_render_t))) == NULL) {
		u_printf_error(" no memory\n");
		return -1;
	}

	u_printf("[flow]: before get vout info\n");
	//get all vout info
	for (i = 0; i < NUM_VOUT; i++) {
		if (get_single_vout_info(i, vout_width + i, vout_height + i, iav_fd) < 0) {
			u_printf_error(" get vout(%d) info fail\n", i);
			continue;
		}
		if (max_vout_width < vout_width[i])
			max_vout_width = vout_width[i];
		if (max_vout_height < vout_height[i])
			max_vout_height = vout_height[i];
		if (vout_start_index < 0) {
			vout_start_index = i;
		}
		num_of_vout ++;
	}

	for (i = vout_start_index; i < (num_of_vout + vout_start_index); i++) {
		vout_cfg[i].disable = 0;
		vout_cfg[i].udec_id = 0;//hard code, fix me
		vout_cfg[i].flip = 0;
		vout_cfg[i].rotate = 0;
		vout_cfg[i].vout_id = i;
		vout_cfg[i].win_offset_x = vout_cfg[i].target_win_offset_x = 0;
		vout_cfg[i].win_offset_y = vout_cfg[i].target_win_offset_y = 0;
		vout_cfg[i].win_width = vout_cfg[i].target_win_width = vout_width[i];
		vout_cfg[i].win_height = vout_cfg[i].target_win_height = vout_height[i];
		vout_cfg[i].zoom_factor_x = vout_cfg[i].zoom_factor_y = 1;
	}

	u_printf("[flow]: before init_udec_mode_config\n");
	init_udec_mode_config(udec_mode);
	udec_mode->num_udecs = total_num;
	udec_mode->udec_config = udec_configs;

	mdec_mode.total_num_win_configs = total_num;
	mdec_mode.windows_config = windows;
	//mdec_mode.render_config = renders;

	mdec_mode.av_sync_enabled = 0;
	mdec_mode.out_to_hdmi = 1;
	mdec_mode.audio_on_win_id = 0;

	if (sds) {
		u_printf("[flow]: before init_udec_configs, sd\n");
		init_udec_configs(udec_configs, sds, file_video_width[0], file_video_height[0]);
	}

	if (hds) {
		u_printf("[flow]: before init_udec_configs, hd\n");
		init_udec_configs(udec_configs + sds, hds, file_video_width[sds], file_video_height[sds]);
	}

	if (sds) {
		u_printf("[flow]: before init_udec_windows, sd\n");
		init_udec_windows(windows, 0, sds, file_video_width[0], file_video_height[0]);
	}

	if (hds) {
		u_printf("[flow]: before init_udec_windows, hd\n");
		init_udec_windows(windows, sds, hds, file_video_width[sds + 0], file_video_height[sds + 0]);
	}

	if (sds) {
		u_printf("[flow]: before init_udec_renders\n");
		init_udec_renders(renders, sds);
	}

	if (hds) {
		//for hd
		tmp_renders = renders + sds;
		for (i = 0; i < hds; i++, tmp_renders ++) {
			u_printf("[flow]: before init_udec_renders_single for hd\n");
			init_udec_renders_single(tmp_renders, i + sds, 0xff, 0xff, i + sds);
		}
	}

	//config renders, which will display
	if (!first_show_hd_stream) {
		mdec_mode.total_num_render_configs = sds;
		mdec_mode.render_config = renders;

		if (sds > 1) {
			cur_display = 0;
		} else if (1 ==  sds){
			u_printf("[flow]: show sd stream, but full screen.\n");
			cur_display = 1;
		} else {
			u_printf_error("[error]: invalid settings, sds(%d) <= 0?\n", sds);
			return (-2);
		}
	} else {
		//first hd stream, hard code here

		tmp_renders = renders + sds;
		init_udec_renders_single(tmp_renders, 0, 0 + sds, 0xff, 0 + sds);

		mdec_mode.total_num_render_configs = 1;
		mdec_mode.render_config = tmp_renders;

		cur_display = 1;
	}

	mdec_mode.total_num_win_configs = total_num;
	mdec_mode.max_num_windows = total_num + 1;

	u_printf("[flow]: before IAV_IOC_ENTER_MDEC_MODE, num render configs %d, num win configs %d, max num windows %d\n", mdec_mode.total_num_render_configs, mdec_mode.total_num_win_configs, mdec_mode.max_num_windows);
	if (ioctl(iav_fd, IAV_IOC_ENTER_MDEC_MODE, &mdec_mode) < 0) {
		perror("IAV_IOC_ENTER_MDEC_MODE");
		u_printf_error(" enter mdec mode fail\n");
		return -1;
	}

	u_printf("[flow]: enter mdec mode done\n");

	i = 0;
	//each instance's parameters, sd
	for (i = 0; i < sds; i++) {
		params[i].udec_index = i;
		params[i].iav_fd = iav_fd;
		params[i].loop = 1;
		params[i].request_bsb_size = bits_fifo_size;
		params[i].udec_type = file_codec[i];
		params[i].pic_width = file_video_width[i];
		params[i].pic_height= file_video_height[i];
		msg_queue_init(&params[i].cmd_queue);

		if (!test_feed_background) {
			if (!first_show_hd_stream) {
				params[i].wait_cmd_begin = 0;
			} else {
				params[i].wait_cmd_begin = 1;
			}
		} else {
			params[i].wait_cmd_begin = 0;
		}

		//debug use
		if (params[i].wait_cmd_begin) {
			feeding_sds = 0;
		} else {
			feeding_sds = 1;
		}

		params[i].wait_cmd_exit = 1;

		params[i].num_vout = num_of_vout;
		params[i].p_vout_config = &vout_cfg[vout_start_index];
		params[i].file_fd[0] = fopen(file_list[i], "rb");
		params[i].file_fd[1] = NULL;
	}

	//each instance's parameters, sd
	for (; i < sds + hds; i++) {
		params[i].udec_index = i;
		params[i].iav_fd = iav_fd;
		params[i].loop = 1;
		params[i].request_bsb_size = bits_fifo_size;
		params[i].udec_type = file_codec[i];
		params[i].pic_width = file_video_width[i];
		params[i].pic_height= file_video_height[i];
		msg_queue_init(&params[i].cmd_queue);

		if (!test_feed_background) {
			if (first_show_hd_stream) {
				params[i].wait_cmd_begin = 0;
			} else {
				params[i].wait_cmd_begin = 1;
			}
		} else {
			params[i].wait_cmd_begin = 0;
		}

		//debug use
		if (params[i].wait_cmd_begin) {
			feeding_hds = 0;
		} else {
			feeding_hds = 1;
		}

		params[i].wait_cmd_exit = 1;

		params[i].num_vout = num_of_vout;
		params[i].p_vout_config = &vout_cfg[vout_start_index];
		params[i].file_fd[0] = fopen(file_list[i], "rb");
		params[i].file_fd[1] = NULL;
	}

	//each instance's USEQ, UPES header
	for (i = 0; i < sds + hds; i++) {
		params[i].cur_feeding_pts = 0;

		//hard code to 29.97 fps
		params[i].frame_tick = 3003;
		params[i].time_scale = 90000;

		params[i].frame_duration = ((u64)90000)*((u64)params[i].frame_tick)/params[i].time_scale;
		u_assert(3003 == params[i].frame_duration);//debug assertion

		//hard code to h264
		//init USEQ/UPES header
		params[i].useq_header_len = fill_useq_header(params[i].useq_buffer, UDEC_H264, params[i].time_scale, params[i].frame_tick, 0, 0, 0);
		u_assert(UDEC_SEQ_HEADER_LENGTH == params[i].useq_header_len);

		init_upes_header(params[i].upes_buffer, UDEC_H264);

		params[i].seq_header_sent = 0;
		params[i].last_pts_from_dsp_valid = 0;
		params[i].paused = 0;
		params[i].trickplay_mode = UDEC_TRICKPLAY_RESUME;
	}


	//spawn all threads
	for (i = 0; i < total_num; i++) {
		pthread_create(&thread_id[i], NULL, udec_instance, &params[i]);
	}

	flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
	if(fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1) {
		u_printf("[error]: stdin_fileno set error.\n");
	}

	memset(buffer, 0x0, sizeof(buffer));
	memset(buffer_old, 0x0, sizeof(buffer_old));

	int switch_render_id;
	int switch_new_udec_id;
	int switch_auto_update_display;

	int input_total_renders;
//	int input_render_id;
	int input_win_id;
	int input_win2_id;
	int input_udec_id;
	char* p_input_tmp;

	unsigned int input_flag;

	//main cmd loop
	while (mdec_running) {
		//add sleep to avoid affecting the performance
		usleep(100000);
		if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
			continue;

		if (buffer[0] == '\n') {
			p_buffer = buffer_old;
			u_printf("repeat last cmd:\n\t\t%s\n", buffer_old);
		} else if (buffer[0] == 'l') {
			u_printf("show last cmd:\n\t\t%s\n", buffer_old);
			continue;
		} else {
			p_buffer = buffer;

			//record last cmd
			strncpy(buffer_old, buffer, sizeof(buffer_old) -1);
			buffer_old[sizeof(buffer_old) -1] = 0x0;
		}

		switch (p_buffer[0]) {
			case 'q':	// exit
				u_printf("Quit\n");
				mdec_running = 0;

				//resume all udecs in this case
				for (i = 0; i < sds + hds; i ++) {
					udec_trickplay(params + i, UDEC_TRICKPLAY_RESUME);
				}
				break;

			case 's':	//step
				if (1 != sscanf(p_buffer, "s%d", &switch_new_udec_id)) {
					u_printf("BAD cmd format, you should type 's\%d' and enter, to do step mode for the udec(\%d).\n");
					break;
				}

				u_assert(switch_new_udec_id >= 0);
				u_assert(switch_new_udec_id < (hds + sds));
				if ((switch_new_udec_id < 0) || (switch_new_udec_id >= (hds + sds))) {
					u_printf_error("BAD input params, udec_id %d, out of range.\n", switch_new_udec_id);
					break;
				}

				udec_step(params + switch_new_udec_id);
				break;

			case ' ':
				if (1 != sscanf(p_buffer, " %d", &switch_new_udec_id)) {
					u_printf("BAD cmd format, you should type ' \%d' and enter, to do pause/resume the udec(\%d).\n");
					break;
				}

				u_assert(switch_new_udec_id >= 0);
				u_assert(switch_new_udec_id < (hds + sds));
				if ((switch_new_udec_id < 0) || (switch_new_udec_id >= (hds + sds))) {
					u_printf_error("BAD input params, udec_id %d, out of range.\n", switch_new_udec_id);
					break;
				}

				udec_pause_resume(params + switch_new_udec_id);
				break;

			case 'z':
				if ('0' == p_buffer[1]) {
					//stop(0)
					input_flag = 0x0;
				} else if ('1' == p_buffer[1]) {
					//stop(1)
					input_flag = 0x1;
				} else if ('f' == p_buffer[1]) {
					//stop(0xff)
					input_flag = 0xff;
				} else {
					u_printf("not support cmd %s.\n", p_buffer);
					u_printf("   type 'z0\%d' to send stop(0) to udec(\%d).\n");
					u_printf("   type 'z1\%d' to send stop(1) to udec(\%d).\n");
					u_printf("   type 'zf\%d' to send stop(0xff) to udec(\%d).\n");
					break;
				}

				if (1 != sscanf(p_buffer + 2, "%d", &switch_new_udec_id)) {
					u_printf("BAD cmd format, you should type 'z%c\%d' and enter, to send stop(%x) for udec(\%d).\n", p_buffer[1], input_flag);
					break;
				}
				ioctl_udec_stop(iav_fd, switch_new_udec_id, input_flag);
				break;

			case 'p':
				if ('a' == p_buffer[1]) {
					//print all udec instance
					for (i = 0; i < total_num; i ++) {
						_print_udec_info(iav_fd, (u8)i);
					}
				} else if (1 == sscanf(p_buffer, "p%d", &i)) {
					//print specified udec instance
					if (i >=0 && i < total_num) {
						_print_udec_info(iav_fd, (u8)i);
					} else {
						u_printf("BAD udec index %d.\n", i);
					}
				} else {
					u_printf("not support cmd %s.\n", p_buffer);
					u_printf("   type 'pa' to print all udec instances.\n");
					u_printf("   type 'p\%d' to print specified udec instance.\n");
				}
				break;

			case 'c':
				if ('s' == p_buffer[1]) {

					switch_auto_update_display = 0;
					if ('a' == p_buffer[2]) {
						switch_auto_update_display = 1;
						if (2 != sscanf(p_buffer, "csa%dto%d", &switch_render_id, &switch_new_udec_id)) {
							u_printf("BAD cmd format, you should type 'csa\%dto\%d' and enter, to do stream switch render_id, new_udec_id, (auto update display after switch is finished).\n");
							break;
						}
					} else {
						if (2 != sscanf(p_buffer, "cs%dto%d", &switch_render_id, &switch_new_udec_id)) {
							u_printf("BAD cmd format, you should type 'cs\%dto\%d' and enter, to do stream switch render_id, new_udec_id, (do not auto update display).\n");
							break;
						}
					}

					//stream switch
					u_assert(switch_render_id < 4);
					u_assert(switch_new_udec_id < (hds + sds));
					if ((switch_render_id >= 4) || (switch_new_udec_id >= (hds + sds))) {
						u_printf_error("BAD input params, switch_render_id %d, switch_new_udec_id %d.\n", switch_render_id, switch_new_udec_id);
						break;
					}

					if ((switch_render_id < 0) || (switch_new_udec_id < 0)) {
						u_printf_error("BAD input params, switch_render_id %d, switch_new_udec_id %d.\n", switch_render_id, switch_new_udec_id);
						break;
					}

					if (renders[switch_render_id].udec_id != switch_new_udec_id) {
						if (!test_feed_background) {
							adjust_start_pts_ex(params, switch_new_udec_id, renders[switch_render_id].udec_id, sds + hds);
						}

						if (!test_feed_background) {
							//resume feeding
							//u_printf("[flow]: resume feeding for thread %d.\n", switch_new_udec_id);
							resume_pause_feeding(params, switch_new_udec_id, switch_new_udec_id, sds + hds, 0);

							//resume UDEC if needed
							udec_trickplay(params + switch_new_udec_id, UDEC_TRICKPLAY_RESUME);
						}

						//send switch cmd
						ioctl_stream_switch_cmd(iav_fd, switch_render_id, switch_new_udec_id);

						//p_input_tmp = strchr(p_buffer, ':');
						//if (p_input_tmp && ('w' == p_input_tmp[1])) {
							u_printf("[flow]: request wait switch done.\n");
							ioctl_wait_stream_switch_cmd(iav_fd, switch_render_id);

							if (!test_feed_background) {
								//pause feeding
								//u_printf("[flow]: pause feeding for thread %d.\n", renders[switch_render_id].udec_id);
								resume_pause_feeding(params, renders[switch_render_id].udec_id, renders[switch_render_id].udec_id, sds + hds, 1);
							}

						//}
						renders[switch_render_id].udec_id = switch_new_udec_id;

						//update display if needed
						if (switch_auto_update_display) {
							if ((0 == cur_display) && (switch_new_udec_id >= sds)) {
								u_printf("[flow]: auto update display to 1 x window.\n");
								//switch to one window display
								for (i = sds; i < sds + hds; i ++) {
									renders[i].render_id = i - sds;
									renders[i].win_config_id = i;
									renders[i].win_config_id_2nd = 0xff;//hard code here
									renders[i].udec_id = i;
								}
								//#endif
								ioctl_render_cmd(iav_fd, hds, hds, renders + sds);
								cur_display = 1;

								if (!test_feed_background) {
									//pause sds
									resume_pause_feeding(params, 0, sds - 1, sds + hds, 1);
								}

							} else if ((1 == cur_display) && (switch_new_udec_id < sds)) {
								u_printf("[flow]: auto update display to 4 x window.\n");

								if (!test_feed_background) {
									//resume if needed
									resume_pause_feeding(params, 0, sds - 1, sds + hds, 0);

									for (i = 0; i < sds; i ++) {
										//resume UDEC if needed
										udec_trickplay(params + i, UDEC_TRICKPLAY_RESUME);
									}
								}

								ioctl_render_cmd(iav_fd, sds, sds, renders);
								cur_display = 0;
								if (!test_feed_background) {
									//pause hds
									resume_pause_feeding(params, sds, sds + hds - 1, sds + hds, 1);
								}
							}
						}
					} else {
						u_printf("udec_id not changed, do not send switch cmd\n");
					}

				} else if ('w' == p_buffer[1]) {
					//wait
					if (1 == sscanf(p_buffer, "cw%d", &switch_render_id)) {
						ioctl_wait_stream_switch_cmd(iav_fd, switch_render_id);
					}
				} else if ('r' == p_buffer[1]) {
					//render, total number of  renders first
					if ('s' == p_buffer[2] && 'd' == p_buffer[3]) {
						// restore to display 4x sd's case
						#if 0
						for (i = 0; i < sds; i ++) {
							renders[i].render_id = i;
							renders[i].win_config_id = i;
							renders[i].win_config_id_2nd = 0xff;//hard code here
							renders[i].udec_id = i;
						}
						#endif
						if (!test_feed_background) {
							//resume cmd
							resume_pause_feeding(params, 0, sds - 1, sds + hds, 0);

							for (i = 0; i < sds; i ++) {
								//resume UDEC if needed
								udec_trickplay(params + i, UDEC_TRICKPLAY_RESUME);
							}
						}
						ioctl_render_cmd(iav_fd, sds, sds, renders);
						if (!test_feed_background) {
							//pause hds
							resume_pause_feeding(params, sds, sds + hds - 1, sds + hds, 1);
						}

						cur_display = 0;
					} else if ('h' == p_buffer[2] && 'd' == p_buffer[3]) {
						// restore display 1x hd's case
						//#if 0
						for (i = sds; i < sds + hds; i ++) {
							renders[i].render_id = i - sds;
							renders[i].win_config_id = i;
							renders[i].win_config_id_2nd = 0xff;//hard code here
							renders[i].udec_id = i;
						}
						//#endif

						if (!test_feed_background) {
							//resume cmd
							resume_pause_feeding(params, sds, sds + hds -1, sds + hds, 0);

							//resume UDEC if needed
							udec_trickplay(params + switch_new_udec_id, UDEC_TRICKPLAY_RESUME);
						}

						ioctl_render_cmd(iav_fd, hds, hds, renders + sds);
						if (!test_feed_background) {
							//pause sds
							resume_pause_feeding(params, 0, sds - 1, sds + hds, 1);
						}

						cur_display = 1;
					} else if (1 == sscanf(p_buffer, "cr %d:", &input_total_renders)) {
						u_printf("[ipnut cmd]: update render, total render count %d.\n", input_total_renders);
						p_input_tmp = strchr(p_buffer, ':');
						u_assert(p_input_tmp);
						u_assert(input_total_renders < 6);
						if (input_total_renders >= 6) {
							u_printf("[input cmd]: bad parameters, number of renders should less than 6.\n");
							break;
						}
						i = 0;
						while ((i < input_total_renders) && p_input_tmp) {
							sscanf(p_input_tmp, ":%x %x %x", &input_win_id, &input_win2_id, &input_udec_id);
							u_printf("[ipnut cmd]: render id %d, win id %d, win2 id %d, udec_id %d.\n", i, input_win_id, input_win2_id, input_udec_id);
							p_input_tmp = strchr(p_input_tmp + 1, ':');

							//update render_t
							renders[i].render_id = i;
							renders[i].win_config_id = input_win_id;
							renders[i].win_config_id_2nd = input_win2_id;
							renders[i].udec_id = input_udec_id;
							i ++;
						}
						ioctl_render_cmd(iav_fd, i, i, renders);
					} else {
						u_printf("\t[not supported cmd]: for update render for NVR, please use 'crsd' to show 4xsd, 'crhd' to show 1xhd.'\n");
						u_printf("\t                              or use 'cr render_number:win_id win2_id udec_id,win_id win2_id udec_id,win_id win2_id udec_id,win_id win2_id udec_id...'\n");
					}
				} else {
					//render
				}

				break;

			default:
				break;
		}

	}

	if(fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
		u_printf("[error]: stdin_fileno set error");
	}

	//exit each threads
	msg.cmd = M_MSG_KILL;
	msg.ptr = NULL;
	for (i = 0; i < total_num; i++) {
		msg_queue_put(&params[i].cmd_queue, &msg);
	}

	void* pv;
	int ret = 0;
	for (i = 0; i < total_num; i++) {
		u_printf("[flow]: wait udec_instance_thread(%d) exit...\n", i);
		ret = pthread_join(thread_id[i], &pv);
		u_printf("[flow]: wait udec_instance_thread(%d) exit done(ret %d).\n", i, ret);
	}

	//exit UDEC mode
	u_printf("[flow]: before enter idle\n");
	ioctl_enter_idle(iav_fd);
	u_printf("[flow]: enter idle done\n");

	//close opened file
	for (i = 0; i < total_num; i++) {
		if (params[i].file_fd[0]) {
			fclose(params[i].file_fd[0]);
			params[i].file_fd[0] = NULL;
		}

		if (params[i].file_fd[1]) {
			fclose(params[i].file_fd[1]);
			params[i].file_fd[1] = NULL;
		}
	}

	free(udec_configs);
	free(windows);
	free(renders);

	return 0;
}

static void init_internal()
{
	u32 i;

	//display windows
	for (i = 0; i < MAX_FILES; i++) {
		display_window[i].win_id = i;
		display_window[i].set_by_user = 0;
		display_window[i].input_offset_x = 0;
		display_window[i].input_offset_y = 0;
		display_window[i].input_width = 0;
		display_window[i].input_height = 0;
		display_window[i].target_win_offset_x = 0;
		display_window[i].target_win_offset_y = 0;
		display_window[i].target_win_width = 0;
		display_window[i].target_win_height = 0;
	}
}

int main(int argc, char **argv)
{
	int ret = 0;
	int iav_fd = -1;

	u_printf("[flow]: mudec start...\n");

	init_internal();

	if ((ret = init_mdec_params(argc, argv)) < 0) {
		u_printf_error("mudec: params invalid.\n");
		return -2;
	}

	u_printf("[flow]: before open_iav().\n");
	// open the device
	if ((iav_fd = open_iav()) < 0) {
		u_printf_error("open_iav fail.\n");
		return -1;
	}

	u_printf("[flow]: before do_test_mdec(%d).\n", iav_fd);
	do_test_mdec(iav_fd);

	u_printf("[flow]: close(iav_fd %d).\n", iav_fd);
	// close the device
	close(iav_fd);

	u_printf("[flow]: mudec exit...\n");
	return 0;
}


