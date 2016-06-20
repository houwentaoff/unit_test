
/*
 * test3.c
 *
 * History:
 *	2008/4/21 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
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
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "types.h"
#include "ambas_common.h"
#include "iav_drv.h"

#ifndef KB
#define KB	(1*1024)
#endif

#ifndef MB
#define MB	(1*1024*1024)
#endif


int fd_iav;

#include "../vout_test/vout_init.c"

// the bitstream buffer
u8 *bsb_mem;
u32 bsb_size;

enum {
	DECODE_NOTHING,
	DECODE_H264,
	DECODE_H264_ONLY,
	DECODE_H264_SIMPLE,
	DECODE_RAW_NV12,
	DECODE_JPEG,
	DECODE_MJPEG,
	DECODE_UDEC,
};

typedef struct udec_type_map_s {
	const char *name;
	int type;
} udec_type_map_t;

static udec_type_map_t udec_type_map[] = {
	{"h264", UDEC_H264},
	{"mp12", UDEC_MP12},
	{"mp4h", UDEC_MP4H},
	{"mp4s", UDEC_MP4S},
	{"vc1", UDEC_VC1},
	{"rv40", UDEC_RV40},
	{"jpeg", UDEC_JPEG},
	{"sw", UDEC_SW},
};

int get_udec_type(const char *name)
{
	int i;
	for (i = 0; i < sizeof(udec_type_map)/sizeof(udec_type_map[0]); i++)
		if (strcmp(name, udec_type_map[i].name) == 0)
			return udec_type_map[i].type;
	return -1;
}

int decode_flag = DECODE_NOTHING;
int udec_type = UDEC_VC1;

int disable_output = 0;
int eos_flag = 0;
int repeat_times = 1;

char filename[256];
const char *default_jpeg_filename = "/mnt/media/test.jpg";
const char *default_h264_filename = "/mnt/media/test";
const char *default_nv12_filename = "/mnt/media/test.nv12";

int output_flag;
int output_fd;
char output_filename[256];

iav_h264_config_t h264_config;
int config_size;

int decoded_frame_count;
int rendered_frame_count;

int swdec_test = 0;
int fb_test = 0;

#define NO_ARG				0
#define HAS_ARG				1
#define	VOUT_OPTIONS_BASE		20
#define DECODING_OPTIONS_BASE		190

enum numeric_short_options {
	//Vout
	VOUT_NUMERIC_SHORT_OPTIONS,

	//Decoding
	APPEND_EOS,
	SWDEC_TEST,
	FB_TEST,
};

static struct option long_options[] = {
	//Vout
	VOUT_LONG_OPTIONS()
	{"no-vout", NO_ARG, 0, 'd'},

	//Decoding
	{"filename", HAS_ARG, 0, 'f'},
	{"output", HAS_ARG, 0, 'o'},
	{"repeat", HAS_ARG, 0, 'r'},
	{"eos", NO_ARG, 0, APPEND_EOS},

	{"h264", NO_ARG, 0, 'h'},
	{"h264-only", NO_ARG, 0, 'H'},
	{"simple-h264", NO_ARG, 0, 's'},
	{"mjpeg", NO_ARG, 0, 'm'},
	{"jpeg", NO_ARG, 0, 'j'},
	{"nv12", NO_ARG, 0, 'n'},
	{"udec", HAS_ARG, 0, 'u'},
	{"soft", NO_ARG, 0, SWDEC_TEST},
	{"fb", NO_ARG, 0, FB_TEST},

	{0, 0, 0, 0}
};


static const char *short_options = "df:hjmr:Hnv:o:su:V:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	//Vout
	VOUT_PARAMETER_HINTS()
	{"",		"\t\t"	"do not send decoded pictures to vout"},

	//Decoding
	{"filename",	""	"specify filename"},
	{"filename",	"\t"	"save decoded pictures to the file"},
	{"times",	"\t"	"repeat playback times. 0 means forever"},
	{"",		"\t\t"	"append eos at bitstream end"},

	{"",		"\t\t"	"decode h.264"},
	{"",		"\t\t"	"decode h.264 only"},
	{"",		"\t"	"decode h.264 using simple decoder"},
	{"",		"\t\t"	"decode mjpeg"},
	{"",		"\t\t"	"decode jpeg"},
	{"",		"\t\t"	"display nv12 raw pictures"},
	{"udec type",	"\t"	"universal decoder"},
	{"",		"\t\t"	"test soft decoder"},
	{"",		"\t\t\t""test frame buffer"},
};

int map_bsb(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_DECODE_BSB, &info) < 0) {
		perror("IAV_IOC_MAP_DECODE_BSB");
		return -1;
	}
	bsb_mem = info.addr;
	bsb_size = info.length;
	mem_mapped = 1;

	memset(bsb_mem, 0, bsb_size);

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	return 0;
}

int map_dsp(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	printf("dsp_mem = 0x%x, size = 0x%x\n", (u32)info.addr, info.length);
	return 0;
}

int select_channel(void)
{
	int channel = 0;

	if (ioctl(fd_iav, IAV_IOC_SELECT_CHANNEL, IAV_DEC_CHANNEL(channel)) < 0) {
		perror("IAV_IOC_SELECT_CHANNEL");
		return -1;
	}

	printf("set channel to %d\n", channel);
	return 0;
}

int enter_decoding(void)
{
	ioctl(fd_iav, IAV_IOC_STOP_DECODE, 0);

	if (ioctl(fd_iav, IAV_IOC_START_DECODE, 0) < 0) {
		perror("IAV_IOC_START_DECODE");
		return -1;
	}

	printf("enter_decoding done\n");
	return 0;
}

int get_file_size(int fd)
{
	struct stat stat;

	if (fstat(fd, &stat) < 0) {
		perror("fstat");
		return -1;
	}

	return stat.st_size;
}

int open_file(const char *name, u32 *size)
{
	int fd;

	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		perror(name);
		return -1;
	}

	if (size != NULL)
		*size = get_file_size(fd);

	return fd;
}

int read_file(int fd, void *buffer, u32 size)
{
	if (read(fd, buffer, size) != size) {
		perror("read");
		return -1;
	}

	return 0;
}

int decode_jpeg(void)
{
	int fd;
	u32 size;
	u32 round_size;
	iav_jpeg_info_t info;

	if (filename[0] == '\0')
		strcpy(filename, default_jpeg_filename);

	if ((fd = open_file(filename, &size)) < 0)
		return -1;

	if (read_file(fd, bsb_mem, size) < 0) {
		close(fd);
		return -1;
	}

	close(fd);

	round_size = (size + 31) & ~31;
	memset(bsb_mem + size, 0, round_size - size);
	size = round_size;

	info.start_addr = bsb_mem;
	info.size = size;
	if (ioctl(fd_iav, IAV_IOC_DECODE_JPEG, &info) < 0) {
		perror("IAV_IOC_DECODE_JPEG");
		return -1;
	}

	printf("decode %s done\n", filename);
	return 0;
}

int info_file_size(void)
{
	return 2*sizeof(int) + config_size;
}

u8 *copy_to_bsb(u8 *ptr, u8 *buffer, u32 size)
{
	if (ptr + size <= bsb_mem + bsb_size) {
		memcpy(ptr, buffer, size);
		return ptr + size;
	} else {
		int room = (bsb_mem + bsb_size) - ptr;
		u8 *ptr2;
		memcpy(ptr, buffer, room);
		ptr2 = buffer + room;
		size -= room;
		memcpy(bsb_mem, ptr2, size);
		return bsb_mem + size;
	}
}

#define GOP_HEADER_SIZE		22
u8 *fill_gop_header(u8 *ptr, u32 pts)
{
	u8 buf[GOP_HEADER_SIZE];
	u32 tick_high = (h264_config.pic_info.rate >> 16) & 0x0000ffff;
	u32 tick_low = h264_config.pic_info.rate & 0x0000ffff;
	u32 scale_high = (h264_config.pic_info.scale >> 16) & 0x0000ffff;
	u32 scale_low = h264_config.pic_info.scale & 0x0000ffff;
	u32 pts_high = (pts >> 16) & 0x0000ffff;
	u32 pts_low = pts & 0x0000ffff;

	buf[0] = 0;			// start code
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 1;

	buf[4] = 0x7a;			// NAL header
	buf[5] = 0x01;			// version main
	buf[6] = 0x01;			// version sub

	buf[7] = tick_high >> 10;
	buf[8] = tick_high >> 2;
	buf[9] = (tick_high << 6) | (1 << 5) | (tick_low >> 11);
	buf[10] = tick_low >> 3;

	buf[11] = (tick_low << 5) | (1 << 4) | (scale_high >> 12);
	buf[12] = scale_high >> 4;
	buf[13] = (scale_high << 4) | (1 << 3) | (scale_low >> 13);
	buf[14] = scale_low >> 5;

	buf[15] = (scale_low << 3) | (1 << 2) | (pts_high >> 14);
	buf[16] = pts_high >> 6;

	buf[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
	buf[18] = pts_low >> 7;
	buf[19] = (pts_low << 1) | 1;

	buf[20] = h264_config.N;
	buf[21] = (h264_config.M << 4) & 0xf0;

	return copy_to_bsb(ptr, buf, sizeof(buf));
}

u8 *fill_eos(u8 *ptr)
{
	static u8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
	return copy_to_bsb(ptr, eos, sizeof(eos));
}

typedef struct video_frame_s {
	u32	size;
	u32	pts;
	u32	flags;
	u32	seq;
} video_frame_t;

typedef struct decode_var_s {
	int	fd;
	int	fd_size;
	u32	info_size;
	int	frame_count;
} decode_var_t;

decode_var_t Gvar;

int open_h264_files(void)
{
	int len;
	char full_filename[256];

	if (filename[0] == '\0')
		strcpy(filename, default_h264_filename);

	len = strlen(filename);
	if (len < 4 || strcmp(filename + len - 4, ".264") != 0) {
		sprintf(full_filename, "%s.264", filename);
		if ((Gvar.fd = open_file(full_filename, NULL))  < 0)
			return -1;

		sprintf(full_filename, "%s.264.info", filename);
		if ((Gvar.fd_size = open_file(full_filename, &Gvar.info_size)) < 0)
			return -1;
	} else {
		if ((Gvar.fd = open_file(filename, NULL)) < 0)
			return -1;

		sprintf(full_filename, "%s.info", filename);
		if ((Gvar.fd_size = open_file(full_filename, &Gvar.info_size)) < 0)
			return -1;
	}

	return 0;
}

int read_h264_header_info(void)
{
	int version;

	if (read(Gvar.fd_size, &version, sizeof(version)) < 0 ||
		read(Gvar.fd_size, &config_size, sizeof(config_size)) < 0) {
		perror("read");
		return -1;
	}

	if (version != 0x00000004 && version != 0x00000003) {
		printf("version is not correct\n");
		return -1;
	}

	if (read(Gvar.fd_size, &h264_config, config_size) < 0) {
		perror("read");
		return -1;
	}

	Gvar.frame_count = (Gvar.info_size - info_file_size()) / sizeof(video_frame_t);
	printf("total frames = %d\n", Gvar.frame_count);

	if (Gvar.frame_count <= 0) {
		printf("file is empty\n");
		return -1;
	}

	printf("           M = %d\n", h264_config.M);
	printf("           N = %d\n", h264_config.N);
	printf("   gop_model = 0x%x\n", h264_config.gop_model);
	printf("idr_interval = %d\n", h264_config.idr_interval);
	printf("     bitrate = %d\n", h264_config.average_bitrate);
	printf("  frame_mode = %d\n", h264_config.pic_info.frame_mode);
	printf("       scale = %d\n", h264_config.pic_info.scale);
	printf("        rate = %d\n", h264_config.pic_info.rate);
	printf("       width = %d\n", h264_config.pic_info.width);
	printf("      height = %d\n", h264_config.pic_info.height);

	return 0;
}

int create_output_file(const char *filename)
{
	int fd;

	if ((fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(filename);
		return -1;
	}

	return fd;
}

int decode_frame(u8 *start_addr, u8 *end_addr, u32 num_pics)
{
	iav_h264_decode_t decode_info;

	decode_info.start_addr = start_addr;
	decode_info.end_addr = end_addr;
	decode_info.first_display_pts = 0;
	decode_info.num_pics = num_pics;
	decode_info.next_size = 0;
	decode_info.pic_width = 0;
	decode_info.pic_height = 0;

	if (ioctl(fd_iav, IAV_IOC_DECODE_H264, &decode_info) < 0) {
		perror("IAV_IOC_DECODE_H264");
		return -1;
	}

	decoded_frame_count++;

	return 0;
}

typedef struct nv12_header_s {
	u8	id[16];
	u8	subtype[16];
	u16	major;
	u16	minor;
} nv12_header_t;

const nv12_header_t ofile_header = {
	{0x52, 0x3A, 0xFA, 0xA6, 0xA5, 0x8B, 0x48, 0x4D, 0xA6, 0x30, 0x0E, 0xC1, 0x60, 0xD4, 0xC4, 0x73},
	{0x25, 0xA9, 0xF2, 0xD0, 0xC8, 0xFB, 0x3D, 0x4A, 0xA2, 0x80, 0x18, 0x78, 0x56, 0x78, 0xCC, 0xCB},
	0, 0
};

typedef struct nv12_picture_s {
	u16	picture_size;
	u8	chroma_format;
	u8	reserved;

	u16	buffer_width;
	u16	buffer_height;

	u16	lu_off_x;
	u16	lu_off_y;

	u16	ch_off_x;
	u16	ch_off_y;

	u16	pic_width;
	u16	pic_height;
} nv12_picture_t;

int config_decoder_by_header(void)
{
	iav_config_decoder_t config;

	config.flags = 0;
	config.decoder_type = (decode_flag == DECODE_H264) ?
		IAV_DEFAULT_DECODER : IAV_SIMPLE_DECODER;
	if (h264_config.pic_info.width == 0 || h264_config.pic_info.height == 0) {
		config.pic_width = 1280;
		config.pic_height = 720;
	}
	else {
		config.pic_width = h264_config.pic_info.width;
		config.pic_height = h264_config.pic_info.height;
	}

	if (ioctl(fd_iav, IAV_IOC_CONFIG_DECODER, &config) < 0) {
		perror("IAV_IOC_CONFIG_DECODER");
		return -1;
	}

	return 0;
}

int config_decoder_by_pic(nv12_picture_t *pic)
{
	iav_config_decoder_t config;

	config.flags = 0;
	config.decoder_type = IAV_SOFTWARE_DECODER;
	config.chroma_format = pic->chroma_format;
	config.num_frame_buffer = 0;
	config.pic_width = pic->buffer_width;
	config.pic_height = pic->buffer_height;
	config.fb_width = 0;
	config.fb_height = 0;

	if (ioctl(fd_iav, IAV_IOC_CONFIG_DECODER, &config) < 0) {
		perror("IAV_IOC_CONFIG_DECODER");
		return -1;
	}

	return 0;
}

int render(void)
{
	iav_frame_buffer_t frame;

	frame.flags = 0;
	if (output_flag) {
		frame.flags |= IAV_FRAME_NEED_ADDR;
	}

	if (ioctl(fd_iav, IAV_IOC_GET_DECODED_FRAME, &frame) < 0) {
		perror("IAV_IOC_GET_DECODED_FRAME");
		return -1;
	}

	if (output_flag) {
		nv12_picture_t pic;
		size_t size;
/*
		printf("buffer_width = %d\n", frame.buffer_width);
		printf("buffer_height = %d\n", frame.buffer_height);
		printf("buffer_pitch = %d\n", frame.buffer_pitch);

		printf("pic_width = %d\n", frame.pic_width);
		printf("pic_height = %d\n", frame.pic_height);

		printf("lu_off_x = %d\n", frame.lu_off_x);
		printf("lu_off_y = %d\n", frame.lu_off_y);
		printf("ch_off_x = %d\n", frame.ch_off_x);
		printf("ch_off_y = %d\n", frame.ch_off_y);

		printf("lu_buf_addr = 0x%p\n", frame.lu_buf_addr);
		printf("ch_buf_addr = 0x%p\n", frame.ch_buf_addr);
*/
		pic.picture_size = sizeof(pic) + frame.buffer_pitch * frame.buffer_height * 3 / 2;
		pic.chroma_format = frame.chroma_format;
		pic.buffer_width = frame.buffer_pitch;
		pic.buffer_height = frame.buffer_height;
		pic.lu_off_x = frame.lu_off_x;
		pic.lu_off_y = frame.lu_off_y;
		pic.ch_off_x = frame.ch_off_x;
		pic.ch_off_y = frame.ch_off_y;
		pic.pic_width = frame.pic_width;
		pic.pic_height = frame.pic_height;

		if (write(output_fd, &pic, sizeof(pic)) != sizeof(pic)) {
			perror("write");
			return -1;
		}

		size = frame.buffer_pitch * frame.buffer_height;
		if (write(output_fd, frame.lu_buf_addr, size) != size) {
			perror("write");
			return -1;
		}

		size = frame.buffer_pitch * frame.buffer_height / 2;
		if (write(output_fd, frame.ch_buf_addr, size) != size) {
			perror("write");
			return -1;
		}
	}

	rendered_frame_count++;

	if (!disable_output) {
		if (ioctl(fd_iav, IAV_IOC_RENDER_FRAME, &frame) < 0) {
			perror("IAV_IOC_RENDER_FRAME");
			return -1;
		}
		printf("frame %d rendered (total %d), fb_id = %d\n",
			rendered_frame_count, Gvar.frame_count, frame.fb_id);
	} else {
		if (ioctl(fd_iav, IAV_IOC_RELEASE_FRAME, &frame) < 0) {
			perror("IAV_IOC_RELEASE_FRAME");
			return -1;
		}
		printf("decoded %d frames (total %d), fb_id = %d\n",
			rendered_frame_count, Gvar.frame_count, frame.fb_id);
	}

	return 0;
}

