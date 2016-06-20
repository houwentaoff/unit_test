/*
 * test_overlay.c
 *
 * History:
 * 2010/3/2 - [Louis Sun] created for A5s
 * 2010/7/28 - [Jian Tang] add 3 areas for each stream
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
#include <signal.h>

#define MAX_ENCODE_STREAM_NUM	(4)
#define MAX_OVERLAY_AREA_NUM	(3)
#define OVERLAY_CLUT_NUM		(16)
#define OVERLAY_CLUT_SIZE		(1024)
#define OVERLAY_CLUT_OFFSET		(0)
#define OVERLAY_YUV_OFFSET		(OVERLAY_CLUT_NUM * OVERLAY_CLUT_SIZE)

#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align)	((size) & ~((align) - 1))
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align)	(((size) + ((align) - 1)) & ~((align) - 1))
#endif

#define VERIFY_STREAMID(x)	do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf("stream id wrong %d \n", (x));			\
				return -1; 	\
			}	\
		} while(0)

#define VERIFY_AREAID(x)	do {		\
			if (((x) < 0) || ((x) >= MAX_OVERLAY_AREA_NUM)) {	\
				printf("area id wrong %d, not in range [0~2].\n", (x));	\
				return -1;	\
			}	\
		} while (0)

typedef struct osd_clut_s {
	u8 v;
	u8 u;
	u8 y;
	u8 alpha;
} osd_clut_t;

typedef struct osd_info_s {
	int enable;
	u16 width;
	u16 height;
	u16 x;
	u16 y;
	int bitmap;
	int update;
	char clut_file[256];
	char data_file[256];
} osd_info_t;

#define	ROTATE_BIT		(0)
#define	HFLIP_BIT		(1)
#define	VFLIP_BIT		(2)
#define	SET_BIT(x)		(1 << (x))

typedef enum rotate_type_s {
	CLOCKWISE_0 = 0,
	CLOCKWISE_90 = SET_BIT(ROTATE_BIT),
	CLOCKWISE_180 = SET_BIT(HFLIP_BIT) | SET_BIT(VFLIP_BIT),
	CLOCKWISE_270 = SET_BIT(HFLIP_BIT) | SET_BIT(VFLIP_BIT) | SET_BIT(ROTATE_BIT),
	AUTO_ROTATE,
} rotate_type_t;

typedef enum osd_status_s {
	INIT = 0,
	OSD_ENABLE,
	OSD_DISABLE,
} osd_status_t;

typedef struct stream_info_s {
	int enable;
	rotate_type_t rotate;
	int win_width;
	int win_height;
	osd_info_t osd[MAX_OVERLAY_AREA_NUM];
} stream_info_t;

static char	VIN_IDSP[] = "/proc/ambarella/vin0_vsync";

static int autorun_flag = 0;
static int show_config_stream_id = 0;
static int show_config_flag = 0;
static int verbose_flag = 0;
static int update_flag = 0;
static stream_info_t stream_info[MAX_ENCODE_STREAM_NUM];
static overlay_insert_ex_t overlay_insert[MAX_ENCODE_STREAM_NUM];

int fd_overlay;
int overlay_yuv_size;
u8 *overlay_yuv_addr;
u8 *overlay_clut_addr;

static unsigned short y_table[256] = {
	5, 191, 0, 191, 0, 191, 0, 192, 128, 255, 0, 255, 0, 255, 0, 255,
	0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
	204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
	102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
	0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
	204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
	102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
	0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
	204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
	102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
	0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
	204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
	102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
	0, 51, 102, 153, 204, 255, 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
	204, 255, 0, 51, 102, 153, 204, 255, 0, 17, 34, 51, 68, 85, 102, 119,
	136, 153, 170, 187, 204, 221, 238, 255, 0, 0, 0, 0, 0, 0, 204, 242,
};

static unsigned short u_table[256] = {
	4, 0, 191, 191, 0, 0, 191, 192, 128, 0, 255, 255, 0, 0, 255, 255,
	0, 0, 0, 0, 0, 0, 51, 51, 51, 51, 51, 51, 102, 102, 102, 102,
	102, 102, 153, 153, 153, 153, 153, 153, 204, 204, 204, 204, 204, 204, 255, 255,
	255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 51, 51, 51, 51, 51, 51,
	102, 102, 102, 102, 102, 102, 153, 153, 153, 153, 153, 153, 204, 204, 204, 204,
	204, 204, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 51, 51,
	51, 51, 51, 51, 102, 102, 102, 102, 102, 102, 153, 153, 153, 153, 153, 153,
	204, 204, 204, 204, 204, 204, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0,
	0, 0, 51, 51, 51, 51, 51, 51, 102, 102, 102, 102, 102, 102, 153, 153,
	153, 153, 153, 153, 204, 204, 204, 204, 204, 204, 255, 255, 255, 255, 255, 255,
	0, 0, 0, 0, 0, 0, 51, 51, 51, 51, 51, 51, 102, 102, 102, 102,
	102, 102, 153, 153, 153, 153, 153, 153, 204, 204, 204, 204, 204, 204, 255, 255,
	255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 51, 51, 51, 51, 51, 51,
	102, 102, 102, 102, 102, 102, 153, 153, 153, 153, 153, 153, 204, 204, 204, 204,
	204, 204, 255, 255, 255, 255, 255, 255, 0, 17, 34, 51, 68, 85, 102, 119,
	136, 153, 170, 187, 204, 221, 238, 255, 0, 0, 0, 0, 0, 0, 0, 102,
};

static unsigned short v_table[256] = {
	3, 0, 0, 0, 191, 191, 191, 192, 128, 0, 0, 0, 255, 255, 255, 255,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
	51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
	51, 51, 51, 51, 51, 51, 51, 51, 102, 102, 102, 102, 102, 102, 102, 102,
	102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
	102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 153, 153, 153, 153,
	153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
	153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
	204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204,
	204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204, 204,
	204, 204, 204, 204, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 0, 17, 34, 51, 68, 85, 102, 119,
	136, 153, 170, 187, 204, 221, 238, 255, 0, 0, 0, 0, 0, 0, 0, 34,
};

static int open_file(const char *name, u32 *size)
{
	int fd;
	struct stat stat;

	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		perror(name);
		return -1;
	}

	if (fstat(fd, &stat) < 0) {
		perror("fstat");
		close(fd);
		return -1;
	}

	if (size != NULL)
		*size = stat.st_size;

	return fd;
}

static int read_file(const char *filename, u8 *buffer, u32 buffer_len)
{
	int fd;
	u32 size;

	if ((fd = open_file(filename, &size)) < 0)
		return -1;

	if (size > buffer_len) {
		printf("file too large\n");
		close(fd);
		return -1;
	}

	if (read(fd, buffer, size) != size) {
		perror(filename);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static inline int is_valid_rotate(rotate_type_t rotate)
{
	switch (rotate) {
	case CLOCKWISE_0:
	case CLOCKWISE_90:
	case CLOCKWISE_180:
	case CLOCKWISE_270:
	case AUTO_ROTATE:
		return 1;
	default:
		return 0;
	}
}

static int check_encode_status(void)
{
	int i;
	iav_state_info_t iav_info;
	iav_encode_format_ex_t format;
	iav_h264_config_ex_t config_h264;
	iav_jpeg_config_ex_t config_jpeg;
	u8 rotate_clockwise, vflip, hflip;

	/* IAV must be in ENOCDE or PREVIEW state */
	if (ioctl(fd_overlay, IAV_IOC_GET_STATE_INFO, &iav_info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return -1;
	}

	if ((iav_info.state != IAV_STATE_PREVIEW) &&
			(iav_info.state != IAV_STATE_ENCODING)) {
		printf("IAV must be in PREVIEW or ENCODE for text OSD.\n");
		return -1;
	}

	/* Check rotate status */
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (stream_info[i].enable == OSD_ENABLE) {
			format.id = (1 << i);
			if (ioctl(fd_overlay, IAV_IOC_GET_ENCODE_FORMAT_EX, &format) < 0) {
				perror("IAV_IOC_GET_ENCODE_FORMAT_EX");
				return -1;
			}

			if (stream_info[i].rotate == AUTO_ROTATE) {
				switch (format.encode_type) {
				case IAV_ENCODE_H264:
					config_h264.id = (1 << i);
					if (ioctl(fd_overlay, IAV_IOC_GET_H264_CONFIG_EX,
							&config_h264) < 0) {
						perror("IAV_IOC_GET_H264_CONFIG_EX");
						return -1;
					}
					rotate_clockwise = config_h264.rotate_clockwise;
					hflip = config_h264.hflip;
					vflip = config_h264.vflip;
					break;
				case IAV_ENCODE_MJPEG:
					config_jpeg.id = (1 << i);
					if (ioctl(fd_overlay, IAV_IOC_GET_JPEG_CONFIG_EX,
							&config_jpeg) < 0) {
						perror("IAV_IOC_GET_JPEG_CONFIG_EX");
						return -1;
					}
					rotate_clockwise = config_jpeg.rotate_clockwise;
					hflip = config_jpeg.hflip;
					vflip = config_jpeg.vflip;
					break;
				default:
					rotate_clockwise = 0;
					vflip = 0;
					hflip = 0;
					printf("!Warning: Stream %c Unknown encode type. OSD is "
							"consistent with VIN orientation.\n", 'A' + i);
					break;
				}
				stream_info[i].rotate = CLOCKWISE_0;
				stream_info[i].rotate |= (rotate_clockwise ?
						SET_BIT(ROTATE_BIT) : 0);
				stream_info[i].rotate |= (hflip ? SET_BIT(HFLIP_BIT) : 0);
				stream_info[i].rotate |= (vflip ? SET_BIT(VFLIP_BIT) : 0);
				if (!is_valid_rotate(stream_info[i].rotate)) {
					printf("!Warning: Stream %c Unknown rotate type. OSD is "
							"consistent with VIN orientation.\n", 'A' + i);
					stream_info[i].rotate = CLOCKWISE_0;
				}
			}

			if (stream_info[i].rotate & SET_BIT(ROTATE_BIT)) {
				stream_info[i].win_width = format.encode_height;
				stream_info[i].win_height = format.encode_width;
			} else {
				stream_info[i].win_width = format.encode_width;
				stream_info[i].win_height = format.encode_height;
			}
		}
	}
	return 0;
}

