/*
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#include "img_struct.h"
#include "ambas_vin.h"
#include "img_api.h"
#include "img_dsp_interface.h"

#include "imx035_adj_param.c"
#include "imx035_aeb_param.c"

#include "imx036_adj_param.c"
#include "imx036_aeb_param.c"

#include "imx072_adj_param.c"
#include "imx072_aeb_param.c"

#include "imx122_adj_param.c"
#include "imx122_aeb_param.c"

#include "ov2710_adj_param.c"
#include "ov2710_aeb_param.c"

#include "ov9715_adj_param.c"
#include "ov9715_aeb_param.c"

#include "ov9718_adj_param.c"
#include "ov9718_aeb_param.c"

#include "ov5653_adj_param.c"
#include "ov5653_aeb_param.c"

#include "ov14810_adj_param.c"
#include "ov14810_aeb_param.c"

#include "mt9m033_adj_param.c"
#include "mt9m033_aeb_param.c"

#include "mt9p006_adj_param.c"
#include "mt9p006_aeb_param.c"

#include "mt9t002_adj_param.c"
#include "mt9t002_aeb_param.c"

#include "s5k5b3gx_adj_param.c"
#include "s5k5b3gx_aeb_param.c"

#include "ar0331_adj_param.c"
#include "ar0331_aeb_param.c"

#include "ar0130_adj_param.c"
#include "ar0130_aeb_param.c"

#include "imx104_adj_param.c"
#include "imx104_aeb_param.c"

#include "mn34041pl_adj_param.c"
#include "mn34041pl_aeb_param.c"

#define	IMGPROC_PARAM_PATH		"/etc/idsp"

#ifndef	ARRAY_SIZE
#define	ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif

#ifndef ABS
#define ABS(a)	(((a) < 0) ? -(a) : (a))
#endif

#ifndef	MAX
#define MAX(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#define REG_SIZE			18752
#define MATRIX_SIZE			17536
#define CALI_AWB_FILENAME	"cali_awb.txt"

static const char *default_filename = CALI_AWB_FILENAME;
static char cali_awb_filename[256];
static char load_awb_filename[256];

static int fd_iav;
static int detect_flag = 0;
static int correct_flag = 0;
static int restore_flag = 0;
static int save_file_flag = 0;

static wb_gain_t wb_gain;
static int correct_param[12];

#define NO_ARG	0
#define HAS_ARG	1

static struct option long_options[] = {
	{"detect",   NO_ARG,  0, 'd'},
	{"correct",  HAS_ARG, 0, 'c'},
	{"restore",  NO_ARG, 0, 'r'},
	{"savefile", HAS_ARG, 0, 'f'},
	{"loadfile", HAS_ARG, 0, 'l'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "dc:rf:l:";

static struct hint_s hint[] = {
	{"", "\t\tGet current AWB gain"},
	{"[v1,v2,...,v12]", "\t\torignal AWB gain in R/G/Bin low/high light, target WB gain in R/G/B in low/high light "},
	{"", "\t\tRestore AWB as default"},
	{"", "\t\tSpecify the file to save current AWB gain"},
	{"", "\t\tload the file for correction"},
};

static void sigstop()
{
	img_stop_aaa();
	printf("Quit cali_wb.\n");
	exit(1);
}

static void usage(void)
{
	int i;
	printf("\ncali_app usage:\n");
	for (i = 0; i < ARRAY_SIZE(long_options) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}

	printf("\n\n");
	printf("The white balance calibration is defined as a gain correction for\n");
	printf("red, green and blue components by gain factors.\n");
	printf("You can use -d option with the perfect sensor and light to get the\n");
	printf("target AWB gain factors (1024 as a unit) for your reference.\n");
	printf("When applying calibration for imperfect sensor, specify the wrong\n");
	printf("gain and the target gain to do calibration by -c option.\n\n");
}

static int get_multi_arg(char *optarg, int *argarr, int argcnt)
{
	int i;
	char *delim = ",:-";
	char *ptr;

	ptr = strtok(optarg, delim);
	argarr[0] = atoi(ptr);

	for (i = 1; i < argcnt; i++) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i] = atoi(ptr);
	}
	if (i < argcnt)
		return -1;
	return 0;
}

static int init_param(int argc, char **argv)
{
	int ch, i = 0, data[3];
	int option_index = 0;
	FILE *fp;
	char s[256];
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'd':
			detect_flag = 1;
			break;
		case 'c':
			correct_flag = 1;
			if (get_multi_arg(optarg, correct_param, ARRAY_SIZE(correct_param)) < 0) {
				printf("Need %d args for opt %c !\n", ARRAY_SIZE(correct_param), ch);
				return -1;
			}
			while (i < ARRAY_SIZE(correct_param)) {
				if (correct_param[i] <= 0) {
					printf("args should be greater than 0!\n");
					return -1;
				}
				i++;
			}
			break;
		case 'r':
			restore_flag = 1;
			break;
		case 'f':
			save_file_flag = 1;
			strcpy(cali_awb_filename, optarg);
			break;
		case 'l':
			correct_flag = 1;
			strcpy(load_awb_filename, optarg);
			if (strlen(load_awb_filename) == 0) {
				printf("Please input the file path used for correction. It is generated by awb_calibration.sh.\n");
				return -1;
			}
			if ((fp = fopen(load_awb_filename, "r+")) == NULL) {
				printf("Open file %s error.\n", load_awb_filename);
				return -1;
			}
			i = 0;
			while (NULL != fgets(s, sizeof(s), fp) && (i < 4)) {
				if (get_multi_arg(s, data, 3) < 0) {
					printf("wrong data format in %s.\n", load_awb_filename);
					fclose(fp);
					return -1;
				}
				if (data[0] <= 0 || data[1] <= 0 || data[2] <= 0) {
					printf("Data in %s should be greater than 0!\n", load_awb_filename);
					fclose(fp);
					return -1;
				}
				correct_param[i*3] = data[0];
				correct_param[i*3+1] = data[1];
				correct_param[i*3+2] = data[2];
				i++;
			}
			fclose(fp);
			if (i < 3) {
				printf("Incomplete data in %s.\n", load_awb_filename);
				return -1;
			}
			break;
		default:
			printf("Unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

static int config_sensor_lens_info(char *sensor_name, image_sensor_param_t *sensor_param)
{
	struct amba_vin_source_info vin_src_info;
	sensor_label sensor_lb;

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_src_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error!");
		return -1;
	}

	switch (vin_src_info.sensor_id) {
	case SENSOR_OV2710:
       	sensor_lb = OV_2710;
		sensor_param->p_adj_param = &ov2710_adj_param;
		sensor_param->p_rgb2yuv = ov2710_rgb2yuv;
		sensor_param->p_chroma_scale = &ov2710_chroma_scale;
		sensor_param->p_awb_param = &ov2710_awb_param;
		sensor_param->p_50hz_lines = ov2710_50hz_lines;
		sensor_param->p_60hz_lines = ov2710_60hz_lines;
		sensor_param->p_tile_config = &ov2710_tile_config;
		sensor_param->p_ae_agc_dgain = ov2710_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = ov2710_ae_sht_dgain;
		sensor_param->p_dlight_range = ov2710_dlight;
		sensor_param->p_manual_LE = ov2710_manual_LE;
	      	sprintf(sensor_name, "ov2710");
		break;
	case SENSOR_OV9710:
		sensor_lb = OV_9715;
		sensor_param->p_adj_param = &ov9715_adj_param;
		sensor_param->p_rgb2yuv = ov9715_rgb2yuv;
		sensor_param->p_chroma_scale = &ov9715_chroma_scale;
		sensor_param->p_awb_param = &ov9715_awb_param;
		sensor_param->p_50hz_lines = ov9715_50hz_lines;
		sensor_param->p_60hz_lines = ov9715_60hz_lines;
		sensor_param->p_tile_config = &ov9715_tile_config;
		sensor_param->p_ae_agc_dgain = ov9715_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = ov9715_ae_sht_dgain;
		sensor_param->p_dlight_range = ov9715_dlight;
		sensor_param->p_manual_LE = ov9715_manual_LE;
		sprintf(sensor_name, "ov9715");
		break;
        case SENSOR_OV9718:
                sensor_lb = OV_9718;
                sensor_param->p_adj_param = &ov9718_adj_param;
                sensor_param->p_rgb2yuv = ov9718_rgb2yuv;
                sensor_param->p_chroma_scale = &ov9718_chroma_scale;
                sensor_param->p_awb_param = &ov9718_awb_param;
                sensor_param->p_50hz_lines = ov9718_50hz_lines;
                sensor_param->p_60hz_lines = ov9718_60hz_lines;
                sensor_param->p_tile_config = &ov9718_tile_config;
                sensor_param->p_ae_agc_dgain = ov9718_ae_agc_dgain;
                sensor_param->p_ae_sht_dgain = ov9718_ae_sht_dgain;
                sensor_param->p_dlight_range = ov9718_dlight;
                sensor_param->p_manual_LE = ov9718_manual_LE;
                sprintf(sensor_name, "ov9718");
                break;
	case SENSOR_OV5653:
		sensor_lb = OV_5653;
		sensor_param->p_adj_param = &ov5653_adj_param;
		sensor_param->p_rgb2yuv = ov5653_rgb2yuv;
		sensor_param->p_chroma_scale = &ov5653_chroma_scale;
		sensor_param->p_awb_param = &ov5653_awb_param;
		sensor_param->p_50hz_lines = ov5653_50hz_lines;
		sensor_param->p_60hz_lines = ov5653_60hz_lines;
		sensor_param->p_tile_config = &ov5653_tile_config;
		sensor_param->p_ae_agc_dgain = ov5653_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain =ov5653_ae_sht_dgain;
		sensor_param->p_dlight_range = ov5653_dlight;
		sensor_param->p_manual_LE = ov5653_manual_LE;
		sprintf(sensor_name, "ov5653");
		break;
	case SENSOR_OV14810:
		sensor_lb = OV_14810;
		sensor_param->p_adj_param = &ov14810_adj_param;
		sensor_param->p_rgb2yuv = ov14810_rgb2yuv;
		sensor_param->p_chroma_scale = &ov14810_chroma_scale;
		sensor_param->p_awb_param = &ov14810_awb_param;
		sensor_param->p_50hz_lines = ov14810_50hz_lines;
		sensor_param->p_60hz_lines = ov14810_60hz_lines;
		sensor_param->p_tile_config = &ov14810_tile_config;
		sensor_param->p_ae_agc_dgain = ov14810_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = ov14810_ae_sht_dgain;
		sensor_param->p_dlight_range = ov14810_dlight;
		sensor_param->p_manual_LE = ov14810_manual_LE;
		sprintf(sensor_name, "ov14810");
		break;
	case SENSOR_MT9J001:
		sensor_lb = MT_9J001;
		sprintf(sensor_name, "mt9j001");
		break;
	case SENSOR_MT9P001:
		sensor_lb = MT_9P031;
		sensor_param->p_adj_param = &mt9p006_adj_param;
		sensor_param->p_rgb2yuv = mt9p006_rgb2yuv;
		sensor_param->p_chroma_scale = &mt9p006_chroma_scale;
		sensor_param->p_awb_param = &mt9p006_awb_param;
		sensor_param->p_50hz_lines = mt9p006_50hz_lines;
		sensor_param->p_60hz_lines = mt9p006_60hz_lines;
		sensor_param->p_tile_config = &mt9p006_tile_config;
		sensor_param->p_ae_agc_dgain = mt9p006_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = mt9p006_ae_sht_dgain;
		sensor_param->p_dlight_range = mt9p006_dlight;
		sensor_param->p_manual_LE = mt9p006_manual_LE;
		sprintf(sensor_name, "mt9p006");
		break;
	case SENSOR_MT9M033:
		sensor_lb = MT_9M033;
		sensor_param->p_adj_param = &mt9m033_adj_param;
		sensor_param->p_rgb2yuv = mt9m033_rgb2yuv;
		sensor_param->p_chroma_scale = &mt9m033_chroma_scale;
		sensor_param->p_awb_param = &mt9m033_awb_param;
		sensor_param->p_50hz_lines = mt9m033_50hz_lines;
		sensor_param->p_60hz_lines = mt9m033_60hz_lines;
		sensor_param->p_tile_config = &mt9m033_tile_config;
		sensor_param->p_ae_agc_dgain = mt9m033_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = mt9m033_ae_sht_dgain;
		sensor_param->p_dlight_range = mt9m033_dlight;
		sensor_param->p_manual_LE = mt9m033_manual_LE;
		sprintf(sensor_name, "mt9m033");
		break;
	case SENSOR_MT9T002:
		sensor_lb = MT_9T002;
		sensor_param->p_adj_param = &mt9t002_adj_param;
		sensor_param->p_rgb2yuv = mt9t002_rgb2yuv;
		sensor_param->p_chroma_scale = &mt9t002_chroma_scale;
		sensor_param->p_awb_param = &mt9t002_awb_param;
		sensor_param->p_50hz_lines = mt9t002_50hz_lines;
		sensor_param->p_60hz_lines = mt9t002_60hz_lines;
		sensor_param->p_tile_config = &mt9t002_tile_config;
		sensor_param->p_ae_agc_dgain = mt9t002_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = mt9t002_ae_sht_dgain;
		sensor_param->p_dlight_range = mt9t002_dlight;
		sensor_param->p_manual_LE = mt9t002_manual_LE;
		sprintf(sensor_name, "mt9t002");
		break;
	case SENSOR_IMX035:
		sensor_lb = IMX_035;
		sensor_param->p_adj_param = &imx035_adj_param;
		sensor_param->p_rgb2yuv = imx035_rgb2yuv;
		sensor_param->p_chroma_scale = &imx035_chroma_scale;
		sensor_param->p_awb_param = &imx035_awb_param;
		sensor_param->p_50hz_lines = imx035_50hz_lines;
		sensor_param->p_60hz_lines = imx035_60hz_lines;
		sensor_param->p_tile_config = &imx035_tile_config;
		sensor_param->p_ae_agc_dgain = imx035_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = imx035_ae_sht_dgain;
		sensor_param->p_dlight_range = imx035_dlight;
		sensor_param->p_manual_LE = imx035_manual_LE;
		sprintf(sensor_name, "imx035");
		break;
	case SENSOR_IMX036:
		sensor_lb = IMX_036;
		sensor_param->p_adj_param = &imx036_adj_param;
		sensor_param->p_rgb2yuv = imx036_rgb2yuv;
		sensor_param->p_chroma_scale = &imx036_chroma_scale;
		sensor_param->p_awb_param = &imx036_awb_param;
		sensor_param->p_50hz_lines = imx036_50hz_lines;
		sensor_param->p_60hz_lines = imx036_60hz_lines;
		sensor_param->p_tile_config = &imx036_tile_config;
		sensor_param->p_ae_agc_dgain = imx036_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = imx036_ae_sht_dgain;
		sensor_param->p_dlight_range = imx036_dlight;
		sensor_param->p_manual_LE = imx036_manual_LE;
		sprintf(sensor_name, "imx036");
		break;
	case SENSOR_IMX072:
		sensor_lb = IMX_072;
		sensor_param->p_adj_param = &imx072_adj_param;
		sensor_param->p_rgb2yuv = imx072_rgb2yuv;
		sensor_param->p_chroma_scale = &imx072_chroma_scale;
		sensor_param->p_awb_param = &imx072_awb_param;
		sensor_param->p_50hz_lines = imx072_50hz_lines;
		sensor_param->p_60hz_lines = imx072_60hz_lines;
		sensor_param->p_tile_config = &imx072_tile_config;
		sensor_param->p_ae_agc_dgain = imx072_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = imx072_ae_sht_dgain;
		sensor_param->p_dlight_range = imx072_dlight;
		sensor_param->p_manual_LE = imx072_manual_LE;
		sprintf(sensor_name, "imx072");
		break;
	case SENSOR_IMX122:
		sensor_lb = IMX_122;
		sensor_param->p_adj_param = &imx122_adj_param;
		sensor_param->p_rgb2yuv = imx122_rgb2yuv;
		sensor_param->p_chroma_scale = &imx122_chroma_scale;
		sensor_param->p_awb_param = &imx122_awb_param;
		sensor_param->p_50hz_lines = imx122_50hz_lines;
		sensor_param->p_60hz_lines = imx122_60hz_lines;
		sensor_param->p_tile_config = &imx122_tile_config;
		sensor_param->p_ae_agc_dgain = imx122_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = imx122_ae_sht_dgain;
		sensor_param->p_dlight_range = imx122_dlight;
		sensor_param->p_manual_LE = imx122_manual_LE;
		sprintf(sensor_name, "imx122");
		break;
	case SENSOR_S5K5B3GX:
		sensor_lb = SAM_S5K5B3;
		sensor_param->p_adj_param = &s5k5b3gx_adj_param;
		sensor_param->p_rgb2yuv = s5k5b3gx_rgb2yuv;
		sensor_param->p_chroma_scale = &s5k5b3gx_chroma_scale;
		sensor_param->p_awb_param = &s5k5b3gx_awb_param;
		sensor_param->p_50hz_lines = s5k5b3gx_50hz_lines;
		sensor_param->p_60hz_lines = s5k5b3gx_60hz_lines;
		sensor_param->p_tile_config = &s5k5b3gx_tile_config;
		sensor_param->p_ae_agc_dgain = s5k5b3gx_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = s5k5b3gx_ae_sht_dgain;
		sensor_param->p_dlight_range = s5k5b3gx_dlight;
		sensor_param->p_manual_LE = s5k5b3gx_manual_LE;
		sprintf(sensor_name, "s5k5b3gx");
		break;
	case SENSOR_AR0331:
		sensor_lb = AR_0331;
		sensor_param->p_adj_param = &ar0331_adj_param;
		sensor_param->p_rgb2yuv = ar0331_rgb2yuv;
		sensor_param->p_chroma_scale = &ar0331_chroma_scale;
		sensor_param->p_awb_param = &ar0331_awb_param;
		sensor_param->p_50hz_lines = ar0331_50hz_lines;
		sensor_param->p_60hz_lines = ar0331_60hz_lines;
		sensor_param->p_tile_config = &ar0331_tile_config;
		sensor_param->p_ae_agc_dgain = ar0331_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = ar0331_ae_sht_dgain;
		sensor_param->p_dlight_range = ar0331_dlight;
		sensor_param->p_manual_LE = ar0331_manual_LE;
		sprintf(sensor_name, "ar0331");
		break;
	case SENSOR_AR0130:
		sensor_lb = AR_0130;
		sensor_param->p_adj_param = &ar0130_adj_param;
		sensor_param->p_rgb2yuv = ar0130_rgb2yuv;
		sensor_param->p_chroma_scale = &ar0130_chroma_scale;
		sensor_param->p_awb_param = &ar0130_awb_param;
		sensor_param->p_50hz_lines = ar0130_50hz_lines;
		sensor_param->p_60hz_lines = ar0130_60hz_lines;
		sensor_param->p_tile_config = &ar0130_tile_config;
		sensor_param->p_ae_agc_dgain = ar0130_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = ar0130_ae_sht_dgain_720p30;
		sensor_param->p_dlight_range = ar0130_dlight;
		sensor_param->p_manual_LE = ar0130_manual_LE;
		sprintf(sensor_name, "ar0130");
		break;
	case SENSOR_MN34041PL:
		sensor_lb = MN_34041PL;
		sensor_param->p_adj_param = &mn34041pl_adj_param;
		sensor_param->p_rgb2yuv = mn34041pl_rgb2yuv;
		sensor_param->p_chroma_scale = &mn34041pl_chroma_scale;
		sensor_param->p_awb_param = &mn34041pl_awb_param;
		sensor_param->p_50hz_lines = mn34041pl_50hz_lines;
		sensor_param->p_60hz_lines = mn34041pl_60hz_lines;
		sensor_param->p_tile_config = &mn34041pl_tile_config;
		sensor_param->p_ae_agc_dgain = mn34041pl_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = mn34041pl_ae_sht_dgain;
		sensor_param->p_dlight_range = mn34041pl_dlight;
		sensor_param->p_manual_LE = mn34041pl_manual_LE;
		sprintf(sensor_name, "mn34041pl");
		break;
	case SENSOR_IMX104:
		sensor_lb = IMX_104;
		sensor_param->p_adj_param = &imx104_adj_param;
		sensor_param->p_rgb2yuv = imx104_rgb2yuv;
		sensor_param->p_chroma_scale = &imx104_chroma_scale;
		sensor_param->p_awb_param = &imx104_awb_param;
		sensor_param->p_50hz_lines = imx104_50hz_lines;
		sensor_param->p_60hz_lines = imx104_60hz_lines;
		sensor_param->p_tile_config = &imx104_tile_config;
		sensor_param->p_ae_agc_dgain = imx104_ae_agc_dgain;
		sensor_param->p_ae_sht_dgain = imx104_ae_sht_dgain;
		sensor_param->p_dlight_range = imx104_dlight;
		sensor_param->p_manual_LE = imx104_manual_LE;
		sprintf(sensor_name, "imx104");
		break;
	default:
		printf("Unknow sensor id: %d \n", vin_src_info.sensor_id);
		return -1;
	}
	if (img_config_sensor_info(sensor_lb) < 0) {
		printf("img_config_sensor_info error!\n");
		return -1;
	}
	if (img_config_lens_info(LENS_CMOUNT_ID) < 0) {
		printf("img_config_lens_info error!\n");
		return -1;
	}

	return 0;
}

static int load_dsp_cc_table(void)
{
	u8 reg[REG_SIZE], matrix[MATRIX_SIZE];
	color_correction_t color_corr;
	color_correction_reg_t color_corr_reg;
	char filename[128];
	int file, count;

	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if ((file = open("/system/ambarella/reg.bin", O_RDONLY, 0)) < 0) {
			printf("Open reg.bin error!\n");
			return -1;
		}
	}
	if ((count = read(file, reg, REG_SIZE)) != REG_SIZE) {
		printf("Read reg.bin error.\n");
		close(file);
		return -1;
	}
	close(file);

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if ((file = open("/system/ambarella/3D.bin", O_RDONLY, 0)) < 0) {
			printf("Open 3D.bin error!\n");
			return -1;
		}
	}
	if ((count = read(file, matrix, MATRIX_SIZE)) != MATRIX_SIZE) {
		printf("Read 3D.bin error!\n");
		close(file);
		return -1;
	}
	close(file);

	color_corr_reg.reg_setting_addr=(u32)reg;
	color_corr.matrix_3d_table_addr =(u32)matrix;
	if (img_dsp_set_color_correction_reg(&color_corr_reg) < 0) {
		printf("img_dsp_set_color_correction_reg error!\n");
		return -1;
	}
	if (img_dsp_set_color_correction(fd_iav,&color_corr) < 0) {
		printf("img_dsp_set_color_correction error!\n");
		return -1;
	}

	return 0;
}

static int load_adj_cc_table(char *sensor_name)
{
	int file, count;
	char filename[128];
	u8 matrix[MATRIX_SIZE];
	u8 i, adj_mode = 4;

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename, "%s/sensors/%s_0%d_3D.bin",
			IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("Open %s error!\n", filename);
			return -1;
		}
		if ((count = read(file, matrix, MATRIX_SIZE)) != MATRIX_SIZE) {
			printf("Read %s error!\n", filename);
			close(file);
			return -1;
		}
		close(file);

		if (img_adj_load_cc_table((u32)matrix, i) < 0) {
			printf("img_ad_load_cc_table error!\n");
			return -1;
		}
	}

	return 0;
}

static int config_aaa(void)
{
	char sensor_name[32];
	image_sensor_param_t sensor_param;
	aaa_api_t custom_aaa_api;
	memset(&sensor_param, 0, sizeof(image_sensor_param_t));
	memset(&custom_aaa_api, 0, sizeof(aaa_api_t));

	if (img_lib_init(0,0) < 0) {
		perror("img_lib_init error!\n");
		return -1;
	}

	if (config_sensor_lens_info(sensor_name, &sensor_param) < 0)
		return -1;

	if (load_dsp_cc_table() < 0)
		return -1;

	if (load_adj_cc_table(sensor_name) < 0)
		return -1;

	if (img_load_image_sensor_param(&sensor_param) < 0) {
		printf("img_load_image_sensor_param error!\n");
		return -1;
	}

	if (img_register_aaa_algorithm(custom_aaa_api) < 0) {
		printf("img_register_aaa_algorithm error!\n");
		return -1;
	}
	return 0;
}

static int start_aaa(void)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (config_aaa() < 0)
		return -1;

	if (img_start_aaa(fd_iav) < 0) {
		printf("img_start_aaa error!\n");
		return -1;
	}
	sleep(1);
	return 0;
}

int awb_cali_get_current_gain(void)
{
	if (img_awb_set_method(AWB_CUSTOM) < 0) {
		printf("img_awb_set_method error!\n");
		return -1;
	}
	printf("\nWait to get current White Balance Gain...\n");
	sleep(2);

	img_awb_get_wb_cal(&wb_gain);
	printf("Current Red Gain %d, Green Gain %d, Blue Gain %d.\n",
		wb_gain.r_gain, wb_gain.g_gain, wb_gain.b_gain);

	if (img_awb_set_method(AWB_NORMAL) < 0) {
		printf("img_awb_set_method error!\n");
		return -1;
	}

	return 0;
}

int awb_cali_correct_gain(void)
{
	wb_gain_t orig[2], target[2];
	int thre_r, thre_b;
	int low_r, low_b, high_r, high_b;
	int i;

	for (i = 0; i < 2; i++) {
		orig[i].r_gain = correct_param[i*3];
		orig[i].g_gain = correct_param[i*3+1];
		orig[i].b_gain = correct_param[i*3+2];
	}

	for (i = 0; i < 2; i++) {
		target[i].r_gain = correct_param[i*3+6];
		target[i].g_gain = correct_param[i*3+7];
		target[i].b_gain = correct_param[i*3+8];
	}

	low_r = target[0].r_gain - orig[0].r_gain;
	low_r = ABS(low_r);
	high_r = target[1].r_gain - orig[1].r_gain;
	high_r = ABS(high_r);
	low_b = target[0].b_gain - orig[0].b_gain;
	low_b = ABS(low_b);
	high_b = target[1].b_gain-orig[1].b_gain;
	high_b = ABS(high_b);

	thre_r = MAX(low_r, high_r);
	thre_b = MAX(low_b, high_b);

	if (img_awb_set_cali_diff_thr(thre_r, thre_b) < 0) {
		printf("img_awb_set_cali_diff_thr error!\n");
		return -1;
	}
	if (img_awb_set_wb_shift(orig, target) < 0) {
		printf("img_awb_set_wb_shift error!\n");
		return -1;
	}

	return 0;
}


int main(int argc, char **argv)
{
	FILE *fp;

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (start_aaa() < 0)
		return -1;

	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	sleep(1);
	if (detect_flag) {
		if (awb_cali_get_current_gain() < 0)
			return -1;
		if (save_file_flag) {
			if ((fp = fopen(cali_awb_filename, "a+")) == NULL) {
				printf("Open file %s error. Save the result to default file %s.\n", cali_awb_filename, default_filename);

				if ((fp = fopen(default_filename, "a+")) == NULL) {
					printf("Save failed.\n");
				}
			}
			if (fp != NULL) {
				fprintf(fp, "%d:%d:%d\n", wb_gain.r_gain, wb_gain.g_gain, wb_gain.b_gain);
				fclose(fp);
			}
		}
	}

	if (correct_flag) {
		if (awb_cali_correct_gain() < 0)
			return -1;
	}



	if (correct_flag || restore_flag)
		while (1)
			sleep(10);

	return 0;
}