int decode_h264(void)
{
	video_frame_t frame;
	u8 *ptr;
	u8 *frame_start_ptr;
	int frame_index;
	iav_wait_decoder_t wait;
	u32 total_bytes;

	if (open_h264_files() < 0)
		return -1;

	if (read_h264_header_info() < 0)
		return -1;

	if (output_flag) {
		if (map_dsp() < 0)
			return -1;
		if ((output_fd = create_output_file(output_filename)) < 0)
			return -1;
		if (write(output_fd, &ofile_header, sizeof(ofile_header)) != sizeof(ofile_header)) {
			perror("write");
			return -1;
		}
	}

	if (config_decoder_by_header() < 0)
		return -1;

	ptr = bsb_mem;
	while (1) {
		frame_index = 0;
		total_bytes = 0;
		lseek(Gvar.fd, 0, SEEK_SET);
		lseek(Gvar.fd_size, info_file_size(), SEEK_SET);

		while (1) {
			frame_start_ptr = ptr;

			// read frame info
			if (read_file(Gvar.fd_size, &frame, sizeof(frame)) < 0)
				return -1;

			// wait bsb, or decoded frame
			while (1) {
				wait.emptiness.room = frame.size + GOP_HEADER_SIZE;
				wait.emptiness.start_addr = ptr;

				wait.flags = IAV_WAIT_BSB;
				if (decode_flag == DECODE_H264_SIMPLE)
					wait.flags |= IAV_WAIT_FRAME;

				if (ioctl(fd_iav, IAV_IOC_WAIT_DECODER, &wait) < 0) {
					if (errno != EAGAIN) {
						perror("IAV_IOC_WAIT_DECODER");
						return -1;
					}
					break;
				}

				if (wait.flags == IAV_WAIT_BSB) {
					// bsb has room, continue
					break;
				}

				if (wait.flags == IAV_WAIT_FRAME) {
					// frame(s) decoded, render to vout
					if (render() < 0)
						return -1;
				}
				else {
					printf("unknown flags returned by IAV_IOC_WAIT_DECODER: %d\n", wait.flags);
					return -1;
				}
			}

			// fill GOP header before IDR
			if (frame.flags == IDR_FRAME)
				ptr = fill_gop_header(ptr, frame.pts);

			// read data into bsb
			if (ptr + frame.size <= bsb_mem + bsb_size) {
				if (read_file(Gvar.fd, ptr, frame.size) < 0)
					return -1;
				ptr += frame.size;
			} else {
				printf("wrap around\n");
				u32 size = (bsb_mem + bsb_size) - ptr;
				if (read_file(Gvar.fd, ptr, size) < 0)
					return -1;
				size = frame.size - size;
				if (read_file(Gvar.fd, bsb_mem, size) < 0)
					return -1;
				ptr = bsb_mem + size;
			}

			// decode the frame
			if (decode_frame(frame_start_ptr, ptr, 1) < 0)
				return -1;

			total_bytes += frame.size;
			frame_index++;

			//if (decode_flag == DECODE_H264)
				printf("frames: %d/%d %d/%d\n", frame_index, Gvar.frame_count, frame.size, total_bytes);

			if (frame_index == Gvar.frame_count) {
				printf("decode done, total frames = %d\n", Gvar.frame_count);
				break;
			}
		}

		// wait EOS
		if (decode_flag == DECODE_H264_SIMPLE && eos_flag) {
			// send eos
			frame_start_ptr = ptr;
			ptr = fill_eos(ptr);
			if (decode_frame(frame_start_ptr, ptr, 0) < 0)
				return -1;

			// render the remaining frames until EOS
			while (1) {
				wait.flags = IAV_WAIT_FRAME | IAV_WAIT_EOS;
				if (ioctl(fd_iav, IAV_IOC_WAIT_DECODER, &wait) < 0) {
					perror("IAV_IOC_WAIT_DECODER");
					return -1;
				}
				if (wait.flags == IAV_WAIT_FRAME) {
					if (render() < 0)
						return -1;
				}
				else if (wait.flags == IAV_WAIT_EOS) {
					printf("EOS received !!!\n");
					break;
				}
			}
		}

		printf("decoded_frame_count = %d\n", decoded_frame_count);
		printf("rendered_frame_count = %d\n", rendered_frame_count);
		printf("total_frame_count = %d\n", Gvar.frame_count);

		if (repeat_times) {
			if (--repeat_times <= 0)
				break;
		}

		printf("repeat %d\n", repeat_times);
	}

	close(Gvar.fd_size);
	close(Gvar.fd);

	if (output_flag)
		close(output_fd);

	printf("playback done\n");
	return 0;
}