static int map_overlay(void)
{
	iav_mmap_info_t overlay_info;
	if (ioctl(fd_overlay, IAV_IOC_MAP_OVERLAY, &overlay_info) < 0) {
		perror("IAV_IOC_MAP_OVERLAY");
		return -1;
	}
	printf("overlay: start = 0x%p, size = 0x%x\n", overlay_info.addr, overlay_info.length);
	//split into MAX_ENCODE_STREAM parts
	overlay_clut_addr = overlay_info.addr + OVERLAY_CLUT_OFFSET;
	overlay_yuv_addr = overlay_info.addr + OVERLAY_YUV_OFFSET;
	overlay_yuv_size = (overlay_info.length - OVERLAY_YUV_OFFSET) /
		MAX_ENCODE_STREAM_NUM;

	return 0;
}

static int prepare_overlay_config(void)
{
	int i, j, win_width, win_height, total_size;
	osd_info_t *osd;
	overlay_insert_area_ex_t *area;
	u8 *overlay_data;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (stream_info[i].enable == OSD_ENABLE) {
			win_width = stream_info[i].win_width;
			win_height = stream_info[i].win_height;
			for (j = 0, total_size = 0; j < MAX_OVERLAY_AREA_NUM; j++) {
				osd = &stream_info[i].osd[j];
				area = &overlay_insert[i].area[j];
				overlay_data = overlay_yuv_addr + i * overlay_yuv_size;
				if (osd->enable) {
					osd->x = (osd->x < 0 ? 0 : osd->x);
					osd->y = (osd->y < 0 ? 0 : osd->y);
					if (osd->width <= 0|| osd->height <= 0) {
						printf("!!!Error: Stream %c Area %d width[%d] or height"
								"[%d] shall be positive.\n", 'A' + i, j,
								osd->width, osd->height);
						return -1;
					}
					if (stream_info[i].rotate & SET_BIT(ROTATE_BIT)) {
						area->width = osd->height = ROUND_DOWN(osd->height, 32);
						area->height = osd->width = ROUND_DOWN(osd->width, 4);
					} else {
						area->width = osd->width = ROUND_DOWN(osd->width, 32);
						area->height = osd->height = ROUND_DOWN(osd->height, 4);
					}
					switch (stream_info[i].rotate) {
					case CLOCKWISE_0:
						area->start_x = osd->x = ROUND_DOWN(osd->x, 2);
						area->start_y = osd->y = ROUND_DOWN(osd->y, 4);
						break;
					case CLOCKWISE_90:
						area->start_x = osd->y = ROUND_DOWN(osd->y, 2);
						area->start_y = ROUND_DOWN(
								win_width - osd->x - osd->width, 4);
						osd->x = win_width - osd->width - area->start_y;
						break;
					case CLOCKWISE_180:
						area->start_x = ROUND_DOWN(
								win_width - osd->x - osd->width, 2);
						area->start_y = ROUND_DOWN(
								win_height - osd->y - osd->height, 4);
						osd->x = win_width - osd->width - area->start_x;
						osd->y = win_height - osd->height -  area->start_y;
						break;
					case CLOCKWISE_270:
						area->start_x = ROUND_DOWN(
								win_height - osd->y - osd->height, 2);
						area->start_y = osd->x = ROUND_DOWN(osd->x, 4);
						osd->y = win_height - osd->height -  area->start_x;
						break;
					default:
						printf("unknown rotate type\n");
						return -1;
					}

					area->pitch = area->width;
					area->enable = 1;
					area->total_size = area->pitch * area->height;
					area->clut_id = MAX_OVERLAY_AREA_NUM * i + j;
					area->data = overlay_data + total_size;
					if (!area->width || !area->height) {
						printf("The area width / area height cannot be smaller than 32 / 4, respectively.\n");
						return -1;
					}
					total_size += area->total_size;
					if (total_size > overlay_yuv_size) {
						printf("The total OSD size is %d (should be <= %d).\n",
								total_size, overlay_yuv_size);
						return -1;
					}
					printf("stream %c Area [%d]:\n", i + 'A', j);
					printf("\twidth = %d\n", area->width);
					printf("\theight = %d\n", area->height);
					printf("\tpitch = %d\n", area->pitch);
					printf("\tarea size = %d\n", area->total_size);
					printf("\ttotal size = %d\n", total_size);
				}
			}
		}
	}

	return 0;
}

