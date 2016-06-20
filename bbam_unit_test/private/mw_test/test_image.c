/***********************************************************
 * test_image.c
 *
 * History:
 *	2010/03/25 - [Jian Tang] created file
 *
 * Copyright (C) 2008-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ***********************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <getopt.h>
#include <sched.h>
#include <errno.h>
#include <basetypes.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/signal.h>

#include "iav_drv.h"
#include "ambas_common.h"
#include "ambas_vin.h"

#include "ambas_imgproc.h"

#include "img_struct.h"
#include "img_api.h"

#include "mw_struct.h"
#include "mw_api.h"


#define	DIV_ROUND(divident, divider)    (((divident)+((divider)>>1)) / (divider))

typedef enum {
	STATIS_AWB = 0,
	STATIS_AE,
	STATIS_AF,
	STATIS_HIST,
} STATISTICS_TYPE;

int fd_iav;
int fd_vin = -1;
mw_local_exposure_curve local_exposure_curve;

/****************************************************/
/*	AE Related Settings									*/
/****************************************************/
mw_ae_param			ae_param = {
	.anti_flicker_mode		= MW_ANTI_FLICKER_60HZ,
	.shutter_time_min		= SHUTTER_1BY8000_SEC,
	.shutter_time_max		= SHUTTER_1BY30_SEC,
	.sensor_gain_max		= ISO_6400,
	.slow_shutter_enable	= 0,
};
u32	exposure_level		= 100;
mw_ae_metering_mode metering_mode	= MW_AE_CENTER_METERING;
u32	dc_iris_enable		= 0;
u32	dc_iris_duty_balance	= 575;

static mw_ae_line ae_60hz_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY120_SEC, ISO_100, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY120_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY120_SEC, ISO_200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY15_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

static mw_ae_line ae_50hz_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY100_SEC, ISO_100, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY100_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY100_SEC, ISO_200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY50_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY50_SEC, ISO_200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY25_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY25_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY15_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

static mw_ae_line ae_customer_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_100, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_150, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_150, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_150, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_150, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_3200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_3200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_3200, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_3200, APERTURE_AUTO}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_102400, APERTURE_AUTO}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

/****************************************************/
/*	AWB Related Settings								*/
/****************************************************/
u32	white_balance_mode	= MW_WB_AUTO;

/****************************************************/
/*	Image Adjustment Settings							*/
/****************************************************/
u32	saturation			= 64;
u32	brightness			= 0;
u32	contrast				= 64;
u32	sharpness			= 128;

/****************************************************/
/*	Image Enhancement Settings							*/
/****************************************************/
u32	mctf_strength			= 32;
u32	auto_local_exposure_mode	= 0;
u32	backlight_comp_enable		= 0;
u32	day_night_mode_enable	= 0;


/***************************************************************************************
	function:	int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
	input:	input_str: input string line for analysis
			argarr: buffer array accommodating the parsed arguments
			argcnt: number of the arguments parsed
	return value:	0: successful, -1: failed
	remarks:
***************************************************************************************/
static int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
{
	int i = 0;
	char *delim = ",:-\n\t";
	char *ptr;
	*argcnt = 0;

	ptr = strtok(input_str, delim);
	if (ptr == NULL)
		return 0;
	argarr[i++] = atoi(ptr);

	while (1) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i++] = atoi(ptr);
	}

	*argcnt = i;
	return 0;
}


/***************************************************************************************
	function:	int load_local_exposure_curve(char *lect_filename, u16 *local_exposure_curve)
	input:	lect_filename: filename of local exposure curve table to be loaded
			local_exposure_curve: buffer structure accommodating the loaded local exposure curve
	return value:	0: successful, -1: failed
	remarks:
***************************************************************************************/
static int load_local_exposure_curve(char *lect_filename, mw_local_exposure_curve *local_exposure_curve)
{
	char key[64] = "local_exposure_curve";
	char line[1024];
	FILE *fp;
	int find_key = 0;
	u16 *param = &local_exposure_curve->gain_curve_table[0];
	int argcnt;

	if ((fp = fopen(lect_filename, "r")) == NULL) {
		printf("Open local exposure curve file [%s] failed!\n", lect_filename);
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strstr(line, key) != NULL) {
			find_key = 1;
			break;
		}
	}

	if (find_key) {
		while ( fgets(line, sizeof(line), fp) != NULL) {
			get_multi_arg(line, param, &argcnt);
//			printf("argcnt %d\n", argcnt);
			param += argcnt;
			if (argcnt == 0)
				break;
		}
	}

	if (!find_key)
		return -1;
	return 0;
}

