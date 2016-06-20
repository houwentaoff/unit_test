
/*
 * test_stream.c
 * the program can read bitstream from BSB buffer and dump to different files
 * in different recording session, if there is no stream being recorded, then this
 * program will be in idle loop waiting and do not exit until explicitly closed.
 *
 * this program may stop encoding if the encoding was started to be "limited frames",
 * or stop all encoding when this program was forced to quit
 *
 * History:
 *	2009/12/31 - [Louis Sun] modify to use EX functions
 *	2012/01/11 - [Jian Tang] add different transfer method for streams
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

#include <signal.h>
#include <basetypes.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "datatx_lib.h"


#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

//#define ENABLE_RT_SCHED
#define MAX_ENCODE_STREAM_NUM	(4)
#define BASE_PORT			(2000)

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#define COLOR_PRINT0(msg)		printf("\033[34m"msg"\033[39m")
#define COLOR_PRINT(msg, arg...)		printf("\033[34m"msg"\033[39m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)


//debug options
#define DEBUG_PRINT_FRAME_INFO

#define ACCURATE_FPS_CALC

#define GET_STREAMID(x)	((((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) ? -1: (x))

typedef struct transfer_method {
	trans_method_t method;
	int port;
	char filename[256];
} transfer_method;

// the device file handle
int fd_iav;

// the bitstream buffer
u8 *bsb_mem;
u32 bsb_size;

static int nofile_flag = 0;
static int frame_info_flag = 0;
static int show_pts_flag = 0;
static int check_pts_flag = 0;
static int file_size_flag = 0;
static int file_size_mega_byte = 100;
static int remove_time_string_flag = 0;

static int fps_statistics_interval = 300;
static int print_interval = 30;


//create files for writing
static int init_none, init_nfs, init_tcp, init_usb, init_stdout;
static transfer_method stream_transfer[MAX_ENCODE_STREAM_NUM];
static int default_transfer_method = TRANS_METHOD_NFS;
static int transfer_port = BASE_PORT;

// bitstream filename base
static char filename[256];
const char *default_filename;
const char *default_filename_nfs = "/mnt/test";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";

//verbose
static int verbose_mode = 0;

// options and usage
#define NO_ARG		0
#define HAS_ARG		1

#define TRANSFER_OPTIONS_BASE		0
#define INFO_OPTIONS_BASE			10
#define TEST_OPTIONS_BASE			20
#define MISC_OPTIONS_BASE			40

enum numeric_short_options {
	FILENAME = TRANSFER_OPTIONS_BASE,
	NOFILE_TRANSER,
	TCP_TRANSFER,
	USB_TRANSFER,
	STDOUT_TRANSFER,

	TOTAL_FRAMES = INFO_OPTIONS_BASE,
	TOTAL_SIZE,
	FILE_SIZE,
	SAVE_FRAME_INFO,
	SHOW_PTS_INFO,
	CHECK_PTS_INFO,

	DURATION_TEST = TEST_OPTIONS_BASE,
	REMOVE_TIME_STRING,
};

static struct option long_options[] = {
	{"filename",	HAS_ARG,	0,	'f'},
	{"tcp",		NO_ARG,		0,	't'},
	{"usb",		NO_ARG,		0,	'u'},
	{"stdout",	NO_ARG,		0,	'o'},
	{"nofile",		NO_ARG,		0,	NOFILE_TRANSER},
	{"frames",	HAS_ARG,	0,	TOTAL_FRAMES},
	{"size",		HAS_ARG,	0,	TOTAL_SIZE},
	{"file-size",	HAS_ARG,	0,	FILE_SIZE},
	{"frame-info",	NO_ARG,		0,	SAVE_FRAME_INFO},
	{"show-pts",	NO_ARG,		0,	SHOW_PTS_INFO},
	{"check-pts",	NO_ARG,		0,	CHECK_PTS_INFO},
	{"rm-time",	NO_ARG,		0,	REMOVE_TIME_STRING},
	{"fps-intvl",	HAS_ARG,	0,	'i'},
	{"frame-intvl",	HAS_ARG,	0,	'n'},
	{"streamA",	NO_ARG,		0,	'A'},   // -A xxxxx	means all following configs will be applied to stream A
	{"streamB",	NO_ARG,		0,	'B'},
	{"streamC",	NO_ARG,		0,	'C'},
	{"streamD",	NO_ARG,		0,	'D'},
	{"verbose",	NO_ARG,		0,	'v'},
	{0, 0, 0, 0}
};

static const char *short_options = "f:tuoi:n:ABCDv";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"filename", "specify filename"},
	{"", "\t\tuse tcp (ps) to receive bitstream"},
	{"", "\t\tuse usb (us_pc2) to receive bitstream"},
	{"", "\t\treceive data from BSB buffer, and write to stdout"},
	{"", "\t\tjust receive data from BSB buffer, but do not write file"},
	{"frames", "\tspecify how many frames to encode"},
	{"size", "\tspecify how many bytes to encode"},
	{"size", "\tcreate new file to capture when size is over maximum (MB)"},
	{"", "\t\tgenerate frame info"},
	{"", "\t\tshow stream pts info"},
	{"", "\t\tcheck stream pts info"},
	{"", "\t\tremove time string from the file name"},
	{"", "\t\tset fps statistics interval"},
	{"", "\tset frame/field statistic interval"},
	{"", "\t\tstream A"},
	{"", "\t\tstream B"},
	{"", "\t\tstream C"},
	{"", "\t\tstream D"},
	{"", "\t\tprint more information"},

};

void usage(void)
{
	int i;
	printf("test_stream usage:\n");
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


typedef struct stream_encoding_state_s{
	int session_id;	//stream encoding session
	int fd;		//stream write file handle
	int fd_info;	//info write file handle
	u32 total_frames; // count how many frames encoded, help to divide for session before session id is implemented
	u64 total_bytes;  //count how many bytes encoded
	int pic_type;	  //picture type,  non changed during encoding, help to validate encoding state.
	u32 pts;
	u64 monotonic_pts;

	struct timeval capture_start_time;	//for statistics only,  this "start" is captured start, may be later than actual start

#ifdef ACCURATE_FPS_CALC
	int 	 total_frames2;	//for statistics only
	struct timeval time2;	//for statistics only
#endif

} stream_encoding_state_t;

static stream_encoding_state_t encoding_states[MAX_ENCODE_STREAM_NUM];
static stream_encoding_state_t old_encoding_states[MAX_ENCODE_STREAM_NUM];  //old states for statistics only

int init_param(int argc, char **argv)
{
	int i, ch;
	int option_index = 0;
	int current_stream = -1;
	transfer_method *trans;
	char str[][16] = { "NONE", "NFS", "TCP", "USB", "STDOUT"};

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		stream_transfer[i].method = TRANS_METHOD_UNDEFINED;
	}

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'A':
			current_stream = 0;
			break;
		case 'B':
			current_stream = 1;
			break;
		case 'C':
			current_stream = 2;
			break;
		case 'D':
			current_stream = 3;
			break;
		case 'f':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream < 0) {
				strcpy(filename, optarg);
			} else {
				strcpy(stream_transfer[current_stream].filename, optarg);
			}
			break;
		case 't':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_TCP;
			}
			default_transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_USB;
			}
			default_transfer_method = TRANS_METHOD_USB;
			break;
		case 'o':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_STDOUT;
			}
			default_transfer_method = TRANS_METHOD_STDOUT;
			break;
		case 'i':
			fps_statistics_interval = atoi(optarg);
			break;
		case 'n':
			print_interval = atoi(optarg);
			break;
		case NOFILE_TRANSER:
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_NONE;
			}
			default_transfer_method = TRANS_METHOD_NONE;
			break;
		case TOTAL_FRAMES:
			break;
		case TOTAL_SIZE:
			break;
		case FILE_SIZE:
			file_size_flag = 1;
			file_size_mega_byte = atoi(optarg);
			break;
		case SAVE_FRAME_INFO:
			frame_info_flag = 1;
			break;
		case SHOW_PTS_INFO:
			show_pts_flag = 1;
			break;
		case CHECK_PTS_INFO:
			check_pts_flag = 1;
			break;
		case REMOVE_TIME_STRING:
			remove_time_string_flag = 1;
			break;

		case 'v':
			verbose_mode = 1;
			break;
		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		trans = &stream_transfer[i];
		trans->port = transfer_port + i * 2;
		if (strlen(trans->filename) > 0) {
			if (trans->method == TRANS_METHOD_UNDEFINED)
				trans->method = TRANS_METHOD_NFS;
		} else {
			if (trans->method == TRANS_METHOD_UNDEFINED) {
				trans->method = default_transfer_method;
			}
			switch (trans->method) {
			case TRANS_METHOD_NFS:
			case TRANS_METHOD_STDOUT:
				if (strlen(filename) == 0)
					default_filename = default_filename_nfs;
				else
					default_filename = filename;
				break;
			case TRANS_METHOD_TCP:
			case TRANS_METHOD_USB:
				if (strlen(filename) == 0)
					default_filename = default_filename_tcp;
				else
					default_filename = filename;
				break;
			default:
				default_filename = NULL;
				break;
			}
			if (default_filename != NULL)
				strcpy(trans->filename, default_filename);
		}
		printf("Stream %c %s: %s\n", 'A' + i, str[trans->method], trans->filename);
	}

	return 0;
}



int init_encoding_states(void)
{
	int i;
	//init all file hander and session id to invalid at start
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		encoding_states[i].fd = -1;
		encoding_states[i].fd_info = -1;
		encoding_states[i].session_id = -1;
		encoding_states[i].total_bytes = 0;
		encoding_states[i].total_frames = 0;
		encoding_states[i].pic_type = 0;
		encoding_states[i].pts = 0;
	}
	return 0;
}


//return 0 if it's not new session,  return 1 if it is new session
int is_new_session(bits_info_ex_t * bits_info)
{
	int stream_id = bits_info->stream_id;
	int new_session = 0 ;
	if  (bits_info ->session_id != encoding_states[stream_id].session_id) {
		//a new session
		new_session = 1;
	}
	if (file_size_flag) {
		if ((encoding_states[stream_id].total_bytes / 1024) > (file_size_mega_byte * 1024))
			new_session = 1;
	}

	return new_session;
}

#include <time.h>


int get_time_string( char * time_str,  int len)
{
	time_t  t;
	struct tm * tmp;

	t= time(NULL);
	tmp = gmtime(&t);
	if (strftime(time_str, len, "%m%d%H%M%S", tmp) == 0) {
		printf("date string format error \n");
		return -1;
	}

	return 0;
}

#define VERSION	0x00000005
#define PTS_IN_ONE_SECOND		(90000)
int write_frame_info_header(int stream_id)
{
	iav_h264_config_ex_t config;
	int version = VERSION;
	u32 size = sizeof(config);
	int fd_info = encoding_states[stream_id].fd_info;
	int method = stream_transfer[stream_id].method;

	config.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &config);
	if (amba_transfer_write(fd_info, &version, sizeof(int), method) < 0 ||
		amba_transfer_write(fd_info, &size, sizeof(u32), method) < 0 ||
		amba_transfer_write(fd_info, &config, sizeof(config), method) < 0) {
		perror("write_data(4)");
		return -1;
	}

	return 0;
}

int write_frame_info(bits_info_ex_t * bits_info)
{
	typedef struct video_frame_s {
		u32     size;
		u32     pts;
		u32     pic_type;
		u32     reserved;
	} video_frame_t;
	video_frame_t frame_info;
	frame_info.pic_type = bits_info->pic_type;
	frame_info.pts = bits_info->PTS;
	frame_info.size = bits_info->pic_size;
	int stream_id = bits_info->stream_id;
	int fd_info = encoding_states[stream_id].fd_info;
	int method = stream_transfer[stream_id].method;

	if (amba_transfer_write(fd_info, &frame_info, sizeof(frame_info), method) < 0) {
		perror("write(5)");
		return -1;
	}
	return 0;
}

//check session and update file handle for write when needed
int check_session_file_handle(bits_info_ex_t * bits_info,  int new_session)
{
	char write_file_name[1024];
	char time_str[256];
	int stream_id = bits_info->stream_id;
	char stream_name;
	int method = stream_transfer[stream_id].method;
	int port = stream_transfer[stream_id].port;

   	if (new_session) {
		//close old session if needed
		if (encoding_states[stream_id].fd > 0) {
			close(encoding_states[stream_id].fd);
			encoding_states[stream_id].fd = -1;
		}
		//character based stream name
		stream_name = 'A' + stream_id;

		get_time_string(time_str, sizeof(time_str));
		if (remove_time_string_flag) {
			memset(time_str, 0, sizeof(time_str));
		}
		sprintf(write_file_name, "%s_%c_%s_%x.%s",
			stream_transfer[stream_id].filename, stream_name,
			time_str, bits_info->session_id,
			(bits_info->pic_type == JPEG_STREAM)? "mjpeg":"h264");
		if ((encoding_states[stream_id].fd =
			amba_transfer_open(write_file_name, method, port)) < 0) {
			printf("create file for write failed %s \n", write_file_name);
			return -1;
		} else {
			if (!nofile_flag) {
				printf("\nnew session file name [%s], fd [%d] \n", write_file_name,
					encoding_states[stream_id].fd);
			}
		}

		if (frame_info_flag) {
			sprintf(write_file_name, "%s.info", write_file_name);
			if ((encoding_states[stream_id].fd_info =
				amba_transfer_open(write_file_name, method, port)) < 0) {
				printf("create file for frame info  failed %s \n", write_file_name);
				return -1;
			}
			if (write_frame_info_header(stream_id) < 0) {
				printf("write h264 header info failed %s \n", write_file_name);
				return -1;
			}
		}
	}
	return 0;
}

int update_session_data(bits_info_ex_t * bits_info, int new_session)
{
	int stream_id = bits_info->stream_id;
	//update pic type, session id on new session
	if (new_session) {
		encoding_states[stream_id].pic_type = bits_info->pic_type;
		encoding_states[stream_id].session_id = bits_info->session_id;
		encoding_states[stream_id].total_bytes = 0;
		encoding_states[stream_id].total_frames = 0;
		old_encoding_states[stream_id] = encoding_states[stream_id];	//for statistics data only

#ifdef ACCURATE_FPS_CALC
		old_encoding_states[stream_id].total_frames2 = 0;
		old_encoding_states[stream_id].time2 = old_encoding_states[stream_id].capture_start_time;	//reset old counter
#endif
	}

	//update statistics on all frame
	encoding_states[stream_id].total_bytes += bits_info->pic_size;
	encoding_states[stream_id].total_frames++;
	encoding_states[stream_id].pts = bits_info->PTS;

	return 0;
}

int write_video_file(bits_info_ex_t * bits_info)
{
	static unsigned int whole_pic_size=0;
	u32 pic_size = bits_info->pic_size;
	int fd = encoding_states[bits_info->stream_id].fd;
	int stream_id = bits_info->stream_id;

	//remove align
	whole_pic_size  += (pic_size & (~(1<<23)));

	if (pic_size>>23) {
		//end of frame
		pic_size = pic_size & (~(1<<23));
	 	 //padding some data to make whole picture to be 32 byte aligned
		pic_size += (((whole_pic_size + 31) & ~31)- whole_pic_size);
		//rewind whole pic size counter
		// printf("whole %d, pad %d \n", whole_pic_size, (((whole_pic_size + 31) & ~31)- whole_pic_size));
		 whole_pic_size = 0;
	}

	if (bits_info->start_addr + pic_size <= (u32)bsb_mem + bsb_size) {
		if (amba_transfer_write(fd, (void*)bits_info->start_addr,
			pic_size, stream_transfer[stream_id].method) < 0) {
			perror("write(1)");
			return -1;
		}
	} else {
		u32 size = (u32)bsb_mem + bsb_size - bits_info->start_addr;
		u32 remain = pic_size - size;
		if (amba_transfer_write(fd, (void*)bits_info->start_addr, size, stream_transfer[stream_id].method) < 0) {
			perror("write(2)");
			return -1;
		}
		if (amba_transfer_write(fd, bsb_mem, remain, stream_transfer[stream_id].method) < 0) {
			perror("write(3)");
			return -1;
		}
	}

	return 0;
}

int write_stream(int *total_frames, u64 *total_bytes)
{
	int new_session; //0:  old session  1: new session
	int print_frame = 1;
	u32 time_interval_us;
#ifdef ACCURATE_FPS_CALC
	u32 time_interval_us2;
#endif
	int stream_id;
	struct timeval pre_time, curr_time;
	int pre_frames ,curr_frames;
	u64 pre_bytes, curr_bytes;
	u32 pre_pts, curr_pts, curr_vin_fps;
	bits_info_ex_t  bits_info;
	char stream_name[128];
	static int end_of_stream[MAX_ENCODE_STREAM_NUM] = {1, 1, 1, 1};
	iav_encode_stream_info_ex_t stream_info;
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
		if (stream_info.state == IAV_STREAM_STATE_ENCODING) {
			end_of_stream[i] = 0;
		}
	}
	// There is no encoding stream, skip to next return
	if (end_of_stream[0] && end_of_stream[1] && end_of_stream[2] && end_of_stream[3])
		return -1;

	if (ioctl(fd_iav, IAV_IOC_READ_BITSTREAM_EX, &bits_info) < 0) {
		if (errno != EAGAIN)
			perror("IAV_IOC_READ_BITSTREAM_EX");
		return -1;
	}

	//update current frame encoding time
	stream_id = bits_info.stream_id;
	gettimeofday(&encoding_states[stream_id].capture_start_time, NULL);

#ifdef DEBUG_PRINT_FRAME_INFO
	if (verbose_mode) {
		printf("type=%d, frmNo=%d, PTS=%d, size=%d, addr=0x%x, strm_id=%d,"
			" sesn_id=%u, monotonic_pts=%lld, mono_diff=%lld\n",
			bits_info.pic_type, bits_info.frame_num, bits_info.PTS, bits_info.pic_size,
			bits_info.start_addr, bits_info.stream_id,
			bits_info.session_id, bits_info.monotonic_pts,
			(bits_info.monotonic_pts - old_encoding_states[stream_id].monotonic_pts));
		old_encoding_states[stream_id].monotonic_pts = bits_info.monotonic_pts;
	}
#endif

	//check if it's a stream end null frame indicator
	if (bits_info.stream_end) {
		end_of_stream[stream_id] = 1;
		//printf("close file of stream %d at end, session id %d \n", stream_id, bits_info.session_id);
		if (encoding_states[stream_id].fd > 0) {
			amba_transfer_close(encoding_states[stream_id].fd,
				stream_transfer[stream_id].method);
			encoding_states[stream_id].fd = -1;
		}
		if (encoding_states[stream_id].fd_info > 0) {
			amba_transfer_close(encoding_states[stream_id].fd,
				stream_transfer[stream_id].method);
			encoding_states[stream_id].fd_info = -1;
		}
		return 0;
	}

	//check if it's new record session, since file name and recording control are based on session,
	//session id and change are important data
	new_session = is_new_session(&bits_info);
	//update session data
	if (update_session_data(&bits_info, new_session) < 0) {
		printf("update session data failed \n");
		return -2;
	}

	//check and update session file handle
	if (check_session_file_handle(&bits_info, new_session) < 0) {
		printf("check session file handle failed \n");
		return -3;
	}

	if (frame_info_flag) {
		if (write_frame_info(&bits_info) < 0) {
			printf("write video frame info failed for stream %d, session id = %d.\n",
				stream_id, bits_info.session_id);
			return -5;
		}
	}

	//write file if file is still opened
	if (write_video_file(&bits_info) < 0) {
		printf("write video file failed for stream %d, session id = %d \n",
			stream_id, bits_info.session_id);
		return -4;
	}

	//update global statistics
	if (total_frames)
		*total_frames = (*total_frames) + 1;
	if (total_bytes)
		*total_bytes = (*total_bytes) + bits_info.pic_size;

	//print statistics
	pre_time = old_encoding_states[stream_id].capture_start_time;
	curr_time = encoding_states[stream_id].capture_start_time;
	pre_frames = old_encoding_states[stream_id].total_frames;
	curr_frames = encoding_states[stream_id].total_frames;
	pre_bytes = old_encoding_states[stream_id].total_bytes;
	curr_bytes = encoding_states[stream_id].total_bytes;
	pre_pts = old_encoding_states[stream_id].pts;
	curr_pts = encoding_states[stream_id].pts;
	if (show_pts_flag) {
		AM_IOCTL(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, &curr_vin_fps);
		time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
			curr_time.tv_usec - pre_time.tv_usec;
		sprintf(stream_name, "stream %c", 'A' + stream_id);
		printf("%s: [%d]\tVIN: [%d], PTS: %d, diff: %d, frames NO: %d, size: %d\n",
			stream_name, time_interval_us, curr_vin_fps,
			curr_pts, (curr_pts - pre_pts), curr_frames, bits_info.pic_size);
		old_encoding_states[stream_id].pts = encoding_states[stream_id].pts;
		old_encoding_states[stream_id].capture_start_time =
			encoding_states[stream_id].capture_start_time;
	}
#if 0		// to be done
	if (check_pts_flag) {
		iav_h264_config_ex_t config;
		int den, pts_delta, current_fps;
		u32 custom_encoder_frame_rate;
		config.id = (1 << stream_id);
		if (ioctl(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &config) < 0) {
			perror("IAV_IOC_GET_H264_CONFIG_EX");
			return -1;
		}
		custom_encoder_frame_rate = config.custom_encoder_frame_rate;
		den = ((custom_encoder_frame_rate & (1 << 30)) >> 30) ? 1001 : 1000;
		current_fps = (custom_encoder_frame_rate & ~(3 << 30));
		pts_delta = PTS_IN_ONE_SECOND * den / current_fps;
		old_encoding_states[stream_id].pts = encoding_states[stream_id].pts;
		if ((curr_pts != 0) && (pre_pts != 0) && (curr_pts - pre_pts) % pts_delta != 0) {
			printf("Check PTS Error: VIN FPS = %4.2f, PTS delta = %d, Curr PTS = %d, Pre PTS = %d\n",
				(current_fps * 1.0 / den), pts_delta, curr_pts, pre_pts);
			exit(-1);
		}
	}
#endif
	if ((curr_frames % print_interval == 0) && (print_frame)) {
		time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
						curr_time.tv_usec - pre_time.tv_usec;

		sprintf(stream_name, "stream %c",  'A'+ stream_id);
		printf("%s:\t%4d %s, %2d fps, %18lld\tbytes, %5d kbps\n", stream_name,
			curr_frames, nofile_flag ? "discard" : "frames",
			DIV_ROUND((curr_frames - pre_frames) * 1000000, time_interval_us), curr_bytes,
			pre_time.tv_sec ? (int)((curr_bytes - pre_bytes) * 8000000LL /time_interval_us /1024) : 0);
		//backup time and states
		old_encoding_states[stream_id].session_id = encoding_states[stream_id].session_id;
		old_encoding_states[stream_id].fd = encoding_states[stream_id].fd;
		old_encoding_states[stream_id].total_frames = encoding_states[stream_id].total_frames;
		old_encoding_states[stream_id].total_bytes = encoding_states[stream_id].total_bytes;
		old_encoding_states[stream_id].pic_type = encoding_states[stream_id].pic_type;
		old_encoding_states[stream_id].capture_start_time = encoding_states[stream_id].capture_start_time;
	}
	#ifdef ACCURATE_FPS_CALC
	{
		const int fps_statistics_interval = 900;
		int pre_frames2;
		struct timeval pre_time2;
		pre_frames2 = old_encoding_states[stream_id].total_frames2;
		pre_time2 = old_encoding_states[stream_id].time2;
		if ((curr_frames % fps_statistics_interval ==0) &&(print_frame)) {
			time_interval_us2 = (curr_time.tv_sec - pre_time2.tv_sec) * 1000000 +
						curr_time.tv_usec - pre_time2.tv_usec;
			double fps = (curr_frames - pre_frames2)* 1000000.0/(double)time_interval_us2;
			BOLD_PRINT("AVG FPS = %4.2f\n",fps);
			old_encoding_states[stream_id].total_frames2 = encoding_states[stream_id].total_frames;
			old_encoding_states[stream_id].time2 = encoding_states[stream_id].capture_start_time;
		}
	}
	#endif

	return 0;
}



int stop_all_encode(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_STOP_ENCODE, 0);

	printf("\nstop_encode\n");

	return 0;
}

//return 1 is IAV is in encoding,  else, return 0
int is_video_encoding(void)
{
	iav_state_info_t info;
	if (ioctl(fd_iav, IAV_IOC_GET_STATE_INFO, &info) < 0 ) {
		perror("IAV_IOC_GET_STATE_INFO");
		exit(1);
	}
	return (info.state == IAV_STATE_ENCODING);
}

int show_waiting(void)
{
	#define DOT_MAX_COUNT 10
	static int dot_count = DOT_MAX_COUNT;
	int i;

	if (dot_count < DOT_MAX_COUNT) {
		fprintf(stderr, ".");	//print a dot to indicate it's alive
		dot_count++;
	} else{
		fprintf(stderr, "\r");
		for ( i = 0; i < 80 ; i++)
			fprintf(stderr, " ");
		fprintf(stderr, "\r");
		dot_count = 0;
	}

	fflush(stderr);
	return 0;
}


int capture_encoded_video()
{
	int rval;
	//open file handles to write to
	int total_frames;
	u64 total_bytes;
	total_frames = 0;
	total_bytes =  0;

#ifdef ENABLE_RT_SCHED
	{
	    struct sched_param param;
	    param.sched_priority = 99;
	    if (sched_setscheduler(0, SCHED_FIFO, &param) < 0)
	        perror("sched_setscheduler");
	}
#endif

	while (1) {
		if ((rval = write_stream(&total_frames, &total_bytes)) < 0) {
			if (rval == -1) {
				usleep(100 * 1000);
				show_waiting();
			} else {
				printf("write_stream err code %d \n", rval);
			}
			continue;
		}
	}

	printf("stop encoded stream capture\n");

	printf("total_frames = %d\n", total_frames);
	printf("total_bytes = %lld\n", total_bytes);

	return 0;
}

static int init_transfer(void)
{
	int i, do_init, rtn = 0;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		do_init = 0;
		switch (stream_transfer[i].method) {
		case TRANS_METHOD_NONE:
			if (init_none == 0)
				do_init = init_none = 1;
			break;
		case TRANS_METHOD_NFS:
			if (init_nfs == 0)
				do_init = init_nfs = 1;
			break;
		case TRANS_METHOD_TCP:
			if (init_tcp == 0)
				do_init = init_tcp = 1;
			break;
		case TRANS_METHOD_USB:
			if (init_usb == 0)
				do_init = init_usb = 1;
			break;
		case TRANS_METHOD_STDOUT:
			if (init_stdout == 0)
				do_init = init_stdout = 1;
			break;
		default:
			return -1;
		}
		if (do_init)
			rtn = amba_transfer_init(stream_transfer[i].method);
		if (rtn < 0)
			return -1;
	}
	return 0;
}

static int deinit_transfer(void) {
	if (init_none > 0)
		init_none = amba_transfer_deinit(TRANS_METHOD_NONE);
	if (init_nfs > 0)
		init_nfs = amba_transfer_deinit(TRANS_METHOD_NFS);
	if (init_tcp > 0)
		init_tcp = amba_transfer_deinit(TRANS_METHOD_TCP);
	if (init_usb)
		init_usb = amba_transfer_deinit(TRANS_METHOD_USB);
	if (init_stdout)
		init_stdout = amba_transfer_deinit(TRANS_METHOD_STDOUT);
	if (init_none < 0 || init_nfs < 0 || init_tcp < 0 || init_usb < 0 || init_stdout < 0)
		return -1;
	return 0;
}

static void sigstop()
{
#if 0				// remove touch on encoding
	int state;
	if (ioctl(fd_iav, IAV_IOC_GET_STATE, &state) < 0) {
		perror("IAV_IOC_GET_STATE");
		exit(2);
	}

	//if it is encoding, then stop encoding
	if (state == IAV_STATE_ENCODING) {
		if (stop_all_encode() < 0) {
			printf("Cannot stop encoding...\n");
			exit(3);
		}
	}
#endif
	deinit_transfer();
	exit(1);
}


int map_bsb(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_BSB, &info) < 0) {
		perror("IAV_IOC_MAP_BSB");
		return -1;
	}
	bsb_mem = info.addr;
	bsb_size = info.length;

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;
	return 0;
}



int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		printf("init param failed \n");
		return -1;
	}

	if (map_bsb() < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	init_encoding_states();

	if (init_transfer() < 0) {
		return -1;
	}

	if (capture_encoded_video() < 0) {
		printf("capture encoded video failed \n");
		return -1;
	}

	if (deinit_transfer() < 0) {
		return -1;
	}

	close(fd_iav);
	return 0;
}