static int fill_overlay_clut(int stream_id, int area_id, u8 clut_id)
{
	int i, color_count = 256;
	osd_clut_t *clut_data = (osd_clut_t *)(overlay_clut_addr +
			OVERLAY_CLUT_SIZE * clut_id);
	if (!autorun_flag && stream_info[stream_id].osd[area_id].bitmap) {
		memset(clut_data, 0, color_count);
		read_file(stream_info[stream_id].osd[area_id].clut_file, (u8 *)clut_data,
		          color_count  * sizeof(osd_clut_t) / sizeof(u8));
		return 0;
	}

	/* The total number of clut is OVERLAY_CLUT_NUM. */
	if (clut_id >= OVERLAY_CLUT_NUM) {
		printf("Invalid clut_id %d, should be in range of [0,  %d).\n",
			clut_id, OVERLAY_CLUT_NUM);
		return -1;
	}
	for (i = 0; i < OVERLAY_CLUT_NUM; i++) {
		clut_data[i].y = y_table[clut_id * OVERLAY_CLUT_NUM + i];
		clut_data[i].u = u_table[clut_id * OVERLAY_CLUT_NUM + i];
		clut_data[i].v = v_table[clut_id * OVERLAY_CLUT_NUM + i];
		clut_data[i].alpha = 255;
	}

	return 0;
}