static int set_sensor_fps(u32 fps)
{
	static int init_flag = 0;
	static char vin_arr[8];
	u32 frame_time = AMBA_VIDEO_FPS_29_97;
	switch (fps) {
	case 0:
		frame_time = AMBA_VIDEO_FPS_29_97;
		break;
	case 1:
		frame_time = AMBA_VIDEO_FPS_30;
		break;
	case 2:
		frame_time = AMBA_VIDEO_FPS_15;
		break;
	case 3:
		frame_time = AMBA_VIDEO_FPS_7_5;
		break;
	default:
		frame_time = AMBA_VIDEO_FPS_29_97;
		break;
	}
	if (init_flag == 0) {
		fd_vin =open("/proc/ambarella/vin0_vsync", O_RDWR);
		if (fd_vin < 0) {
			printf("CANNOT OPEN VIN FILE!\n");
			return -1;
		}
		init_flag = 1;
	}
	read(fd_vin, vin_arr, 8);
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_FRAME_RATE, frame_time) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
		return -1;
	}
	printf("[Done] set sensor fps [%d] [%d].\n", fps, frame_time);
	return 0;
}

static int update_sensor_fps(void)
{
	if (ioctl(fd_iav, IAV_IOC_UPDATE_VIN_FRAMERATE_EX, 0) < 0) {
		perror("IAV_IOC_UPDATE_VIN_FRAMERATE_EX");
		return -1;
	}
	printf("[Done] update_vin_frame_rate !\n");
	return 0;
}

static int show_global_setting_menu(void)
{
	printf("\n================ Global Settings ================\n");
	printf("  s -- 3A library start and stop\n");
//	printf("  f -- Change Sensor fps\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("G > ");
	return 0;
}