int decode_h264_only(void)
{
	int len;
	int fd;
	u32 size;
	char full_filename[256];

	if (filename[0] == '\0')
		strcpy(filename, default_h264_filename);

	len = strlen(filename);
	if (len < 4 || strcmp(filename + len - 4, ".264") != 0) {
		sprintf(full_filename, "%s.264", filename);
		if ((fd = open_file(full_filename, &size))  < 0)
			return -1;
	} else {
		strncpy(full_filename, filename, 255);
		if ((fd = open_file(full_filename, &size)) < 0)
			return -1;
	}

	if (size > bsb_size)
		size = bsb_size;

	if (read_file(fd, bsb_mem, size) < 0)
		return -1;

	if (config_decoder_by_header() < 0)
		return -1;

	if (decode_frame(bsb_mem, bsb_mem + size, 24) < 0)
		return -1;

	printf("decode %s done\n", full_filename);
	return 0;
}

int decode_nv12(void)
{
	int fd;
	u32 size;
	u32 width;
	u32 height;
	u32 count = 0;
	nv12_header_t header;
	nv12_picture_t pic;

	if (filename[0] == '\0')
		strcpy(filename, default_nv12_filename);

	if ((fd = open_file(filename, &size)) < 0)
		return -1;

	if (read_file(fd, &header, sizeof(header)) < 0)
		return -1;

	if (memcmp(header.id, ofile_header.id, 16) || memcmp(header.subtype, ofile_header.subtype, 16)) {
		printf("file not recognized\n");
		return -1;
	}

	if (read_file(fd, &pic, sizeof(pic)) < 0)
		return -1;

	if (config_decoder_by_pic(&pic) < 0)
		return -1;

	width = pic.buffer_width;
	height = pic.buffer_height;
	size = width * height / 2;
	while (1) {
		iav_frame_buffer_t fb;
		iav_frame_buffer_t frame;

		if (ioctl(fd_iav, IAV_IOC_GET_FRAME_BUFFER, &fb) < 0) {
			perror("IAV_IOC_GET_FRAME_BUFFER");
			return -1;
		}

		if (read_file(fd, fb.lu_buf_addr, size * 2) < 0) {
			break;
		}

		if (read_file(fd, fb.ch_buf_addr, size) < 0) {
			break;
		}

		frame.flags = 0;
		frame.fb_id = fb.fb_id;
		frame.pic_width = pic.pic_width;
		frame.pic_height = pic.pic_height;
		frame.lu_off_x = pic.lu_off_x;
		frame.lu_off_y = pic.lu_off_y;
		frame.ch_off_x = pic.ch_off_x;
		frame.ch_off_y = pic.ch_off_y;
		if (ioctl(fd_iav, IAV_IOC_RENDER_FRAME, &frame) < 0) {
			perror("IAV_IOC_RENDER_FRAME");
			return -1;
		}

		printf("picture %d rendered, fb_id = %d\n", ++count, fb.fb_id);

		if (read_file(fd, &pic, sizeof(pic)) < 0) {
			break;
		}

		if (pic.buffer_width != width || pic.buffer_height != height) {
			printf("pic info is wrong\n");
			return -1;
		}
	}

	printf("decode nv12 done\n");
	return 0;
}