/* Sample
 * If loading bitmap, fill with data from bitmap file
 * Or fill with solid color slice.
 */
static int get_overlay_content(int stream_id, int area_id, u8 *content)
{
	static int count = 0;
	int i, n, slice_size;

	if (!autorun_flag && stream_info[stream_id].osd[area_id].bitmap) {
		memset(content, 0,
			overlay_insert[stream_id].area[area_id].total_size);
		read_file(stream_info[stream_id].osd[area_id].data_file, content,
		    overlay_insert[stream_id].area[area_id].total_size);
		return 0;
	}

	n = 8;
	slice_size = overlay_insert[stream_id].area[area_id].total_size / n;
	for (i = 0; i < n; i++) {
		memset(content + i * slice_size, (i + count) % n, slice_size);
	}
	++count;
	return 0;
}

/* Fill the overlay data based on rotate degree */
static int fill_overlay_data(int stream_id, int area_id, u8 *content)
{
	int row, col, area_pitch, area_height;
	u8 *dst, *src;

	dst = overlay_insert[stream_id].area[area_id].data;
	area_pitch = overlay_insert[stream_id].area[area_id].pitch;
	area_height = overlay_insert[stream_id].area[area_id].height;
	switch (stream_info[stream_id].rotate) {
	case CLOCKWISE_0:
		src = content;
		memcpy(dst, src, area_pitch * area_height);
		break;
	case CLOCKWISE_90:
		for (row = 0; row < area_height; row++) {
			src = content + area_height - 1 - row;
			for (col = 0; col < area_pitch; col++) {
				*dst = *src;
				dst++;
				src += area_height;
			}
		}
		break;
	case CLOCKWISE_180:
		for (row = 0; row < area_height; row++) {
			src = content + area_pitch * area_height - 1 - row * area_pitch;
			for (col = 0; col < area_pitch; col++) {
				*dst = *src;
				dst++;
				src--;
			}
		}
		break;
	case CLOCKWISE_270:
		for (row = 0; row < area_height; row++) {
			src = content + area_height * (area_pitch - 1) + row;
			for (col = 0; col < area_pitch; col++) {
				*dst = *src;
				dst++;
				src -= area_height;
			}
		}
		break;
	default:
		printf("Unknown rotate type.");
		return -1;
	}
	return 0;
}

