/*
 * test_fb.c
 *
 * History:
 *	2009/10/13 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <getopt.h>
#include <ctype.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/un.h>

#define	NO_ARG		0
#define	HAS_ARG		1


static struct option long_options[] = {
	{"clut-id",	HAS_ARG,	0,	'c'},
	{"height",	HAS_ARG,	0,	'h'},
	{"width",		HAS_ARG,	0,	'w'},

	{0, 0, 0, 0}
};

static const char *short_options = "c:h:w:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0~255",	"\tset clut id"},
	{"",			"\t\tset frame buffer height"},
	{"",			"\t\tset frame buffer width"},
};


struct fb_cmap_user {
	unsigned int start;			/* First entry	*/
	unsigned int len;			/* Number of entries */
	unsigned short *y;		/* Red values	*/
	unsigned short *u;
	unsigned short *v;
	unsigned short *transp;		/* transparency, can be NULL */
};


static unsigned short y_table[256] =
{
	  5,
	191,
	  0,
	191,
	  0,
	191,
	  0,
	192,
	128,
	255,
	  0,
	255,
	  0,
	255,
	  0,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	204,
	242,
};


static unsigned short u_table[256] =
{
	  4,
	  0,
	191,
	191,
	  0,
	  0,
	191,
	192,
	128,
	  0,
	255,
	255,
	  0,
	  0,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	102,
};


static unsigned short v_table[256] =
{
	  3,
	  0,
	  0,
	  0,
	191,
	191,
	191,
	192,
	128,
	  0,
	  0,
	  0,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 34,
};

static unsigned short blend_table[256] =
{
		0,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
};


static int fb_width = 0;
static int fb_width_flag = 0;
static int fb_height = 0;
static int fb_height_flag = 0;
static int fb_clut_id = 0;
static int fb_clut_id_flag = 0;

void usage(void)
{
	int i;

	printf("test_fb usage:\n");
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
	printf("Example:\n    test_fb -c 2\n\n");
}

int init_param(int argc, char ** argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'c':
			fb_clut_id = atoi(optarg);
			fb_clut_id_flag = 1;
			break;

		case 'h':
			fb_height = atoi(optarg);
			fb_height_flag = 1;
			break;

		case 'w':
			fb_width = atoi(optarg);
			fb_width_flag = 1;
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
	int				fb;
	struct fb_var_screeninfo	var;
	struct fb_fix_screeninfo	fix;
	unsigned char			*buf;
	struct fb_cmap_user	cmap;
	int color, width, height;

	if (argc < 2) {
		usage();
		return 0;
	}

	fb = open("/dev/fb0", O_RDWR);
	if (fb < 0) {
		perror("Open fb error");
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if(ioctl(fb, FBIOGET_VSCREENINFO, &var) >= 0
		&& ioctl(fb, FBIOGET_FSCREENINFO, &fix) >= 0) {
		buf = (unsigned char *)mmap(NULL, fix.smem_len, PROT_WRITE,  MAP_SHARED, fb, 0);
		printf("FB: %d x %d @ %dbpp\n", var.xres, var.yres, var.bits_per_pixel);
	} else {
		printf("Unable to get var and fix info!\n");
		return -1;
	}

	printf("Set color map ...\n");
	cmap.start = 0;
	cmap.len = 256;
	cmap.y = y_table;
	cmap.u = u_table;
	cmap.v = v_table;
	cmap.transp = blend_table;
	if (ioctl(fb, FBIOPUTCMAP, &cmap) < 0) {
		printf("Unable to put cmap!\n");
		return -1;
	}

	printf("Initializing frame buffer ...\n");
	ioctl(fb, FBIOPAN_DISPLAY, &var);

	printf("Setting frame buffer size ...\n");
	width =  fb_width_flag ? fb_width : var.xres;
	height = fb_height_flag ? fb_height : var.yres;
	var.xres = width;
	var.yres = height;
	printf("xres = %d, yres = %d, line = %d\n", var.xres, var.yres, fix.line_length);
	ioctl(fb, FBIOPUT_VSCREENINFO, &var);

	printf("Filling ...\n");
	color = fb_clut_id_flag ? fb_clut_id : 0;
	memset(buf, color, (var.yres * fix.line_length) >> 1);
	memset(buf + var.yres * fix.line_length / 2, color + 10, (var.yres * fix.line_length) >> 1);
	ioctl(fb, FBIOPAN_DISPLAY, &var);
	sleep(5);

	printf("Blanking ...\n");
	memset(buf, 0, var.yres * fix.line_length);
	ioctl(fb, FBIOBLANK, FB_BLANK_NORMAL);
	ioctl(fb, FBIOPAN_DISPLAY, &var);

	munmap(buf, fb);
	close(fb);

	return 0;
}