int decode_mjpeg(void)
{
	printf("decode_mjpeg not implemented\n");
	return -1;
}

#if 0
int open_vout(int id, int offset_x, int offset_y, int width, int height)
{
	iav_vout_change_video_offset_t offset;
	iav_vout_change_video_size_t size;
	iav_open_vout_t vout;

	memset(&offset, 0, sizeof(offset));
	offset.vout_id = id;
	offset.specified = 1;
	offset.offset_x = offset_x;
	offset.offset_y = offset_y;
	if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET, &offset) < 0) {
		perror("IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET");
		return -1;
	}

	memset(&size, 0, sizeof(size));
	size.vout_id = id;
	size.width = width;
	size.height = height;
	if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_VIDEO_SIZE, &size) < 0) {
		perror("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE");
		return -1;
	}

	memset(&vout, 0, sizeof(vout));
	vout.vout_id = id;
	vout.is_default = 1;
	if (ioctl(fd_iav, IAV_IOC_OPEN_VOUT, &vout) < 0) {
		perror("IAV_IOC_VOUT");
		return -1;
	}
	return 0;
}

int test_swdec(void)
{
#if 0
#define NUM_FB		8
#define PIC_WIDTH	512
#define PIC_HEIGHT	512
#define LU_BYTES	(PIC_WIDTH * PIC_HEIGHT)
#define CH_BYTES	(PIC_WIDTH * PIC_HEIGHT / 2)

	iav_fbp_info_t info;

	printf(" === test_swdec === \n");

	if (map_dsp() < 0)
		return -1;

	if (open_vout(1, 128, 128, PIC_WIDTH, PIC_HEIGHT) < 0)
		return -1;

	info.max_frm_num = NUM_FB;
	info.buffer_width = PIC_WIDTH;
	info.buffer_height = PIC_HEIGHT;
	info.chroma_format = 1;
	info.tile_mode = 0;

	if (ioctl(fd_iav, IAV_IOC_CREATE_FB_POOL, &info) < 0) {
		perror("IAV_IOC_CREATE_FB_POOL");
		return -1;
	}

	printf("fbp created: fbp_id = %d, num_fb = %d\n", info.fbp_id, info.num_fb);

	while (1) {
		int y = 50;
		int uv = 44;

		iav_frame_buffer_t frame;
		memset(&frame, 0, sizeof(frame));

		frame.decoder_id = IAV_SW_DECODER;
		frame.fbp_id = info.fbp_id;

		if (ioctl(fd_iav, IAV_IOC_GET_FRAME_BUFFER, &frame) < 0) {
			perror("IAV_IOC_GET_FRAME_BUFFER");
			return -1;
		}

		memset(frame.lu_buf_addr, y, LU_BYTES);
		memset(frame.ch_buf_addr, uv, CH_BYTES);

		printf("render %d:\n", frame.fb_id);
		frame.flags = IAV_FRAME_SYNC_VOUT;

		if (ioctl(fd_iav, IAV_IOC_RENDER_FRAME, &frame) < 0) {
			perror("IAV_IOC_RENDER_FRAME");
			return -1;
		}

		printf("render %d done\n", frame.fb_id);
	}
#endif
	return 0;
}