static int set_overlay(int stream_id)
{
	int i;
	u8 *content;
	overlay_insert_area_ex_t *area;

	switch (stream_info[stream_id].enable) {
	case OSD_DISABLE:
		overlay_insert[stream_id].enable = 0;
		break;
	case OSD_ENABLE:
		overlay_insert[stream_id].enable = 1;
		for (i = 0; i < MAX_OVERLAY_AREA_NUM; i++) {
			if (overlay_insert[stream_id].area[i].enable) {
				area = &overlay_insert[stream_id].area[i];
				fill_overlay_clut(stream_id, i, area->clut_id);
				if (NULL == (content = (u8 *)malloc(area->total_size))) {
					perror("set_overlay: malloc");
					return -1;
				}
				get_overlay_content(stream_id, i, content);
				fill_overlay_data(stream_id, i, content);
				free(content);
			}
		}
		break;
	default:
		return 0;
	}

	overlay_insert[stream_id].id = (1 << stream_id);

	if (ioctl(fd_overlay, IAV_IOC_OVERLAY_INSERT_EX, &overlay_insert[stream_id]) < 0) {
		perror("IAV_IOC_OVERLAY_INSERT_EX");
		return -1;
	}

	return 0;
}

static int autorun_overlay(void)
{
	int i, j, n, x, y, dx, dy, total_times, tmp, total_size, vin_fd;
	u8 *data, *overlay_data;
	char vin_int_arr[8];
	static int srcbuf[MAX_ENCODE_STREAM_NUM];
	iav_encode_stream_info_ex_t encode_stream;
	iav_encode_format_ex_t encode_format;
	overlay_insert_area_ex_t *area;

	static struct timeval tm1, tm2;
	struct timeval *p_tm[2];
	int cur_tm = 0;
	p_tm[0] = &tm1;
	p_tm[1] = &tm2;
	if (verbose_flag)
		gettimeofday(&tm2, NULL);

	if ((vin_fd = open(VIN_IDSP, O_RDONLY)) < 0) {
		printf("CANNOT open [%s].\n", VIN_IDSP);
		return -1;
	}

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		memset(&encode_stream, 0, sizeof(encode_stream));
		memset(&encode_format, 0, sizeof(encode_format));
		encode_stream.id = (1 << i);
		encode_format.id = (1 << i);
		if (ioctl(fd_overlay, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &encode_stream) < 0) {
			perror("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
			return -1;
		}
		if (ioctl(fd_overlay, IAV_IOC_GET_ENCODE_FORMAT_EX, &encode_format) < 0) {
			perror("IAV_IOC_GET_ENCODE_FORMAT_EX");
			return -1;
		}
		if (encode_stream.state == IAV_STREAM_STATE_ENCODING &&
				srcbuf[encode_format.source] == 0) {
			srcbuf[encode_format.source] = 1;
			stream_info[i].enable = OSD_ENABLE;
			stream_info[i].win_width = encode_format.encode_width;
			stream_info[i].win_height = encode_format.encode_height;
		}
	}


	total_times = 30;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		overlay_data = overlay_yuv_addr + overlay_yuv_size * i;
		overlay_insert[i].id = (1 << i);
		if (stream_info[i].enable == OSD_ENABLE) {
			overlay_insert[i].enable = 1;
			total_size = 0;
			for (j = 0; j < MAX_OVERLAY_AREA_NUM; ++j) {
				area = &overlay_insert[i].area[j];
				area->enable = 1;
				tmp = ROUND_DOWN((stream_info[i].win_width - 48) /
					MAX_OVERLAY_AREA_NUM, 32);
				area->width = (tmp < 64 ? tmp : 64);
				area->height = (stream_info[i].win_height > 240 ? 128 : 32);
				area->pitch = (tmp < 64 ? tmp: 64);
				area->total_size = area->pitch * area->height;
				data = overlay_data + total_size;
				total_size += area->total_size;
				area->clut_id = MAX_OVERLAY_AREA_NUM * i + j;
				fill_overlay_clut(i, j, area->clut_id);
				get_overlay_content(i, j, data);
				area->data = data;
			}
		}
	}

	while (1) {
		for (n = 0, x = 16, y = 0, dy = 4; n < total_times; ++n) {
			y += dy;
			for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
				if (overlay_insert[i].enable) {
					dx = ROUND_DOWN(stream_info[i].win_width /
						MAX_OVERLAY_AREA_NUM, 2);
					for (j = 0; j < MAX_OVERLAY_AREA_NUM; j++ ) {
						overlay_insert[i].area[j].start_x = x + dx * j;
						overlay_insert[i].area[j].start_y = y;
					}
				}
			}
			read(vin_fd, vin_int_arr, 8);

			if (verbose_flag) {
				cur_tm = !cur_tm;
				gettimeofday(p_tm[cur_tm], NULL);
				printf("interval %ldus\n",(1000000 * (p_tm[cur_tm]->tv_sec -
					p_tm[!cur_tm]->tv_sec) + (p_tm[cur_tm]->tv_usec -
					p_tm[!cur_tm]->tv_usec)));
			}

			for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
				if (overlay_insert[i].enable) {
					if (y + overlay_insert[i].area[0].height >
							stream_info[i].win_height)
						continue;
					if (ioctl(fd_overlay, IAV_IOC_OVERLAY_INSERT_EX,
							&overlay_insert[i]) < 0) {
						perror("IAV_IOC_OVERLAY_INSERT_EX");
						return -1;
					}
				}
			}
		}
	}

	return 0;
}

