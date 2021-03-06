#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#include "iav_drv.h"
#include "img_struct.h"
#include "ambas_vin.h"
#include "img_api.h"
#include "img_dsp_interface.h"
#include "mw_struct.h"
#include "mw_api.h"

#define LENS_SHADING_CALI_FILENAME			"lens_shading.bin"
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

static const char *default_filename = LENS_SHADING_CALI_FILENAME;
static char filename[256];

static int fd_iav;

static int detect_flag = 0;
static int correct_flag = 0;
static int restore_flag = 0;
static int flicker_mode = 50;
static int ae_target = 1024;

#define NO_ARG			0
#define HAS_ARG			1

mw_ae_param			ae_param = {
	.anti_flicker_mode		= MW_ANTI_FLICKER_50HZ,
	.shutter_time_min		= SHUTTER_1BY8000_SEC,
	.shutter_time_max		= SHUTTER_1BY30_SEC,
	.sensor_gain_max		= ISO_6400,
};

static struct option long_options[] = {
	{"detect", NO_ARG, 0, 'd'},
	{"correct", NO_ARG, 0, 'c'},
	{"restore", NO_ARG, 0, 'r'},
	{"anti-flicker", HAS_ARG, 0, 'a'},
	{"ae-target", HAS_ARG, 0, 't'},
	{"filename", HAS_ARG, 0, 'f'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "dcra:f:t:";

static const struct hint_s hint[] = {
	{"", "\t\tlens shading detect"},
	{"", "\t\tlens shading correct"},
	{"", "\t\tlens shading restore"},
	{"50|60", "specify anti-flicker mode"},
	{"1024~4096", "specify AE target value"},
	{"filename", "specify location for saving calibration result"},
};

static void usage(void)
{
	int i;

	printf("\ncali_lens_shading usage:\n");
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
	printf("\nExamples:\n");
	printf("\n  detect lens shading:\n    cali_lens_shading -d -a 50 -t 2048 -f lens_shading.bin\n");
	printf("\n  correct lens shading:\n    cali_lens_shading -c\n");
	printf("\n  restore lens shading:\n    cali_lens_shading -r\n");
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'd':
			detect_flag = 1;
			break;

		case 'c':
			correct_flag = 1;
			break;

		case 'r':
			restore_flag = 1;
			break;

		case 'a':
			flicker_mode = atoi(optarg);
			break;

		case 't':
			ae_target = atoi(optarg);
			break;

		case 'f':
			strcpy(filename, optarg);
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
	u16* raw_buff;
	int shutter_idx;
	u32 lookup_shift;
	iav_mmap_info_t mmap_info;
	iav_raw_info_t raw_info;
	u16 vignette_table[33*33*4] = {0};
	blc_level_t blc;
	still_cap_info_t still_cap_info;
	vignette_cal_t vig_detect_setup;
	vignette_info_t vignette_info = {0};
	int fd_lenshading = -1;
	int rval = -1;

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	if (mw_start_aaa(fd_iav) < 0) {
		perror("mw_start_aaa");
		return -1;
	}

	if (flicker_mode == 50)
		ae_param.anti_flicker_mode = MW_ANTI_FLICKER_50HZ;
	else
		ae_param.anti_flicker_mode = MW_ANTI_FLICKER_60HZ;

	mw_set_ae_param(&ae_param);
	img_ae_set_target_ratio(ae_target);
	if (detect_flag) {
		printf("wait 5 seconds...\n");
		sleep(5);
		while (1) {
			shutter_idx = img_get_sensor_shutter_index();
			if (flicker_mode == 50) {
				if (shutter_idx == 1106 || shutter_idx == 1234)	// 1/50s, 1/100s
					break;
			} else {	//anti-flicker 60 Hz
				if (shutter_idx == 1012 || shutter_idx == 1140 || shutter_idx == 1268)	// 1/30s, 1/60s, 1/120s
					break;
			}
			printf("Light is too strong (idx=%d)! Please turn the light box a little weaker.\n", shutter_idx);
			sleep(1);
		}
		printf("Light strength is okay.\n");
		img_enable_ae(0);
		img_enable_awb(0);
		img_enable_af(0);
		img_enable_adj(0);
		sleep(1);

		//Capture raw here
		printf("Raw capture started\n");
		memset(&still_cap_info, 0, sizeof(still_cap_info));
		still_cap_info.capture_num = 1;
		still_cap_info.need_raw = 1;
		img_init_still_capture(fd_iav, 95);
		img_start_still_capture(fd_iav, &still_cap_info);
		rval = ioctl(fd_iav, IAV_IOC_MAP_DSP, &mmap_info);
		if (rval < 0) {
			perror("IAV_IOC_MAP_DSP");
			return rval;
		}
		while (1) {
			rval = ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info);
			if (rval < 0) {
				perror("IAV_IOC_READ_RAW_INFO");
				continue;
			}
			break;
		}
		raw_buff = (u16*)malloc(raw_info.width*raw_info.height*sizeof(u16));
		memcpy(raw_buff,raw_info.raw_addr,(raw_info.width * raw_info.height * 2));
		img_stop_still_capture(fd_iav);
//		save_jpeg_stream();
		/*input*/
		vig_detect_setup.raw_addr = raw_buff;
		vig_detect_setup.raw_w = raw_info.width;
		vig_detect_setup.raw_h = raw_info.height;
		vig_detect_setup.bp = raw_info.bayer_pattern;
		vig_detect_setup.threshold = 8192;
		vig_detect_setup.compensate_ratio = 1024;
		vig_detect_setup.lookup_shift = 255;
		/*output*/
		vig_detect_setup.r_tab = vignette_table + 0*VIGNETTE_MAX_SIZE;
		vig_detect_setup.ge_tab = vignette_table + 1*VIGNETTE_MAX_SIZE;
		vig_detect_setup.go_tab = vignette_table + 2*VIGNETTE_MAX_SIZE;
		vig_detect_setup.b_tab = vignette_table + 3*VIGNETTE_MAX_SIZE;
		img_dsp_get_global_blc(&blc);
		vig_detect_setup.blc.r_offset = blc.r_offset;
		vig_detect_setup.blc.gr_offset = blc.gr_offset;
		vig_detect_setup.blc.gb_offset = blc.gb_offset;
		vig_detect_setup.blc.b_offset = blc.b_offset;
		rval = img_cal_vignette(&vig_detect_setup);
		if (rval < 0) {
			printf("img_cal_vignette error!\n");
			goto vignette_cal_exit;
		}
		lookup_shift = vig_detect_setup.lookup_shift;
		if ((fd_lenshading = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
			printf("vignette table file open error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, vignette_table, (4*VIGNETTE_MAX_SIZE*sizeof(u16)));
		if (rval < 0) {
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, &lookup_shift, sizeof(lookup_shift));
		if (rval < 0) {
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, &raw_info.width, sizeof(raw_info.width));
		if (rval < 0) {
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, &raw_info.height, sizeof(raw_info.height));
		if (rval < 0) {
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		close(fd_lenshading);
		//revert preview state
		rval = ioctl(fd_iav, IAV_IOC_LEAVE_STILL_CAPTURE);
		if (rval < 0) {
			perror("IAV_IOC_LEAVE_STILL_CAPTURE");
			goto vignette_cal_exit;
		}
		rval = 0;

vignette_cal_exit:
		free(raw_buff);
		return rval;
	}

	if (correct_flag || restore_flag) {
		int count;
		static u32 gain_shift;

		if (correct_flag) {
			if((fd_lenshading = open(filename, O_RDONLY, 0)) < 0) {
				printf("lens_shading.bin cannot be opened\n");
				return -1;
			}
			count = read(fd_lenshading, vignette_table, 4*VIGNETTE_MAX_SIZE*sizeof(u16));
			if (count != 4*VIGNETTE_MAX_SIZE*sizeof(u16)) {
				printf("read lens_shading.bin error\n");
				return -1;
			}
			count = read(fd_lenshading, &gain_shift, sizeof(u32));
			if (count != sizeof(u32)) {
				printf("read lens_shading.bin error\n");
				return -1;
			}
			vignette_info.enable = 1;
		} else if (restore_flag) {
			vignette_info.enable = 0;
		}

		vignette_info.gain_shift = (u8)gain_shift;
		vignette_info.vignette_red_gain_addr = (u32)(vignette_table + 0*VIGNETTE_MAX_SIZE);
		vignette_info.vignette_green_even_gain_addr = (u32)(vignette_table + 1*VIGNETTE_MAX_SIZE);
		vignette_info.vignette_green_odd_gain_addr = (u32)(vignette_table + 2*VIGNETTE_MAX_SIZE);
		vignette_info.vignette_blue_gain_addr = (u32)(vignette_table + 3*VIGNETTE_MAX_SIZE);
		rval = img_dsp_set_vignette_compensation(fd_iav, &vignette_info);
	}

	return 0;
}