iav_udec_info_t G_udec_info;

int udec_create(void)
{
	iav_udec_info_t *info = &G_udec_info;
	memset(info, 0, sizeof(*info));

	printf("create udec %d\n", udec_type);

	info->udec_type = udec_type;
	info->bits_fifo_size = 4*MB;
	info->pjpeg_buffer_size = 4*MB;
	info->mv_fifo_size = 0;

	info->tiled_mode = 0;
	info->frm_chroma_fmt = 1;	// 4:2:0

	info->postp_mode = 1;
	info->postp_chroma_fmt = 2;	// 4:2:2

	if (udec_type == UDEC_JPEG) {
		info->frame_width = 2000;
		info->frame_height = 2000;
		info->postp_width = 1280;
		info->postp_height = 720;
		info->max_frm_num = 6;
	}
	else if (udec_type == UDEC_H264) {
		info->frame_width = 1280;	// need to retrieve from demuxer
		info->frame_height = 720;	// need to retrieve from demuxer
		info->postp_width = 1280;
		info->postp_height = 720;
		info->max_frm_num = 20;
	}
	else {
		info->frame_width = 1920;	// need to retrieve from demuxer
		info->frame_height = 1088;	// need to retrieve from demuxer
		info->postp_width = 1280;
		info->postp_height = 720;
		info->max_frm_num = 6;
	}

	info->max_ppvout_num = 6;

	info->zoom_factor_x = 1;
	info->zoom_factor_y = 1;

	if (ioctl(fd_iav, IAV_IOC_CREATE_UDEC, info) < 0) {
		perror("IAV_IOC_CREATE_UDEC");
		return -1;
	}

	return 0;
}