static int global_setting(int imgproc_running)
{
	int i, exit_flag, error_opt;
	char ch;

	show_global_setting_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 's':
			printf("Imgproc library is %srunning.\n",
				imgproc_running ? "" : "not ");
			printf("0 - Stop 3A library, 1 - Start 3A library\n> ");
			scanf("%d", &i);
			if (i == 0) {
				if (imgproc_running == 1) {
					mw_stop_aaa();
					imgproc_running = 0;
				}
			} else if (i == 1) {
				if (imgproc_running == 0) {
					mw_start_aaa(fd_iav);
					imgproc_running = 1;
				}
			} else {
				printf("Invalid input for this option!\n");
			}
			break;
		case 'f':
			printf("Set sensor frame rate : 0 - 29.97, 1 - 30, 2 - 15, 3 - 7.5\n");
			scanf("%d", &i);
			set_sensor_fps(i);
			update_sensor_fps();
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_global_setting_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_exposure_setting_menu(void)
{
	printf("\n================ Exposure Control Settings ================\n");
	printf("  E -- AE enable and disable\n");
	printf("  e -- Set exposure level\n");
	printf("  a -- Anti-flicker mode\n");
	printf("  s -- Manually set shutter time\n");
	printf("  T -- Shutter time max\n");
	printf("  t -- Shutter time min\n");
	printf("  R -- Slow shutter enable and disable\n");
	printf("  g -- Sensor gain max\n");
	printf("  G -- Manually set sensor gain\n");
	printf("  m -- Set AE metering mode\n");
	printf("  l -- Get AE current lines\n");
	printf("  L -- Set AE customer lines\n");
	printf("  p -- Set AE switch point\n");
	printf("  D -- DC iris enable and disable\n");
	printf("  d -- DC iris balance duty\n");
	printf("  y -- Get Luma value\n");
	printf("  r -- Get current shutter time and sensor gain\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("E > ");
	return 0;
}

static int exposure_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	u32 value;
	mw_ae_metering_table	custom_ae_metering_table[2] = {
		{	//Left half window as ROI
			{1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,}
		},
		{	//Right half window as ROI
			{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,}
		},
	};
	mw_ae_line lines[10];
	mw_ae_point switch_point;

	show_exposure_setting_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'E':
			printf("0 - AE disable  1 - AE enable\n> ");
			scanf("%d", &i);
			mw_enable_ae(i);
			break;
		case 'e':
			mw_get_exposure_level(&i);
			printf("Current exposure level is %d\n", i);
			printf("Input new exposure level: (range 25 ~ 400)\n> ");
			scanf("%d", &i);
			mw_set_exposure_level(i);
			break;
		case 'a':
			printf("Anti-flicker mode? 0 - 50Hz  1 - 60Hz\n> ");
			scanf("%d", &i);
			mw_get_ae_param(&ae_param);
			ae_param.anti_flicker_mode = i;
			mw_set_ae_param(&ae_param);
			break;
		case 's':
			mw_get_shutter_time(fd_iav, &i);
			printf("Current shutter time is 1/%d sec\n", DIV_ROUND(512000000, i));
			printf("Input new shutter time in 1/n sec format: (range 1 ~ 8000)\n> ");
			scanf("%d", &i);
			mw_set_shutter_time(fd_iav, DIV_ROUND(512000000, i));
			break;
		case 'T':
			mw_get_ae_param(&ae_param);
			printf("Current shutter time max is 1/%d sec\n",
				DIV_ROUND(512000000, ae_param.shutter_time_max));
			printf("Input new shutter time max in 1/n sec fomat? (Ex, 6, 10, 15...)(slow shutter)\n> ");
			scanf("%d", &i);
			ae_param.shutter_time_max = DIV_ROUND(512000000, i);
			mw_set_ae_param(&ae_param);
			break;
		case 't':
			mw_get_ae_param(&ae_param);
			printf("Current shutter time min is 1/%d sec\n",
				DIV_ROUND(512000000, ae_param.shutter_time_min));
			printf("Input new shutter time min in 1/n sec fomat? (Ex, 120, 200, 400 ...)\n> ");
			scanf("%d", &i);
			ae_param.shutter_time_min = DIV_ROUND(512000000, i);
			mw_set_ae_param(&ae_param);
			break;
		case 'R':
			mw_get_ae_param(&ae_param);
			printf("Current slow shutter mode is %s.\n",
				ae_param.slow_shutter_enable ? "enabled" : "disabled");
			printf("Input slow shutter mode? (0 - disable, 1 - enable)\n> ");
			scanf("%d", &i);
			ae_param.slow_shutter_enable = !!i;
			mw_set_ae_param(&ae_param);
			break;
		case 'g':
			mw_get_ae_param(&ae_param);
			printf("Current sensor gain max is %d dB\n",
				ae_param.sensor_gain_max);
			printf("Input new sensor gain max in dB? (Ex, 24, 36, 48 ...)\n> ");
			scanf("%d", &i);
			ae_param.sensor_gain_max = i;
			mw_set_ae_param(&ae_param);
			break;
		case 'G':
			mw_get_sensor_gain(fd_iav, &i);
			printf("Current sensor gain is %d dB\n", i >> 24);
			printf("Input new sensor gain in dB: (range 0 ~ 36)\n> ");
			scanf("%d", &i);
			mw_set_sensor_gain(fd_iav, i << 24);
			break;
		case 'm':
			printf("0 - Spot Metering, 1 - Center Metering, 2 - Average Metering, 3 - Custom Metering\n");
			mw_get_ae_metering_mode(&metering_mode);
			printf("Current ae metering mode is %d\n", metering_mode);
			printf("Input new ae metering mode:\n> ");
			scanf("%d", (int *)&metering_mode);
			mw_set_ae_metering_mode(metering_mode);
			if (metering_mode != MW_AE_CUSTOM_METERING)
				break;
			printf("Please choose the AE window:\n");
			printf("0 - left half window, 1 - right half window\n> ");
			scanf("%d", &i);
			mw_set_ae_metering_table(&custom_ae_metering_table[i]);
			break;
		case 'l':
			printf("Get current AE lines:\n");
			mw_get_ae_lines(lines, 10);
			break;
		case 'L':
			printf("Set customer AE lines: (0: 60Hz, 1: 50Hz, 2: customize)\n> ");
			scanf("%d", &i);
			if (i == 0) {
				memcpy(lines, ae_60hz_lines, sizeof(ae_60hz_lines));
				mw_set_ae_lines(lines, sizeof(ae_60hz_lines) / sizeof(mw_ae_line));
			} else if (i == 1) {
				memcpy(lines, ae_50hz_lines, sizeof(ae_50hz_lines));
				mw_set_ae_lines(lines, sizeof(ae_50hz_lines) / sizeof(mw_ae_line));
			} else if (i == 2) {
				memcpy(lines, ae_customer_lines, sizeof(ae_customer_lines));
				mw_set_ae_lines(lines, sizeof(ae_customer_lines) / sizeof(mw_ae_line));
			} else {
				printf("Invalid input : %d.\n", i);
			}
			break;
		case 'p':
			printf("Please set switch point in AE lines :\n");
			printf("Input shutter time in 1/n sec format: (range 30 ~ 120)\n> ");
			scanf("%d", &i);
			switch_point.factor[MW_SHUTTER] = DIV_ROUND(512000000, i);
			printf("Input switch point of AE line: (0 - start point; 1 - end point)\n> ");
			scanf("%d", &i);
			switch_point.pos = i;
			printf("Input sensor gain in dB: (range 0 ~ 36)\n> ");
			scanf("%d", &i);
			switch_point.factor[MW_DGAIN] = i;
			mw_set_ae_points(&switch_point, 1);
			break;
		case 'D':
			printf("DC iris control? 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_enable_dc_iris_control(i);
			break;
		case 'd':
			mw_get_dc_iris_balance_duty(&value);
			printf("Current balance duty cycle is %d\n", value);
			printf("Input new balance duty cycle: (range 400 ~ 800)\n> ");
			scanf("%d", &value);
			mw_set_dc_iris_balance_duty(value);
			break;
		case 'y':
			mw_get_ae_luma_value(&i);
			printf("Current luma value is %d.\n", i);
			break;
		case 'r':
			mw_get_shutter_time(fd_iav, &i);
			printf("Current shutter time is 1/%d sec.\n", DIV_ROUND(512000000, i));
			mw_get_sensor_gain(fd_iav, &i);
			printf("Current sensor gain is %d dB.\n", (i >> 24));
			i = img_ae_get_system_gain();
			i = i * 1024 * 6 / 128;
			printf("Current system gain is %d dB.\n", (i >> 10));
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_exposure_setting_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_white_balance_menu(void)
{
	printf("\n================ White Balance Settings ================\n");
	printf("  W -- AWB enable and disable\n");
	printf("  m -- Select AWB mode\n");
	printf("  M -- Select AWB method\n");
	printf("  g -- Manually set RGB gain (Need to turn off AWB first)\n");
	printf("  c -- Set RGB gain for custom mode (Don't need to turn off AWB)\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("W > ");
	return 0;
}

static int white_balance_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	mw_wb_gain wb_gain;
	mw_white_balance_method wb_method;

	show_white_balance_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'W':
			printf("0 - AWB disable  1 - AWB enable\n> ");
			scanf("%d", &i);
			mw_enable_awb(i);
			break;
		case 'm':
			printf("Choose AWB mode (0 - auto 1 - 2800K 2 - 4000K 3 - 5000K 4 - 6500K 5 - 7500K)\n> ");
			scanf("%d", &i);
			mw_set_white_balance_mode(i);
			break;
		case 'M':
			mw_get_awb_method(&wb_method);
			printf("Current AWB method is [%d].\n", wb_method);
			printf("Choose AWB method (0 - Normal, 1 - Custom, 2 - Grey world)\n> ");
			scanf("%d", &i);
			mw_set_awb_method(i);
			break;
		case 'g':
			mw_get_rgb_gain(&wb_gain);
			printf("Current rgb gain is %d,%d,%d\n", wb_gain.r_gain,
				wb_gain.g_gain, wb_gain.b_gain);
			printf("Input new rgb gain (Ex, 1500,1024,1400)\n> ");
			scanf("%d,%d,%d", &wb_gain.r_gain, &wb_gain.g_gain, &wb_gain.b_gain);
			wb_gain.d_gain = 1024;
			mw_set_rgb_gain(fd_iav, &wb_gain);
			break;
		case 'c':
			mw_get_rgb_gain(&wb_gain);
			printf("Current RGB gain is %d, %d, %d\n", wb_gain.r_gain,
				wb_gain.g_gain, wb_gain.b_gain);
			printf("Input new RGB gain for custom mode (Ex, 1500,1024,1400)\n> ");
			scanf("%d,%d,%d", &wb_gain.r_gain, &wb_gain.g_gain, &wb_gain.b_gain);
			printf("Enter 'Custom' WB mode...\n");
			mw_set_white_balance_mode(MW_WB_CUSTOM);
			printf("The new RGB gain you set for custom mode is : %d, %d, %d\n",
				wb_gain.r_gain, wb_gain.g_gain, wb_gain.b_gain);
			mw_set_custom_gain(&wb_gain);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_white_balance_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_adjustment_menu(void)
{
	printf("\n================ Image Adjustment Settings ================\n");
	printf("  a -- Saturation adjustment\n");
	printf("  b -- Brightness adjustment\n");
	printf("  c -- Contrast adjustment\n");
	printf("  s -- Sharpness adjustment\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("A > ");
	return 0;
}

static int adjustment_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;

	show_adjustment_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'a':
			mw_get_saturation(&i);
			printf("Current saturation is %d\n", i);
			printf("Input new saturation: (range 0 ~ 255)\n> ");
			scanf("%d", &i);
			mw_set_saturation(i);
			break;
		case 'b':
			mw_get_brightness(&i);
			printf("Current brightness is %d\n", i);
			printf("Input new brightness: (range -255 ~ 255)\n> ");
			scanf("%d", &i);
			mw_set_brightness(i);
			break;
		case 'c':
			mw_get_contrast(&i);
			printf("Current contrast is %d\n", i);
			printf("Input new contrast: (range 0 ~ 128)\n> ");
			scanf("%d", &i);
			mw_set_contrast(i);
			break;
		case 's':
			mw_get_sharpness(&i);
			printf("Current sharpness is %d\n", i);
			printf("Input new sharpness: (range 0 ~ 255)\n> ");
			scanf("%d", &i);
			mw_set_sharpness(i);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_adjustment_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_enhancement_menu(void)
{
	printf("\n================ Image Enhancement Settings ================\n");
	printf("  m -- Set MCTF 3D noise filter strength\n");
	printf("  L -- Set auto local exposure mode\n");
	printf("  l -- Load custom local exposure curve from file\n");
	printf("  b -- Enable backlight compensation\n");
	printf("  d -- Enable day and night mode\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("H > ");
	return 0;
}

static int enhancement_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	u32 value;
	char str[64];
	mw_local_exposure_curve local_exposure_curve;

	show_enhancement_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'm':
			mw_get_mctf_strength(&value);
			printf("Current mctf strength is %d\n", value);
			printf("Input new mctf strength: (range 0 ~ 512)\n> ");
			scanf("%d", &value);
			mw_set_mctf_strength(value);
			break;
		case 'L':
			printf("Auto local exposure (0~255): 0 - Stop, 1 - Auto, 2~255 - weakest~strongest\n> ");
			scanf("%d", &i);
			mw_set_auto_local_exposure_mode(i);
			break;
		case 'l':
			printf("Input the filename of local exposure curve: (Ex, le_2x.txt)\n> ");
			scanf("%s", str);
			if (load_local_exposure_curve(str, &local_exposure_curve) < 0) {
				printf("The curve file %s is either found or corrupted!\n", str);
				return -1;
			}
			mw_set_local_exposure_curve(fd_iav, &local_exposure_curve);
			break;
		case 'b':
			printf("Backlight compensation: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_enable_backlight_compensation(i);
			break;
		case 'd':
			printf("Day and night mode: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			mw_enable_day_night_mode(i);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_enhancement_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_misc_menu(void)
{
	printf("\n================ Misc Settings ================\n");
	printf("  v -- Set log level\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("M > ");
	return 0;
}

static int misc_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;

	show_misc_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'v':
			printf("Input log level: (0~2)\n> ");
			scanf("%d", &i);
			mw_set_log_level(i);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_misc_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_statistics_menu(void)
{
	printf("\n================ Statistics Data ================\n");
	printf("  b -- Get AWB statistics data once\n");
	printf("  e -- Get AE statistics data once\n");
	printf("  f -- Get AF statistics data once\n");
	printf("  h -- Get Histogram data once\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("S > ");
	return 0;
}

static int display_AWB_data(struct cfa_awb_stat * cfa_awb,
	u16 tile_num_col, u16 tile_num_row)
{
	u32 i, sum_r, sum_g, sum_b, total_size;

	sum_r = sum_g = sum_b = 0;
	total_size = tile_num_col * tile_num_row;

	for (i = 0; i < total_size; ++i) {
		sum_r += cfa_awb[i].sum_r;
		sum_g += cfa_awb[i].sum_g;
		sum_b += cfa_awb[i].sum_b;
	}
	printf("== AWB = [%dx%d] = [R : G : B] -> [%d : %d : %d].\n",
		tile_num_col, tile_num_row, sum_r, sum_g, sum_b);
	sum_r = (sum_r << 10) / sum_g;
	sum_b = (sum_b << 10) / sum_b;
	printf("== AWB = Normalized to 1024 [%d : 1024 : %d].\n",
		sum_r, sum_b);

	return 0;
}

static int display_AE_data(struct cfa_ae_stat * cfa_ae, u16 *rgb_sum_y,
	u16 tile_num_col, u16 tile_num_row)
{
	int i, rgb_sum, cfa_sum, total_size;

	rgb_sum = cfa_sum = 0;
	total_size = tile_num_col * tile_num_row;

	for (i = 0; i < total_size; ++i) {
		rgb_sum += rgb_sum_y[i];
		cfa_sum += cfa_ae[i].lin_y;
	}
	printf("== AE = [%dx%d] == Total lum value : CFA [%d], RGB [%d].\n",
		tile_num_col, tile_num_row, cfa_sum, rgb_sum);

	return 0;
}

static int display_AF_data(struct af_stat * cfa_af, struct af_stat * rgb_af,
	u16 tile_num_col, u16 tile_num_row)
{
	int i, total_size, shift_bits;
	u32 cfa_sum_fv1, cfa_sum_fv2, cfa_sum_lum;
	u32 rgb_sum_fv1, rgb_sum_fv2, rgb_sum_lum;

	total_size = tile_num_col * tile_num_row;
	cfa_sum_fv1 = cfa_sum_fv2 = cfa_sum_lum = 0;
	rgb_sum_fv1 = rgb_sum_fv2 = rgb_sum_lum = 0;
	shift_bits = 10;

	for (i = 0; i < total_size; ++i) {
		cfa_sum_fv1 += ((cfa_af[i].sum_fv1*cfa_af[i].sum_fv1) >> shift_bits);
		cfa_sum_fv2 += ((cfa_af[i].sum_fv2*cfa_af[i].sum_fv2) >> shift_bits);
		cfa_sum_lum += ((cfa_af[i].sum_fy*cfa_af[i].sum_fy) >> shift_bits);
		rgb_sum_fv1 += ((rgb_af[i].sum_fv1*rgb_af[i].sum_fv1) >> shift_bits);
		rgb_sum_fv2 += ((rgb_af[i].sum_fv2*rgb_af[i].sum_fv2) >> shift_bits);
		rgb_sum_lum += ((rgb_af[i].sum_fy*rgb_af[i].sum_fy) >> shift_bits);
	}
	printf("== AF = [%dx%d] == [FV1, FV2, Lum] :"
		" CFA [%d, %d, %d], RGB [%d, %d, %d].\n",
		tile_num_col, tile_num_row,
		cfa_sum_fv1, cfa_sum_fv2, cfa_sum_lum,
		rgb_sum_fv1, rgb_sum_fv2, rgb_sum_lum);

	return 0;
}

static int display_hist_data(struct cfa_histogram_stat * cfa,
	struct rgb_histogram_stat * rgb)
{
	const int total_bin_num = 64;
	int bin_num;

	while (1) {
		printf("  Please choose the bin number (range 0~63) : ");
		scanf("%d", &bin_num);
		if ((bin_num >= 0) && (bin_num < total_bin_num))
			break;
		printf("  Invalid bin number [%d], choose again!\n", bin_num);
	}

	printf("== HIST Bin [%d] [Y : R : G : B] = [%d : %d : %d : %d]\n", bin_num,
		rgb->his_bin_y[bin_num], rgb->his_bin_r[bin_num],
		rgb->his_bin_g[bin_num], rgb->his_bin_b[bin_num]);

	return 0;
}

static int get_statistics_data(STATISTICS_TYPE type)
{
	struct img_statistics arg;
	struct cfa_aaa_stat cfa_aaa;
	struct rgb_aaa_stat rgb_aaa;
	struct rgb_histogram_stat hist_aaa;

	int retv = 0;
	u8 cfa_valid, rgb_valid, hist_valid;
	u16 awb_tile_col, awb_tile_row;
	u16 ae_tile_col, ae_tile_row;
	u16 af_tile_col, af_tile_row;

	u16 *ae_sum_y = NULL;
	struct cfa_awb_stat * pawb_stat = NULL;
	struct cfa_ae_stat * pae_stat = NULL;
	struct af_stat * pcfa_af_stat = NULL, * prgb_af_stat = NULL;

	memset(&arg, 0, sizeof(arg));
	memset(&cfa_aaa, 0, sizeof(cfa_aaa));
	memset(&rgb_aaa, 0, sizeof(rgb_aaa));
	memset(&hist_aaa, 0, sizeof(hist_aaa));

	cfa_valid = rgb_valid = hist_valid = 0;
	arg.cfa_data_valid = &cfa_valid;
	arg.rgb_data_valid = &rgb_valid;
	arg.hist_data_valid = &hist_valid;
	arg.cfa_statis = &cfa_aaa;
	arg.rgb_statis = &rgb_aaa;
	arg.hist_statis = &hist_aaa;

	if (ioctl(fd_iav, IAV_IOC_IMG_GET_STATISTICS, &arg) < 0) {
		perror("IAV_IOC_IMG_GET_STATISTICS");
		if (errno == EINTR)
			exit(1);
		return -1;
	}
	awb_tile_row = cfa_aaa.tile_info.awb_tile_num_row;
	awb_tile_col = cfa_aaa.tile_info.awb_tile_num_col;
	ae_tile_row = cfa_aaa.tile_info.ae_tile_num_row;
	ae_tile_col = cfa_aaa.tile_info.ae_tile_num_col;
	af_tile_row = cfa_aaa.tile_info.af_tile_num_row;
	af_tile_col = cfa_aaa.tile_info.af_tile_num_col;
	pawb_stat = cfa_aaa.awb_stat;
	pae_stat = (struct cfa_ae_stat * )((u8 *)pawb_stat + awb_tile_row *
		awb_tile_col * sizeof(struct cfa_awb_stat));
	pcfa_af_stat = (struct af_stat *)((u8 *)pae_stat + ae_tile_row *
		ae_tile_col * sizeof(struct cfa_awb_stat));
	ae_sum_y = rgb_aaa.ae_sum_y;
	prgb_af_stat = rgb_aaa.af_stat;

	switch (type) {
	case STATIS_AWB:
		retv = display_AWB_data(pawb_stat, awb_tile_col, awb_tile_row);
		break;
	case STATIS_AE:
		retv = display_AE_data(pae_stat, ae_sum_y, ae_tile_col, ae_tile_row);
		break;
	case STATIS_AF:
		retv = display_AF_data(pcfa_af_stat, prgb_af_stat,
			af_tile_col, af_tile_row);
		break;
	case STATIS_HIST:
		retv = display_hist_data(&cfa_aaa.histogram_stat, &rgb_aaa.histogram_stat);
		break;
	default:
		printf("Invalid statistics type !\n");
		retv = -1;
		break;
	}

	return retv;
}

static int statistics_data_setting(void)
{
	int exit_flag, error_opt;
	char ch;

	show_statistics_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'b':
			get_statistics_data(STATIS_AWB);
			break;
		case 'e':
			get_statistics_data(STATIS_AE);
			break;
		case 'f':
			get_statistics_data(STATIS_AF);
			break;
		case 'h':
			get_statistics_data(STATIS_HIST);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_statistics_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_menu(void)
{
	printf("\n================================================\n");
	printf("  g -- Global settings (3A library control, sensor fps, read 3A info)\n");
	printf("  e -- Exposure control settings (AE lines, shutter, gain, DC-iris, metering)\n");
	printf("  w -- White balance settings (AWB control, WB mode, RGB gain)\n");
	printf("  a -- Adjustment settings (Saturation, brightness, contrast, ...)\n");
	printf("  h -- Enhancement settings (3D de-noise, local exposure, backlight)\n");
	printf("  s -- Get statistics data (AE, AWB, AF, Histogram)\n");
	printf("  m -- Misc settings (Log level)\n");
	printf("  q -- Quit");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}

static void sigstop(int signo)
{
	mw_stop_aaa();
	exit(1);
}

int main(int argc, char ** argv)
{
	char ch, error_opt;
	int quit_flag, imgproc_running_flag;

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	if (mw_start_aaa(fd_iav) < 0) {
		perror("mw_start_aaa");
		return -1;
	}
	imgproc_running_flag = 1;

	/*
	 * Exposure Control Settings
	 */
	mw_enable_ae(1);
	mw_set_ae_param(&ae_param);
	mw_set_exposure_level(exposure_level);
	mw_set_ae_metering_mode(metering_mode);
	mw_enable_dc_iris_control(dc_iris_enable);
	mw_set_dc_iris_balance_duty(dc_iris_duty_balance);

	/*
	 * White Balance Settings
	 */
	mw_enable_awb(1);
	mw_set_white_balance_mode(MW_WB_AUTO);

	/*
	 * Image Adjustment Settings
	 */
	mw_set_saturation(saturation);
	mw_set_brightness(brightness);
	mw_set_contrast(contrast);
	mw_set_sharpness(sharpness);

	/*
	 * Image Enhancement Settings
	 */
	mw_set_mctf_strength(mctf_strength);
	mw_set_auto_local_exposure_mode(auto_local_exposure_mode);
	mw_enable_backlight_compensation(backlight_comp_enable);
	mw_enable_day_night_mode(day_night_mode_enable);

	show_menu();
	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'g':
			global_setting(imgproc_running_flag);
			break;
		case 'e':
			exposure_setting();
			break;
		case 'w':
			white_balance_setting();
			break;
		case 'a':
			adjustment_setting();
			break;
		case 'h':
			enhancement_setting();
			break;
		case 's':
			statistics_data_setting();
			break;
		case 'm':
			misc_setting();
			break;
		case 'q':
			quit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (quit_flag)
			break;
		if (error_opt == 0) {
			show_menu();
		}
		ch = getchar();
	}

	if (mw_stop_aaa() < 0) {
		perror("mw_stop_aaa");
		return -1;
	}

	close(fd_iav);

	return 0;
}