#define NO_ARG	  0
#define HAS_ARG	 1
static struct option long_options[] = {
	{"streamA", NO_ARG, 0, 'A'},   // -A xxxxx	means all following configs will be applied to stream A
	{"streamB", NO_ARG, 0, 'B'},
	{"streamC", NO_ARG, 0, 'C'},
	{"streamD", NO_ARG, 0, 'D'},
	{"area", HAS_ARG, 0, 'a'},
	{"xstart", HAS_ARG, 0, 'x'},
	{"ystart", HAS_ARG, 0, 'y'},
	{"width", HAS_ARG, 0, 'w'},
	{"height", HAS_ARG, 0, 'h'},
	{"update", NO_ARG, 0, 'u'},

	{"clut_file", HAS_ARG, 0, 'c'},
	{"data_file", HAS_ARG, 0, 'd'},

	{"norotate", NO_ARG, 0, 'n'},
	{"disable", NO_ARG, 0, 's'},

	{"autorun", NO_ARG, 0, 'r'},
	{"show-config", NO_ARG, 0, 'l'},
	{"verbose", NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const char *short_options = "ABCDa:x:y:w:h:uc:d:nsrlv";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tset stream A"},
	{"", "\t\tset stream B"},
	{"", "\t\tset stream C"},
	{"", "\t\tset stream D"},

	{"0~2", "\t\tset overlay area num"},
	{"", "\t\tset x"},
	{"", "\t\tset y"},
	{"", "\t\tset width (must be larger than 32)"},
	{"", "\t\tset height"},
	{"", "\t\tupdate overlay data every second."},

	{"filename", "read clut from file"},
	{"filename", "read yuv index from file"},

	{"", "\t\tWhen rotate clockwise degree is 90/180/270, OSD is consistent with stream orientation."},
	{"", "\t\tturn off overlay on stream"},

	{"", "\t\tupdate overlay info every frame automatically"},
	{"", "\tshow current overlay area settings"},
	{"", "\t\tprint debug message"},

};

void usage(void)
{
	int i;

	printf("test_overlay usage:\n");
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
	printf("\nExample:\n"
			"  Three rectangle OSD on one stream:\n"
			"    test_overlay -A -a0 -x0 -y0 -w128 -h64 -a1 -x200 -y100 -w64 -h128 -a2 -x0 -y300 -w48 -h96\n"
			"  Bitmap OSD:\n"
			"    bmp_convert\n"
			"    test_overlay -A -a0 -x20 -y40 -w256 -h128 -c bmp_clut.dat -d bmp_data.dat\n"
			"  Keep OSD upright on 270 degree clockwise stream:\n"
			"    test_encode -A --smaxsize 720x1280 -h720p --rotate 1 --hflip 1 --vflip 1 -e\n"
			"    test_overlay -A -a0 -x100 -y200 -w100 -h200 --norotate\n"
			"  Update OSD data by rewriting osd buffer:\n"
			"    test_overlay -A -a0 -x0 -y100 -w64 -h128 -u\n"
			"  Updata OSD every frame:\n"
			"    test_overlay -r\n"
			"  Show current overlay area settings of streams:\n"
			"    test_overlay -A -l\n");
	printf("\n");
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int current_stream = -1;		// -1 is a invalid stream, for initialize data only
	int current_area = -1;		// -1 is a invalid area, for initialize data only

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'A':
			current_stream = 0;
			stream_info[current_stream].enable = OSD_ENABLE;
			break;
		case 'B':
			current_stream = 1;
			stream_info[current_stream].enable = OSD_ENABLE;
			break;
		case 'C':
			current_stream = 2;
			stream_info[current_stream].enable = OSD_ENABLE;
			break;
		case 'D':
			current_stream = 3;
			stream_info[current_stream].enable = OSD_ENABLE;
			break;
		case 'a':
			current_area = atoi(optarg);
			VERIFY_STREAMID(current_stream);
			VERIFY_AREAID(current_area);
			stream_info[current_stream].osd[current_area].enable = 1;
			break;
		case 'x':
			VERIFY_STREAMID(current_stream);
			VERIFY_AREAID(current_area);
			stream_info[current_stream].osd[current_area].x = atoi(optarg);
			break;
		case 'y':
			VERIFY_STREAMID(current_stream);
			VERIFY_AREAID(current_area);
			stream_info[current_stream].osd[current_area].y = atoi(optarg);
			break;
		case 'w':
			VERIFY_STREAMID(current_stream);
			VERIFY_AREAID(current_area);
			stream_info[current_stream].osd[current_area].width = atoi(optarg);
			break;
		case 'h':
			VERIFY_STREAMID(current_stream);
			VERIFY_AREAID(current_area);
			stream_info[current_stream].osd[current_area].height = atoi(optarg);
			break;
		case 'u':
			VERIFY_STREAMID(current_stream);
			VERIFY_AREAID(current_area);
			stream_info[current_stream].osd[current_area].update = 1;
			update_flag = 1;
			break;
		case 'c':
			VERIFY_STREAMID(current_stream);
			VERIFY_AREAID(current_area);
			strcpy(stream_info[current_stream].osd[current_area].clut_file,
			       optarg);
			stream_info[current_stream].osd[current_area].bitmap = 1;
			break;
		case 'd':
			VERIFY_STREAMID(current_stream);
			VERIFY_AREAID(current_area);
			strcpy(stream_info[current_stream].osd[current_area].data_file,
			       optarg);
			stream_info[current_stream].osd[current_area].bitmap = 1;
			break;
		case 'n':
			VERIFY_STREAMID(current_stream);
			stream_info[current_stream].rotate = AUTO_ROTATE;
			break;
		case 's':
			VERIFY_STREAMID(current_stream);
			stream_info[current_stream].enable = OSD_DISABLE;
			break;
		case 'r':
			autorun_flag = 1;
			break;
		case 'l':
			show_config_stream_id |= (1 << current_stream);
			show_config_flag = 1;
			break;
		case 'v':
			verbose_flag = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

static int update_data(int stream_id)
{
	int i;
	u8 *data;
	for (i = 0; i < MAX_OVERLAY_AREA_NUM; i++) {
		if (!stream_info[stream_id].osd[i].bitmap &&
				stream_info[stream_id].osd[i].update &&
				overlay_insert[stream_id].area[i].enable) {
			data = (u8 *)malloc(overlay_insert[stream_id].area[i].total_size);
			if (NULL == data) {
				perror("update_data: malloc");
				return -1;
			}
			get_overlay_content(stream_id, i, data);
			fill_overlay_data(stream_id, i, data);
			free(data);
		}
	}
	return 0;
}

static int show_overlay_config(int stream_id)
{
	int i, j, total_size;
	overlay_insert_area_ex_t * area = NULL;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (stream_id & (1 << i)) {
			overlay_insert[i].id = (1 << i);
			if (ioctl(fd_overlay, IAV_IOC_GET_OVERLAY_INSERT_EX, &overlay_insert[i]) < 0) {
				perror("IAV_IOC_GET_OVERLAY_INSERT_EX");
				return -1;
			}
			printf("\n  Stream %c overlay configuration :\n", 'A' + i);
			total_size = 0;
			for (j = 0; j < MAX_OVERLAY_AREA_NUM; ++j) {
				area = &overlay_insert[i].area[j];
				printf("    Area [%d] : size [%dx%d], offset [%dx%d], pitch [%d].\n",
					j, area->width, area->height, area->start_x,
					area->start_y, area->pitch);
				total_size += area->total_size;
			}
			printf("  Total Size : [%d].\n", total_size);
		}
	}
	return 0;
}

static void sigstop()
{
	int i;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (stream_info[i].enable == OSD_ENABLE) {
			overlay_insert[i].enable = 0;
			overlay_insert[i].id = (1 << i);
			if (ioctl(fd_overlay, IAV_IOC_OVERLAY_INSERT_EX, &overlay_insert[i]) < 0) {
				perror("IAV_IOC_OVERLAY_INSERT_EX");
			}
		}
	}
	exit(0);
}

int main(int argc, char **argv)
{
	int i;
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_overlay = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (check_encode_status() < 0)
		return -1;

	if (map_overlay() < 0)
		return -1;

	if (show_config_flag) {
		show_overlay_config(show_config_stream_id);
	} else if (autorun_flag) {
		autorun_overlay();
	} else {
		if (prepare_overlay_config() < 0)
			return -1;

		for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
			if (stream_info[i].enable != INIT) {
				if (set_overlay(i) < 0)
					return -1;
			}
		}

		while (update_flag) {
			sleep(1);
			for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
				if (stream_info[i].enable == OSD_ENABLE)
					update_data(i);
			}
		}
	}
	return 0;
}