static int wrap_count = 0;

// for udec
int read_file_to_fifo(int fd, u8 *ptr, u32 size)
{
	u8 *end = G_udec_info.bits_fifo_start + G_udec_info.bits_fifo_size;

	if (ptr + size <= end) {
		if (read_file(fd, ptr, size) < 0)
			return -1;
	}
	else {
		u32 toread = end - ptr;
		if (toread > 0) {
			if (read_file(fd, ptr, toread) < 0)
				return -1;
		}
		toread = size - toread;
		if (toread > 0) {
			if (read_file(fd, G_udec_info.bits_fifo_start, toread) < 0)
				return -1;
		}

		wrap_count++;
		printf("fifo wrap around (%d): %d, %d ====================================\n",
			wrap_count, size - toread, toread);
	}

	return 0;
}

// for udec
void copy_to_fifo(u8 *ptr, const u8 *buffer, u32 size)
{
	u8 *end = G_udec_info.bits_fifo_start + G_udec_info.bits_fifo_size;

	if (ptr + size <= end)
		memcpy(ptr, buffer, size);
	else {
		u32 tocopy = end - ptr;
		if (tocopy > 0) {
			memcpy(ptr, buffer, tocopy);
			buffer += tocopy;
		}
		tocopy = size - tocopy;
		if (tocopy > 0)
			memcpy(G_udec_info.bits_fifo_start, buffer, tocopy);

		wrap_count++;
		printf("EOS wrap around (%d): %d, %d *************************************\n",
			wrap_count, size - tocopy, tocopy);
	}
}

// for udec
void *udec_new_ptr(u8 *ptr, u32 size)
{
	u8 *end = G_udec_info.bits_fifo_start + G_udec_info.bits_fifo_size;

	if ((ptr += size) <= end)
		return ptr;

	return G_udec_info.bits_fifo_start + (ptr - end);
}

// for udec
int udec_append_eos(u8 *ptr)
{
	switch (udec_type) {
	case UDEC_H264: {
			static const u8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
			copy_to_fifo(ptr, eos, sizeof(eos));
			return sizeof(eos);
		}
		break;

	case UDEC_VC1: {
			static const u8 eos[4] = {0, 0, 0x01, 0xA};
			copy_to_fifo(ptr, eos, sizeof(eos));
			return sizeof(eos);
		}
		break;

	case UDEC_MP12: {
			static const u8 eos[4] = {0, 0, 0x01, 0xB7};
			copy_to_fifo(ptr, eos, sizeof(eos));
			return sizeof(eos);
		}
		break;

	case UDEC_MP4H: {
			static const u8 eos[4] = {0, 0, 0x01, 0xB1};
			copy_to_fifo(ptr, eos, sizeof(eos));
			return sizeof(eos);
		}
		break;

	case UDEC_JPEG:
		return 0;

	default:
		printf("not implemented!\n");
		return 0;
	}
}

// for udec
int do_udec_decode(u8 *ptr, u32 size, int num_pics)
{
	iav_udec_decode_t dec;

	memset(&dec, 0, sizeof(dec));
	dec.udec_type = udec_type;
	dec.decoder_id = G_udec_info.decoder_id;
	dec.num_pics = num_pics;
	dec.u.fifo.start_addr = ptr;
	dec.u.fifo.end_addr = udec_new_ptr(ptr, size);

	if (ioctl(fd_iav, IAV_IOC_UDEC_DECODE, &dec) < 0) {
		perror("IAV_IOC_UDEC_DECODE");
		return -1;
	}

	return 0;
}

int test_udec_decode(void)
{
#define MAX_INPUT_FRAMES	20

	int fd;
	u32 fsize;
	u32 fpos = 0;
	u32 size;
	u8 *ptr;

	int times = 0;

	int fd_info;
	u32 info_fsize;
	u32 frames;
	u32 frame_index = 0;
	u32 *frame_sizes = NULL;

	int ninput = 0;

	int flags = IAV_WAIT_BITS_FIFO | IAV_WAIT_OUTPIC;

	if (udec_create() < 0)
		return -1;

	printf("udec id = %d\n", G_udec_info.decoder_id);

//	if (open_vout(1, 128, 128, 352, 288) < 0)
	if (open_vout(1, 0, 0, 1280, 720) < 0)
		return -1;

	ptr = G_udec_info.bits_fifo_start;
	printf("bits_fifo_start = %p\n", ptr);

	// the file
	if ((fd = open_file(filename, &fsize)) < 0)
		return -1;

	printf("open file %s\n", filename);

	// info file
	if (udec_type == UDEC_JPEG)
		frames = 1;
	else {
		char info_filename[256];

		sprintf(info_filename, "%s.info", filename);
		if ((fd_info = open_file(info_filename, &info_fsize)) < 0)
			return -1;

		frames = info_fsize / 4;
		if ((frame_sizes = malloc(frames * sizeof(u32))) == NULL) {
			perror("malloc");
			return -1;
		}

		if (read(fd_info, frame_sizes, frames * 4) < 0) {
			perror(info_filename);
			return -1;
		}

		close(fd_info);

		printf("total frames = %d in %s\n", frames, info_filename);
	}

	while (1) {
		iav_wait_decoder_t wait;
		memset(&wait, 0, sizeof(wait));

		size = (udec_type == UDEC_JPEG) ? fsize : frame_sizes[frame_index];

		wait.flags = flags;
		wait.decoder_id = G_udec_info.decoder_id;
		wait.emptiness.room = size + 16;	// 16 more for EOS
		wait.emptiness.start_addr = ptr;

		//printf("wait_decoder\n");
		if (ioctl(fd_iav, IAV_IOC_WAIT_DECODER, &wait) < 0) {
			if (errno != EAGAIN) {
				perror("IAV_IOC_WAIT_DECODER");
				return -1;
			}
			printf("decoder is stopped\n");
			break;
		}
		//printf("wait_decoder result = %d\n", wait.flags);

		// if input buffer has room, fill it
		if (wait.flags == IAV_WAIT_BITS_FIFO && frame_index < frames) {
			//printf("bits fifo pos: 0x%x\n", (u32)wait.emptiness.end_addr);
			//printf("read file 0x%x, size = 0x%x\n", (u32)ptr, size);
			//printf("=== frame start word: 0x%x 0x%x (%d)\n", *(u32*)ptr, *((u32*)ptr + 1), frame_index);

			printf("[%d] decode %d bytes at 0x%x (%d): %p - %p, wrap = %d\n",
				frame_index + 1, size, fpos, fpos, ptr, ptr + size, wrap_count);

			if (read_file_to_fifo(fd, ptr, size) < 0) {
				return -1;
			}

			if (++ninput >= MAX_INPUT_FRAMES)
				flags &= ~IAV_WAIT_BITS_FIFO;

			//printf("frame start word: 0x%x 0x%x (%d)\n", *(u32*)ptr, *((u32*)ptr + 1), frame_index);

			if (do_udec_decode(ptr, size, 1) < 0)
				return -1;

			fpos += size;
			ptr = udec_new_ptr(ptr, size);

			if (++frame_index == frames) {
				if (--repeat_times == 0) {
					printf("send EOS ====\n");

					size = udec_append_eos(ptr);
					if (size > 0) {
						if (do_udec_decode(ptr, size, 1) < 0)
							return -1;

						ptr = udec_new_ptr(ptr, size);
					}

					flags = IAV_WAIT_OUTPIC;
				}
				else {
					printf("decoding done: %d times\n\n\n", ++times);

					frame_index = 0;
					lseek(fd, 0, SEEK_SET);
				}
			}
		}

		// if there's a decoded buffer
		if (wait.flags == IAV_WAIT_OUTPIC) {
			iav_frame_buffer_t frame;

			//printf("frame received\n");

			--ninput;
			flags |= IAV_WAIT_BITS_FIFO;

			memset(&frame, 0, sizeof(frame));
			frame.flags = output_flag ? IAV_FRAME_NEED_ADDR : 0;
			frame.decoder_id = G_udec_info.decoder_id;

			if (ioctl(fd_iav, IAV_IOC_GET_DECODED_FRAME, &frame) < 0) {
				perror("IAV_IOC_GET_DECODED_FRAME");
				return -1;
			}

			frame.flags |= IAV_FRAME_SYNC_VOUT;
			if (ioctl(fd_iav, IAV_IOC_RENDER_FRAME, &frame) < 0) {
				perror("IAV_IOC_RENDER_FRAME");
				return -1;
			}

			rendered_frame_count++;
			printf("frame rendered %d\n", rendered_frame_count);

			if (udec_type == UDEC_JPEG) {
				printf("JPEG decoding done.\nPress enter key to exit...\n");
				getchar();
				break;
			}

			if (frame.eos_flag) {
				printf("=== EOS received ===\n\tPress enter key to exit...\n");
				getchar();
				break;
			}

			//sleep(2);
		}
	}

	printf("udec done.\n");
	return 0;
}

int test_fb(void)
{
	iav_frame_buffer_t frame;

	if (udec_create() < 0)
		return -1;

	memset(&frame, 0, sizeof(frame));

	frame.decoder_id = G_udec_info.decoder_id;
	frame.fbp_id = 0;

	if (ioctl(fd_iav, IAV_IOC_GET_FRAME_BUFFER, &frame) < 0) {
		perror("IAV_IOC_GET_FRAME_BUFFER");
		return -1;
	}

	printf("fb_id = %d:\n", frame.fb_id);

	if (ioctl(fd_iav, IAV_IOC_RELEASE_FRAME, &frame) < 0) {
		perror("IAV_IOC_RENDER_FRAME");
		return -1;
	}

	printf("test_fb done\n");

	return 0;
}
#endif

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {

		//Vout
		VOUT_INIT_PARAMETERS()
		case 'd':
			disable_output = 1;
			break;

		//Decoding
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'o':
			output_flag = 1;
			strcpy(output_filename, optarg);
			break;
		case 'r':
			repeat_times = atoi(optarg);
			break;
		case APPEND_EOS:
			eos_flag = 1;
			break;

		case SWDEC_TEST:
			swdec_test = 1;
			break;

		case FB_TEST:
			fb_test = 1;
			break;

		case 'h':
			decode_flag = DECODE_H264;
			break;

		case 'H':
			decode_flag = DECODE_H264_ONLY;
			break;

		case 's':
			decode_flag = DECODE_H264_SIMPLE;
			break;
		case 'm':
			decode_flag = DECODE_MJPEG;
			break;
		case 'j':
			decode_flag = DECODE_JPEG;
			break;
		case 'n':
			decode_flag = DECODE_RAW_NV12;
			break;
		case 'u':
			decode_flag = DECODE_UDEC;
			udec_type = get_udec_type(optarg);
			if (udec_type < 0) {
				printf("unknown decoder type '%s'\n", optarg);
				return -1;
			}
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

void usage(void)
{
	int i;

	printf("test3 usage:\n");
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

int open_iav(void)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	return fd_iav;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return -1;
	}

	// open the device
	if (open_iav() < 0)
		return -1;

	if (init_param(argc, argv) < 0)
		return -1;

	if (decode_flag != DECODE_NOTHING && decode_flag != DECODE_UDEC)
		if (map_bsb() < 0)
			return -1;

	//Dynamically change vout
	if (dynamically_change_vout())
		return 0;

	//check vout
	if (check_vout() < 0)
		return -1;

	//Init vout
	if (vout_flag[VOUT_0] && init_vout(VOUT_0, 0) < 0)
		return -1;
	if (vout_flag[VOUT_1] && init_vout(VOUT_1, 0) < 0)
		return -1;

	if (select_channel() < 0)
		return -1;

	if (decode_flag != DECODE_NOTHING && decode_flag != DECODE_UDEC)
		if (enter_decoding() < 0)
			return -1;

	if (swdec_test) {
//		test_swdec();
		return 0;
	}

	if (fb_test) {
//		test_fb();
		return 0;
	}

	switch (decode_flag) {
	case DECODE_JPEG:
		if (decode_jpeg() < 0)
			return -1;
		break;

	case DECODE_H264:
	case DECODE_H264_SIMPLE:
		if (decode_h264() < 0)
			return -1;
		break;

	case DECODE_H264_ONLY:
		if (decode_h264_only() < 0)
			return -1;
		break;

	case DECODE_RAW_NV12:
		if (decode_nv12() < 0)
			return -1;
		break;

	case DECODE_MJPEG:
		if (decode_mjpeg() < 0)
			return -1;
		break;

	case DECODE_UDEC:
		switch (udec_type) {
		case UDEC_H264:
		case UDEC_VC1:
		case UDEC_MP12:
		case UDEC_MP4H:
		case UDEC_JPEG:
//			if (test_udec_decode() < 0)
//				return -1;
			break;
		}
		break;

	default:
		printf("unknown decoder type!\n");
		break;
	}

	return 0;
}


