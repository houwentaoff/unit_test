#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include <sched.h>
#include <signal.h>
#include "types.h"

#include "iav_drv.h"
#include "iav_drv_ex.h"

#include "ambas_imgproc_arch.h"
#ifdef BUILD_AMBARELLA_EIS
#include "ambas_eis.h"
#endif
#include "ambas_vin.h"
#include "img_struct.h"
#include "img_dsp_interface.h"
#include "img_api.h"

#include "adj_params/imx036_adj_param.c"
#include "adj_params/imx036_aeb_param.c"

#include "adj_params/imx035_adj_param.c"
#include "adj_params/imx035_aeb_param.c"

#include "adj_params/ov2710_adj_param.c"
#include "adj_params/ov2710_aeb_param.c"

#include "adj_params/ov9715_adj_param.c"
#include "adj_params/ov9715_aeb_param.c"


#include "adj_params/ov5653_adj_param.c"
#include "adj_params/ov5653_aeb_param.c"

#include "adj_params/imx072_adj_param.c"
#include "adj_params/imx072_aeb_param.c"

#include "adj_params/imx122_adj_param.c"
#include "adj_params/imx122_aeb_param.c"

#include "adj_params/ov14810_adj_param.c"
#include "adj_params/ov14810_aeb_param.c"

#include "adj_params/mt9m033_adj_param.c"
#include "adj_params/mt9m033_aeb_param.c"

#include "adj_params/mt9t002_adj_param.c"
#include "adj_params/mt9t002_aeb_param.c"

#include "adj_params/s5k5b3gx_adj_param.c"
#include "adj_params/s5k5b3gx_aeb_param.c"

#include "adj_params/mt9p006_adj_param.c"
#include "adj_params/mt9p006_aeb_param.c"

#include "adj_params/ar0331_adj_param.c"
#include "adj_params/ar0331_aeb_param.c"

#include "adj_params/ar0130_adj_param.c"
#include "adj_params/ar0130_aeb_param.c"

#include "adj_params/mn34041pl_adj_param.c"
#include "adj_params/mn34041pl_aeb_param.c"

#include "adj_params/imx104_adj_param.c"
#include "adj_params/imx104_aeb_param.c"

#define	IMGPROC_PARAM_PATH	"/etc/idsp"
#define	YUV_EXTRA_BRIGHT		20
#define 	HDR_OP_MODE			21

#undef TUNING_ON
#define TUNING_printf(...)
#define NO_ARG	0
#define HAS_ARG	1
#define UNIT_chroma_scale (64)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

static const char* short_options = "cn:l:m:s:t:A:S:ap:z:t:d:L:C:E:w:M:H:P:y:f:F:Bb:Vv:i:q:R:1:2:3:4:56:7:8:D:9gx:";
static struct option long_options[] = {
	{"config", NO_ARG, 0, 'c'},
	{"noise-filter-setup", HAS_ARG, 0, 'n'},
	{"blc", HAS_ARG, 0, 'l'},
	{"cc", HAS_ARG, 0, 'm'},
	{"chroma_scale", HAS_ARG, 0, 's'},
	{"mctf", HAS_ARG, 0, 't'},
	{"agc", HAS_ARG, 0, 'A'},
	{"shutter", HAS_ARG, 0, 'S'},
	{"3a", NO_ARG, 0, 'a'},
	{"sharpeness", HAS_ARG, 0, 'p'},
	{"zoom_factor", HAS_ARG, 0, 'z'},
	{"mctf_factor", HAS_ARG, 0, 't'},
	{"dbp", HAS_ARG, 0, 'd'},
	{"anti_aliasing", HAS_ARG, 0, 'L'},
	{"cfa_filter", HAS_ARG, 0, 'C'},
	{"wb", HAS_ARG, 0, 'w'},
	{"chroma_median_filter", HAS_ARG, 0, 'M'},
	{"local_exposure", HAS_ARG, 0, 'P'},
	{"high_freq_noise_reduct", HAS_ARG, 0, 'H'},
	{"rgb2yuv", HAS_ARG, 0, 'y'},
	{"cfa_leakage", HAS_ARG, 0, 'f'},
	{"spatial_filter", HAS_ARG, 0, 'F'},
	{"sharpening_fir", NO_ARG, 0, 'B'},
	{"coring_table", HAS_ARG, 0, 'b'},
	{"vignette_compensation", HAS_ARG, 0, 'V'},
	{"max_change", HAS_ARG, 0, 'v'},
	{"cap", HAS_ARG, 0, 'i'},
	{"mqueue", HAS_ARG, 0, 'q'},
	{"raw-proc",HAS_ARG,0,'R'},
	{"fpn",HAS_ARG,0,1},
	{"loadfpn",HAS_ARG,0,2},
	{"hdr",HAS_ARG,0,3},
	{"eis",HAS_ARG,0,4},
	{"eislog",NO_ARG,0,5},
	{"gyrocal",HAS_ARG,0,6},
	{"lens",HAS_ARG,0,7},
	{"wb_cali_org",NO_ARG,0,9},
	{"wb_shift",HAS_ARG,0,8},
	{"wb_cali_thr",HAS_ARG,0,'D'},
	{"vig_to_awb",NO_ARG,0,'g'},
	{"awb_speed",HAS_ARG,0,'x'},
	{"extra_bright",HAS_ARG,0,YUV_EXTRA_BRIGHT},
	{"hdr_op_mode",HAS_ARG,0,HDR_OP_MODE},
	{0, 0, 0, 0},
};
u8 hdr_operation_mode_flg = 0;
static u8 hdr_operation_mode = 0;
u8 yuv_extra_bright_flg = 0;
static u8 yuv_extra_bright_enable = 0;
u8 awb_speed_flag;
static u8 awb_speed;
u8 fpn_flag;
u8 load_fpn_flag;
u8 hdr_flag;
u8 still_cap;
u8 eis_flag;
u8 eis_log_flag = 0;
u8 gyro_calib_flag = 0;
u8 wb_cali_org_flag = 0;
u8 wb_shift_flg = 0;
u8 wb_cali_thr_flg = 0;
static int wb_shift_param[12];
wb_gain_t org_gain_cali[2];
static wb_gain_t wb_cali_org[2],wb_cali_ref[2];
static u16 wb_cali_thr_r;
static u16 wb_cali_thr_b;
static int wb_cali_thr_param[2];
char pic_file_name[64];
u8 config_flag, mctf_flag, start_aaa;
u8 set_noise_filter_flag, chroma_scale_flag;
u8 blc_flag,cfa_filter_flag,cfa_filter_strength;
u8 cc_flag,zoom_flag,anti_aliasing_flag,anti_aliasing_strength;
float zoom_factor;
u8 blue_flag, green_flag, red_flag, agc_flag, shutter_flag;
u32 noise_filter,dbp_flag;
u32 r_gain, b_gain, g_gain;
u32 chroma_scale;
u32 sharpeness_flag,sharpeness_strength;
u32 shutter_index, agc_index;
u8 local_exposure_flag;
u8 cc_enable = 0;
u16 wb_flag,chroma_median_flag,high_freq_flag,rgb2yuv_flag;
u16 cfa_leakage_flag,spatial_filter_flag, sharpening_fir_flag,coring_table_flag;
u16 max_change_flag,vignette_compensation_flag,cal_lens_shading,flicker_mode;
u8 vig_to_awb_flag;
u16 msg_send_flag;
int msg_param;
u8 raw_proc_flag;
static u8 eis_enable;
static u8 gyro_calib_mode;
static cali_badpix_setup_t badpixel_detect_algo;
static int fpn_param[10];
static int load_fpn_param[2];
static fpn_correction_t fpn;
//static still_proc_mem_init_info_t proc_init_info;
//static still_proc_mem_info_t proc_info;
static blc_level_t blc;
static s32 black_correct_param[4];
static cfa_noise_filter_info_t cfa_noise_filter;
static int cfa_noise_filter_param[5];
static dbp_correction_t bad_corr;
static int bad_corr_param[3];
static wb_gain_t wb_gain;
static int wb_gain_param[3];
static int dgain;
static chroma_median_filter_t chroma_median_setup;
static int chroma_median_param[3];
u8 coring_strength;
static video_mctf_info_t mctf_info;
static u8 luma_high_freq_noise_reduction_strength;
static int mctf_param[4];
static rgb_to_yuv_t rgb2yuv_matrix;
static int rgb2yuv_param[9];
static int spatial_filter_param[3];
static sharpen_level_t sharpen_setting;
static image_sensor_param_t app_param_image_sensor;
static int eis_param[5];
static u8 eis_eis_str_x,eis_eis_str_y,eis_rsc_str_x,eis_rsc_str_y;
#ifdef TUNING_ON
static int predefined_param;
static int sharpen_setting_param[7];
static int fir_param[11];
static int coring_param[257];
static int chroma_scale_param[129];
static int local_exposure_param[262];
#endif

static cfa_leakage_filter_t cfa_leakage_filter;
static max_change_t max_change;
static int max_change_param[2];
static spatial_filter_t spatial_filter_info;
static fir_t fir = {-1, {0, -1, 0, 0, 0, 8, 0, 0, 0, 0}};
static u8 retain_level = 1;
static vignette_info_t vignette_info = {0};
static int vin_op_mode = AMBA_VIN_LINEAR_MODE;	//VIN is linear or HDR
u8 *bsb_mem;
u32 bsb_size;

extern int convert_vig_table_to_awb_table(u16* vignette_table,u16* vig_awb_gain_table,const u16 raw_width,const u16 raw_height,
											const u16 awb_tile_num_col, const u16 awb_tile_num_row);

static local_exposure_t local_exposure = { 1, 4, 16, 16, 16, 6,
	{1024,1054,1140,1320,1460,1538,1580,1610,1630,1640,1640,1635,1625,1610,1592,1563,
	1534,1505,1475,1447,1417,1393,1369,1345,1321,1297,1273,1256,1238,1226,1214,1203,
	1192,1180,1168,1157,1149,1142,1135,1127,1121,1113,1106,1098,1091,1084,1080,1077,
	1074,1071,1067,1065,1061,1058,1055,1051,1048,1045,1044,1043,1042,1041,1040,1039,
	1038,1037,1036,1035,1034,1033,1032,1031,1030,1029,1029,1029,1029,1028,1028,1028,
	1028,1027,1027,1027,1026,1026,1026,1026,1025,1025,1025,1025,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024}};
tone_curve_t tone_curve = {
		{/* red */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* green */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* blue */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023}
	};
static coring_table_t coring;
static chroma_scale_filter_t cs;

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\t\tnothing"},
	{"0", "\tidsp filter enable"},
	{"value", "\t\tblack level"},
	{"value", "\t\t\tload color correction"},
	{"64 unit", "\tchroma scale"},
	{"enable, alpha, threshold1, threshold2", "\tmctf filter"},
	{"value", "\t\tsensor gain index"},
	{"value", "\t\tsensor e-shutter speed index"},
	{"0~6", "\t\t\tturn 3a on"},
	{"value", "\t\tconfig for sharpeness"},
	{"0~10", "\t\tzoom factor"},
	{"enable, alpha, threshold1, threshold2", "\tmctf filter"},
	{"value", "\t\tdynamic bad pixel strength"},
	{"0~255", "\tanti aliasing filter strength"},
	{"enable, weight, weight, weight, threshold", "\tcfa filter noise filter"},
	{"r gain, b gain, d gain", "white balance"},
	{"0~255", "chroma median filter strength"},
	{"0|1", "\tlocal exposure enable"},
	{"0~255", "high frequncy noise reduction filter strength"},
	{"value", "\t\tload rgb2yuv matrix"},
	{"value", "\tcfa leakage filter"},
	{"threshold, dirctional, isotropic", "\tspatial noise filter"},
	{"", "\t\tconfig sharpening fir"},
	{"unavailable", "\tload coring table"},
	{"unavailable", "\t\tconfig vignette compensation"},
	{"0~255", "\t\tmax change"},
	{"", "\t\t\tstill capture"},
	{"", "\t\t\tmessage queue"},
	{"raw file name","\traw processing from memory"},
	{"","\t\t\tstatic bad pixel maping"},
	{"","\t\t\tload bad pixel data"},
	{"","\t\t\tcalculate original wb gain for wb calibration"},
	{0, 0},
};

void usage(void){
	int i;

	printf("test_idsp usage:\n");
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

static void sigstop()
{
	img_stop_aaa();
	printf("3A is off.\n");
	exit(1);
}

#ifdef TUNING_ON
static int get_param(char* keyword, char* param_buffer, int* argarr, int argcnt){
	char* ptr = NULL;
	char item[16];
	int i,item_length = 0;
	memset(item,'\0',16);
	ptr = strstr(param_buffer,keyword);
	if(ptr == NULL){
		printf("NOT found keywords: %s\n",keyword);
		return -1;
	}
	ptr = ptr + strlen(keyword)+1; // point to the first number
	for(i = 0; i<argcnt; i++ ){
		for(;;ptr++){
			if (((*ptr)<0x30)||((*ptr)>0x39)){ // not a number,over
				break;
			}
			item_length++;
		}
		memcpy(item,ptr-item_length,item_length);
		argarr[i] = atoi(item);
		memset(item,'\0',16);
		item_length = 0;
		ptr += 1;
	}
	return 0;
}
#endif

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
int fd_iav;
static inline int get_vin_mode(u32* vin_mode)
{
	
	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_MODE, vin_mode)) 
	{
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_MODE");
		return -1;
	}
//	printf("vin_mode %d\n",*vin_mode);
	return 0;
}
static inline int get_vin_frame_rate(u32 *pFrame_time)
{
	int rval;
	if ((rval = ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, pFrame_time)) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return rval;
	}
//	printf("frame_rate %d\n",*pFrame_time);
	return 0;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int i;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case	'c':
			config_flag = 1;
			break;
		case	'n':
			set_noise_filter_flag = 1;
			noise_filter = atoi(optarg);
			break;
		case 'x':
			awb_speed_flag = 1;
			awb_speed = atoi(optarg);
			break;
		case	'l':
			blc_flag = 1;
			if (get_multi_arg(optarg, black_correct_param, ARRAY_SIZE(black_correct_param)) < 0) {
				printf("need %d args for opt %c\n", ARRAY_SIZE(black_correct_param), ch);
				break;
			}
			blc.r_offset = -black_correct_param[0];
			blc.gb_offset = -black_correct_param[1];
			blc.gr_offset = -black_correct_param[2];
			blc.b_offset = -black_correct_param[3];
			break;
		case	'm':
			cc_flag = 1;
			cc_enable = atoi(optarg);
			break;
		case	'w':
			wb_flag = 1;
			if (get_multi_arg(optarg, wb_gain_param, ARRAY_SIZE(wb_gain_param)) < 0) {
				printf("need %d args for opt %c\n", ARRAY_SIZE(wb_gain_param), ch);
				break;
			}
			wb_gain.r_gain = wb_gain_param[0];
			wb_gain.g_gain = 1024;
			wb_gain.b_gain = wb_gain_param[1];
			dgain = wb_gain_param[2];
			break;
		case	's':
/*			if (get_multi_arg(optarg, chroma_scale_param, ARRAY_SIZE(chroma_scale_param)) < 0) {
				printf("need %d args for opt %c\n", ARRAY_SIZE(chroma_scale_param), ch);
				break;
			}
*/			chroma_scale_flag = 1;
			chroma_scale = atoi(optarg);
			break;
		case	't':
			mctf_flag = 1;
			if (get_multi_arg(optarg, mctf_param, ARRAY_SIZE(mctf_param)) < 0) {
				printf("need %d args for opt %c\n", ARRAY_SIZE(mctf_param), ch);
				break;
			}
			mctf_info.enable = mctf_param[0];
			mctf_info.alpha = mctf_param[1];
			mctf_info.threshold_1 = mctf_param[2];
			mctf_info.threshold_2 = mctf_param[3];
			mctf_info.y_max_change = 255;
			mctf_info.u_max_change = 255;
			mctf_info.v_max_change = 255;
			mctf_info.padding = 0;
			break;
		case	'a':
			start_aaa = 1;
//			predefined_param = atoi(optarg);
			break;
		case	'A':
			agc_flag = 1;
			agc_index= atoi(optarg);
			break;
		case	'S':
			shutter_flag = 1;
			shutter_index = atoi(optarg);
			break;
		case	'p':
			sharpeness_flag = 1;
			sharpeness_strength = atoi(optarg);
			break;
		case	'z':
			zoom_flag = 1;
			zoom_factor = atof(optarg)*65536;
			break;
		case	'd':
			dbp_flag = 1;
			if (get_multi_arg(optarg, bad_corr_param, ARRAY_SIZE(bad_corr_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(bad_corr_param), ch);
				break;
			}
			bad_corr.enable = bad_corr_param[0];
			bad_corr.dark_pixel_strength = bad_corr_param[1];
			bad_corr.hot_pixel_strength = bad_corr_param[2];
			break;
		case	'L':
			anti_aliasing_flag = 1;
			anti_aliasing_strength = atoi(optarg);
			break;
		case	'C':
			cfa_filter_flag = 1;
			if (get_multi_arg(optarg, cfa_noise_filter_param, ARRAY_SIZE(cfa_noise_filter_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(cfa_noise_filter_param), ch);
				break;
			}
			cfa_noise_filter.enable = cfa_noise_filter_param[0];
			cfa_noise_filter.direct_center_weight_red = cfa_noise_filter_param[1];
			cfa_noise_filter.direct_center_weight_green = cfa_noise_filter_param[2];
			cfa_noise_filter.direct_center_weight_blue = cfa_noise_filter_param[3];
			cfa_noise_filter.direct_thresh_k0_red = cfa_noise_filter_param[4];
			cfa_noise_filter.direct_thresh_k0_green = cfa_noise_filter_param[4];
			cfa_noise_filter.direct_thresh_k0_blue = cfa_noise_filter_param[4];
			break;
		case	'P':
			local_exposure_flag = 1;
			local_exposure.enable = atoi(optarg);
			break;
		case	'M':
			if (get_multi_arg(optarg, chroma_median_param, ARRAY_SIZE(chroma_median_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(chroma_median_param), ch);
				break;
			}
			if((chroma_median_param[1]<256)&&(chroma_median_param[2]<256)){
				chroma_median_setup.enable = chroma_median_param[0];
				chroma_median_setup.cb_str = chroma_median_param[1];
				chroma_median_setup.cr_str = chroma_median_param[2];
				chroma_median_flag = 1;
			}else{
				printf("chroma_median_strength exceed 256!\n");
				return -1;
			}
			break;
		case	'H':
			high_freq_flag = 1;
			luma_high_freq_noise_reduction_strength = atoi(optarg);
			break;
		case	'y':
			rgb2yuv_flag = 1;
			if (get_multi_arg(optarg, rgb2yuv_param, ARRAY_SIZE(rgb2yuv_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(rgb2yuv_param), ch);
				break;
			}
			for(i = 0;i<9;i++)
				rgb2yuv_matrix.matrix_values[i] = rgb2yuv_param[i];
			break;
		case	'f':
			cfa_leakage_flag = 1;
			cfa_leakage_filter.enable = atoi(optarg);
			break;
		case	'F':
			spatial_filter_flag = 1;
			if (get_multi_arg(optarg, spatial_filter_param, ARRAY_SIZE(spatial_filter_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(spatial_filter_param), ch);
				break;
			}
			spatial_filter_info.edge_threshold = spatial_filter_param[0];
			spatial_filter_info.directional_strength = spatial_filter_param[1];
			spatial_filter_info.isotropic_strength = spatial_filter_param[2];
			break;
		case 	'B':
			sharpening_fir_flag = 1;
			break;
		case	'b':
			coring_table_flag = 1;
			coring_strength = atoi(optarg);
			break;
		case	'v':
			max_change_flag = 1;
			if (get_multi_arg(optarg, max_change_param, ARRAY_SIZE(max_change_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(max_change_param), ch);
				break;
			}
			max_change.max_change_up = max_change_param[0];
			max_change.max_change_down = max_change_param[1];
			break;
		case	'V':
			vignette_compensation_flag = 1;
			break;
		case	'i':
			still_cap = 1;
			strcpy(pic_file_name, optarg);
			break;
		case	'q':
			msg_send_flag = 1;
			msg_param = atoi(optarg);
			break;
		case	'R':
			raw_proc_flag = 1;
			strcpy(pic_file_name, optarg);
			break;
		case	1:
			fpn_flag = 1;
			if (get_multi_arg(optarg, fpn_param, ARRAY_SIZE(fpn_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(fpn_param), ch);
				break;
			}
			badpixel_detect_algo.cap_width = fpn_param[0];
			badpixel_detect_algo.cap_height= fpn_param[1];
			badpixel_detect_algo.block_w= fpn_param[2];
			badpixel_detect_algo.block_h = fpn_param[3];
			badpixel_detect_algo.badpix_type = fpn_param[4];
			badpixel_detect_algo.upper_thres= fpn_param[5];
			badpixel_detect_algo.lower_thres= fpn_param[6];
			badpixel_detect_algo.detect_times = fpn_param[7];
			badpixel_detect_algo.agc_idx = fpn_param[8];
			badpixel_detect_algo.shutter_idx= fpn_param[9];
			break;
		case	2:
			load_fpn_flag = 1;
			if (get_multi_arg(optarg, load_fpn_param, ARRAY_SIZE(load_fpn_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(load_fpn_param), ch);
				break;
			}
			break;
		case	3:
			hdr_flag = 1;
			strcpy(pic_file_name, optarg);
			break;
		case	4:
			eis_flag = 1;
			if (get_multi_arg(optarg, eis_param, ARRAY_SIZE(eis_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(eis_param), ch);
				break;
			}
			eis_enable = eis_param[0];
			eis_eis_str_x = eis_param[1];
			eis_eis_str_y = eis_param[2];
			eis_rsc_str_x = eis_param[3];
			eis_rsc_str_y = eis_param[4];
			break;
		case	5:
			eis_log_flag = 1;
			break;
		case	6:
			gyro_calib_flag = 1;
			gyro_calib_mode = atoi(optarg);
			break;
		case	7:
			cal_lens_shading = 1;
			flicker_mode = atoi(optarg);
			break;
		case 8:
			wb_shift_flg = 1;
			if (get_multi_arg(optarg, wb_shift_param, ARRAY_SIZE(wb_shift_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(wb_shift_param), ch);
				break;
			}
			wb_cali_org[0].r_gain = wb_shift_param[0];
			wb_cali_org[0].g_gain = wb_shift_param[1];
			wb_cali_org[0].b_gain = wb_shift_param[2];

			wb_cali_org[1].r_gain = wb_shift_param[3];
			wb_cali_org[1].g_gain = wb_shift_param[4];
			wb_cali_org[1].b_gain = wb_shift_param[5];

			wb_cali_ref[0].r_gain = wb_shift_param[6];
			wb_cali_ref[0].g_gain = wb_shift_param[7];
			wb_cali_ref[0].b_gain = wb_shift_param[8];

			wb_cali_ref[1].r_gain = wb_shift_param[9];
			wb_cali_ref[1].g_gain = wb_shift_param[10];
			wb_cali_ref[1].b_gain = wb_shift_param[11];
			break;
		case 9:
			wb_cali_org_flag = 1;
			break;
		case 'D':
			wb_cali_thr_flg = 1;
			if (get_multi_arg(optarg, wb_cali_thr_param, ARRAY_SIZE(wb_cali_thr_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(wb_cali_thr_param), ch);
				break;
			}
			wb_cali_thr_r = wb_cali_thr_param[0];
			wb_cali_thr_b = wb_cali_thr_param[1];
			break;
		case 'g':
			vig_to_awb_flag = 1;
			break;
		case YUV_EXTRA_BRIGHT:
			yuv_extra_bright_flg = 1;
			yuv_extra_bright_enable = atoi(optarg);
			break;
		case HDR_OP_MODE:
			hdr_operation_mode_flg = 1;
			hdr_operation_mode = atoi(optarg);
			break;
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}
	return 0;
}


u16 gain_curve[NUM_CHROMA_GAIN_CURVE] = {256, 299, 342, 385, 428, 471, 514, 557, 600, 643, 686, 729, 772, 815, 858, 901,
	 936, 970, 990,1012,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1012, 996, 956, 916, 856, 796, 736, 676, 616, 556, 496, 436, 376, 316, 256};

static int save_raw(void){
	int 	 fd_raw;
//	iav_mmap_info_t			mmap_info;
	iav_raw_info_t		raw_info;

/*	rval = ioctl(fd_iav, IAV_IOC_MAP_DSP, &mmap_info);
	if (rval < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}*/

	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info) < 0) {
		perror("IAV_IOC_READ_RAW_INFO");
		return -1;
	}

	printf("raw_addr = %p\n", raw_info.raw_addr);
//	printf("bayer_pattern = %d [%s]\n", raw_info.bayer_pattern, get_bayer_pattern_str(raw_info.bayer_pattern));
	printf("bit_resolution = %d\n", raw_info.bit_resolution);
	printf("resolution: %dx%d\n", raw_info.width, raw_info.height);

	sprintf(pic_file_name, "%s.raw", pic_file_name);
	fd_raw = open(pic_file_name, O_WRONLY | O_CREAT, 0666);
	if (write(fd_raw, raw_info.raw_addr, raw_info.width * raw_info.height * 2) < 0) {
		perror("write(5)");
		return -1;
	}

	printf("raw picture written to %s\n", pic_file_name);
	close(fd_raw);
	return 0;
}

#if 1
static int save_jpeg_stream(void)
{
	int				rval = 0, fd_jpeg, i;
//	iav_mmap_info_t			mmap_info;
	bs_fifo_info_t			bs_info;


	/* map bsb buffer to user space */
/*	rval = ioctl(fd_iav, IAV_IOC_MAP_BSB, &mmap_info);
	if (rval < 0) {
		perror("IAV_IOC_MAP_BSB");
		goto save_jpeg_stream_exit;
	}
*/
	/* Read bit stream descriptiors */
	rval = ioctl(fd_iav, IAV_IOC_READ_BITSTREAM_EX, &bs_info);
	if (rval < 0) {
		perror("IAV_IOC_READ_BITSTREAM");
		goto save_jpeg_stream_exit;
	}

	if (bs_info.count) {
		printf("Read bitstream #: %d\n", bs_info.count);
	} else {
		printf("No bitstream available!\n");
		goto save_jpeg_stream_exit;
	}

	/* Write jpeg stream into file */
	sprintf(pic_file_name, "%s.jpeg", pic_file_name);
	fd_jpeg = open(pic_file_name, O_WRONLY | O_CREAT, 0666);
	if (fd_jpeg < 0) {
		printf("Error opening file %s!\n", pic_file_name);
		goto save_jpeg_stream_exit;
	}

	for (i = 0; i < bs_info.count; i++) {
		u32			start_addr, pic_size, bsb_end;

		pic_size   = (bs_info.desc[i].pic_size + 31) & ~31;
		start_addr = bs_info.desc[i].start_addr;
		bsb_end    = (u32)bsb_mem+ bsb_size;

		if (start_addr + pic_size <= bsb_end) {
			rval = write(fd_jpeg, (void *)start_addr, pic_size);
			if (rval < 0) {
				perror("WRITE FILE");
				goto save_jpeg_stream_exit;
			}
		} else {
			u32		size1, size2;

			size1 = bsb_end - start_addr;
			size2 = pic_size - size1;

			rval = write(fd_jpeg, (void *)start_addr, size1);
			if (rval < 0) {
				perror("WRITE FILE 1");
				goto save_jpeg_stream_exit;
			}

			rval = write(fd_jpeg, (void *)bsb_mem, size2);
			if (rval < 0) {
				perror("WRITE FILE 2");
				goto save_jpeg_stream_exit;
			}
		}
	}

	close(fd_jpeg);

save_jpeg_stream_exit:
	return rval;
}
#endif

int map_bsb(int fd_iav)
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

int load_dsp_cc_table(void)
{
	u8 reg[18752], matrix[17536];
	char filename[128];
	int file, count;

	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if ((file = open("reg.bin", O_RDONLY, 0)) < 0) {
			printf("reg.bin cannot be opened\n");
			return -1;
		}
	}
	if((count = read(file, reg, 18752)) != 18752) {
		printf("read reg.bin error\n");
		return -1;
	}
	close(file);

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		if((file = open("3D.bin", O_RDONLY, 0)) < 0) {
			printf("reg.bin cannot be opened\n");
			return -1;
		}
	}
	if((count = read(file, matrix, 17536)) != 17536) {
		printf("read 3D.bin error\n");
		return -1;
	}
	close(file);
	img_dsp_load_color_correction_table((u32)reg, (u32)matrix);

	return 0;
}

int load_adj_cc_table(char * sensor_name)
{
	int file, count;
	char filename[128];
	u8 matrix[17536];
	u8 i, adj_mode = 4;

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin",
			IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("can not open 3D.bin\n");
			return -1;
		}
		if((count = read(file, matrix, 17536)) != 17536) {
			printf("read imx036_01_3D.bin error\n");
			return -1;
		}
		close(file);
		img_adj_load_cc_table((u32)matrix, i);
	}

	return 0;
}

int main(int argc, char **argv)
{
	u32  frame_rate;
	u32 vin_mode;
	char sensor_name[32];
	struct amba_vin_source_info vin_info;
	struct amba_video_info video_info;
	sensor_label sensor_lb;
	int yuv_type_vin_flag;

	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if(img_lib_init(0,0)<0) {
		perror("/dev/iav");
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}


	if (argc < 2) {
		usage();
		return -1;
	}

	if (map_bsb(fd_iav) < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	

	if (init_param(argc, argv) < 0)
		return -1;

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO\n");
		return -1;
	}

	yuv_type_vin_flag = AMBA_VIDEO_TYPE_IS_YUV(video_info.type);
	if (!yuv_type_vin_flag)  {
		get_vin_frame_rate(&frame_rate);
		get_vin_mode(&vin_mode);
	}

	switch (vin_info.sensor_id) {
		case SENSOR_OV9710:
			sensor_lb = OV_9715;
			app_param_image_sensor.p_adj_param = &ov9715_adj_param;
			app_param_image_sensor.p_rgb2yuv = ov9715_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &ov9715_chroma_scale;
			app_param_image_sensor.p_awb_param = &ov9715_awb_param;
			app_param_image_sensor.p_50hz_lines = ov9715_50hz_lines;
			app_param_image_sensor.p_60hz_lines = ov9715_60hz_lines;
			app_param_image_sensor.p_tile_config = &ov9715_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = ov9715_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = ov9715_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = ov9715_dlight;
			app_param_image_sensor.p_manual_LE = ov9715_manual_LE;
			sprintf(sensor_name, "ov9715");
			break;
		case SENSOR_OV2710:
		       	sensor_lb = OV_2710;
			app_param_image_sensor.p_adj_param = &ov2710_adj_param;
			app_param_image_sensor.p_rgb2yuv = ov2710_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &ov2710_chroma_scale;
			app_param_image_sensor.p_awb_param = &ov2710_awb_param;
			app_param_image_sensor.p_50hz_lines = ov2710_50hz_lines;
			app_param_image_sensor.p_60hz_lines = ov2710_60hz_lines;
			app_param_image_sensor.p_tile_config = &ov2710_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = ov2710_ae_agc_dgain;
			if (vin_mode == AMBA_VIDEO_MODE_1080P || vin_mode == AMBA_VIDEO_MODE_AUTO)
				app_param_image_sensor.p_ae_sht_dgain = ov2710_ae_sht_dgain_1080p;
			else if (vin_mode == AMBA_VIDEO_MODE_720P)
				app_param_image_sensor.p_ae_sht_dgain = ov2710_ae_sht_dgain_720p;
			else
				app_param_image_sensor.p_ae_sht_dgain = ov2710_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = ov2710_dlight;
			app_param_image_sensor.p_manual_LE = ov2710_manual_LE;
		      	sprintf(sensor_name, "ov2710");
			break;
		case SENSOR_OV5653:
			sensor_lb = OV_5653;
			app_param_image_sensor.p_adj_param = &ov5653_adj_param;
			app_param_image_sensor.p_rgb2yuv = ov5653_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &ov5653_chroma_scale;
			app_param_image_sensor.p_awb_param = &ov5653_awb_param;
			app_param_image_sensor.p_50hz_lines = ov5653_50hz_lines;
			app_param_image_sensor.p_60hz_lines = ov5653_60hz_lines;
			app_param_image_sensor.p_tile_config = &ov5653_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = ov5653_ae_agc_dgain;
			if(frame_rate ==AMBA_VIDEO_FPS_12&&vin_mode==AMBA_VIDEO_MODE_QSXGA)
			{
				printf("5M12\n");
				app_param_image_sensor.p_ae_sht_dgain =ov5653_ae_sht_dgain_5m12;
			}
			else if(frame_rate ==AMBA_VIDEO_FPS_15&&vin_mode ==AMBA_VIDEO_MODE_QXGA)
			{
				printf("3M15\n");
				app_param_image_sensor.p_ae_sht_dgain =ov5653_ae_sht_dgain_3m15;
			}
			else if(frame_rate == AMBA_VIDEO_FPS_29_97&&vin_mode ==AMBA_VIDEO_MODE_1080P)
			{
				printf("1080p\n");
				app_param_image_sensor.p_ae_sht_dgain = ov5653_ae_sht_dgain_1080p;
			}
			else 
				app_param_image_sensor.p_ae_sht_dgain = ov5653_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = ov5653_dlight;
			app_param_image_sensor.p_manual_LE = ov5653_manual_LE;
			sprintf(sensor_name, "ov5653");
			break;
		case SENSOR_MT9J001:
			sensor_lb = MT_9J001;
			sprintf(sensor_name, "mt9j001");
			break;
		case SENSOR_MT9P001:
			sensor_lb = MT_9P031;
			app_param_image_sensor.p_adj_param = &mt9p006_adj_param;
			app_param_image_sensor.p_rgb2yuv = mt9p006_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &mt9p006_chroma_scale;
			app_param_image_sensor.p_awb_param = &mt9p006_awb_param;
			app_param_image_sensor.p_50hz_lines = mt9p006_50hz_lines;
			app_param_image_sensor.p_60hz_lines = mt9p006_60hz_lines;
			app_param_image_sensor.p_tile_config = &mt9p006_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = mt9p006_ae_agc_dgain;
			if(frame_rate ==AMBA_VIDEO_FPS_12&&vin_mode==AMBA_VIDEO_MODE_QSXGA)
			{
				printf("5M12\n");
				app_param_image_sensor.p_ae_sht_dgain =mt9p006_ae_sht_dgain_5m12;
			}
			else if(frame_rate ==AMBA_VIDEO_FPS_15&&vin_mode ==AMBA_VIDEO_MODE_QXGA)
			{
				printf("3M15\n");
				app_param_image_sensor.p_ae_sht_dgain =mt9p006_ae_sht_dgain_3m15;
			}
			else if(frame_rate == AMBA_VIDEO_FPS_29_97&&vin_mode ==AMBA_VIDEO_MODE_1080P)
			{
				printf("1080p\n");
				app_param_image_sensor.p_ae_sht_dgain = mt9p006_ae_sht_dgain_1080p;
			}
			else 
				app_param_image_sensor.p_ae_sht_dgain = mt9p006_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = mt9p006_dlight;
			app_param_image_sensor.p_manual_LE = mt9p006_manual_LE;
			sprintf(sensor_name, "mt9p006");
			break;
		case SENSOR_MT9M033:
			sensor_lb = MT_9M033;
			app_param_image_sensor.p_adj_param = &mt9m033_adj_param;
			app_param_image_sensor.p_rgb2yuv = mt9m033_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &mt9m033_chroma_scale;
			app_param_image_sensor.p_awb_param = &mt9m033_awb_param;
			app_param_image_sensor.p_50hz_lines = mt9m033_50hz_lines;
			app_param_image_sensor.p_60hz_lines = mt9m033_60hz_lines;
			app_param_image_sensor.p_tile_config = &mt9m033_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = mt9m033_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = mt9m033_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = mt9m033_dlight;
			app_param_image_sensor.p_manual_LE = mt9m033_manual_LE;
			sprintf(sensor_name, "mt9m033");
			break;
		case SENSOR_IMX035:
			sensor_lb = IMX_035;
			app_param_image_sensor.p_adj_param = &imx035_adj_param;
			app_param_image_sensor.p_rgb2yuv = imx035_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &imx035_chroma_scale;
			app_param_image_sensor.p_awb_param = &imx035_awb_param;
			app_param_image_sensor.p_50hz_lines = imx035_50hz_lines;
			app_param_image_sensor.p_60hz_lines = imx035_60hz_lines;
			app_param_image_sensor.p_tile_config = &imx035_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = imx035_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = imx035_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = imx035_dlight;
			app_param_image_sensor.p_manual_LE = imx035_manual_LE;
			sprintf(sensor_name, "imx035");
			break;
		case SENSOR_IMX036:
			sensor_lb = IMX_036;
			app_param_image_sensor.p_adj_param = &imx036_adj_param;
			app_param_image_sensor.p_rgb2yuv = imx036_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &imx036_chroma_scale;
			app_param_image_sensor.p_awb_param = &imx036_awb_param;
			app_param_image_sensor.p_50hz_lines = imx036_50hz_lines;
			app_param_image_sensor.p_60hz_lines = imx036_60hz_lines;
			app_param_image_sensor.p_tile_config = &imx036_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = imx036_ae_agc_dgain;
			if(frame_rate ==AMBA_VIDEO_FPS_15&&vin_mode ==AMBA_VIDEO_MODE_QXGA)
			{
				printf("3M15\n");
				app_param_image_sensor.p_ae_sht_dgain =imx036_ae_sht_dgain_3m15;
			}
			else if(frame_rate == AMBA_VIDEO_FPS_29_97&&vin_mode ==AMBA_VIDEO_MODE_1080P)
			{
				printf("1080p\n");
				app_param_image_sensor.p_ae_sht_dgain = imx036_ae_sht_dgain_1080p;
			}
			else 
				app_param_image_sensor.p_ae_sht_dgain = imx036_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = imx036_dlight;
			app_param_image_sensor.p_manual_LE = imx036_manual_LE;
			sprintf(sensor_name, "imx036");
			break;
		case SENSOR_IMX072:
			sensor_lb = IMX_072;
			app_param_image_sensor.p_adj_param = &imx072_adj_param;
			app_param_image_sensor.p_rgb2yuv = imx072_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &imx072_chroma_scale;
			app_param_image_sensor.p_awb_param = &imx072_awb_param;
			app_param_image_sensor.p_50hz_lines = imx072_50hz_lines;
			app_param_image_sensor.p_60hz_lines = imx072_60hz_lines;
			app_param_image_sensor.p_tile_config = &imx072_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = imx072_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = imx072_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = imx072_dlight;
			app_param_image_sensor.p_manual_LE = imx072_manual_LE;
			sprintf(sensor_name, "imx072");
			break;
		case SENSOR_IMX122:
			sensor_lb = IMX_122;
			app_param_image_sensor.p_adj_param = &imx122_adj_param;
			app_param_image_sensor.p_rgb2yuv = imx122_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &imx122_chroma_scale;
			app_param_image_sensor.p_awb_param = &imx122_awb_param_blue_glass;
			app_param_image_sensor.p_50hz_lines = imx122_50hz_lines;
			app_param_image_sensor.p_60hz_lines = imx122_60hz_lines;
			app_param_image_sensor.p_tile_config = &imx122_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = imx122_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = imx122_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = imx122_dlight;
			app_param_image_sensor.p_manual_LE = imx122_manual_LE;
			sprintf(sensor_name, "imx122");
			break;
		case SENSOR_OV14810:
			sensor_lb = OV_14810;
			app_param_image_sensor.p_adj_param = &ov14810_adj_param;
			app_param_image_sensor.p_rgb2yuv = ov14810_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &ov14810_chroma_scale;
			app_param_image_sensor.p_awb_param = &ov14810_awb_param;
			app_param_image_sensor.p_50hz_lines = ov14810_50hz_lines;
			app_param_image_sensor.p_60hz_lines = ov14810_60hz_lines;
			app_param_image_sensor.p_tile_config = &ov14810_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = ov14810_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = ov14810_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = ov14810_dlight;
			app_param_image_sensor.p_manual_LE = ov14810_manual_LE;
			sprintf(sensor_name, "ov14810");
			break;
		case SENSOR_MT9T002:
			sensor_lb = MT_9T002;
			app_param_image_sensor.p_adj_param = &mt9t002_adj_param;
			app_param_image_sensor.p_rgb2yuv = mt9t002_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &mt9t002_chroma_scale;
			app_param_image_sensor.p_awb_param = &mt9t002_awb_param;
			app_param_image_sensor.p_50hz_lines = mt9t002_50hz_lines;
			app_param_image_sensor.p_60hz_lines = mt9t002_60hz_lines;
			app_param_image_sensor.p_tile_config = &mt9t002_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = mt9t002_ae_agc_dgain;
			if(frame_rate ==AMBA_VIDEO_FPS_15&&vin_mode ==AMBA_VIDEO_MODE_QXGA)
			{
				printf("3M15\n");
				app_param_image_sensor.p_ae_sht_dgain =mt9t002_ae_sht_dgain_3m15;
			}
			else if(frame_rate == AMBA_VIDEO_FPS_29_97&&vin_mode ==AMBA_VIDEO_MODE_1080P30)
			{
				printf("1080p30\n");
				app_param_image_sensor.p_ae_sht_dgain = mt9t002_ae_sht_dgain_1080p30;
			}
			else 
				app_param_image_sensor.p_ae_sht_dgain = mt9t002_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = mt9t002_dlight;
			app_param_image_sensor.p_manual_LE = mt9t002_manual_LE;
			sprintf(sensor_name, "mt9t002");
			break;
		case SENSOR_AR0331:
			sensor_lb = AR_0331;
			app_param_image_sensor.p_adj_param = &ar0331_adj_param;
			app_param_image_sensor.p_rgb2yuv = ar0331_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &ar0331_chroma_scale;
			app_param_image_sensor.p_awb_param = &ar0331_awb_param;
			app_param_image_sensor.p_50hz_lines = ar0331_50hz_lines;
			app_param_image_sensor.p_60hz_lines = ar0331_60hz_lines;
			app_param_image_sensor.p_tile_config = &ar0331_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = ar0331_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = ar0331_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = ar0331_dlight;
			app_param_image_sensor.p_manual_LE = ar0331_manual_LE;
			sprintf(sensor_name, "ar0331");
			break;
		case SENSOR_AR0130:
			sensor_lb = AR_0130;
			app_param_image_sensor.p_adj_param = &ar0130_adj_param;
			app_param_image_sensor.p_rgb2yuv =ar0130_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &ar0130_chroma_scale;
			app_param_image_sensor.p_awb_param = &ar0130_awb_param;
			app_param_image_sensor.p_50hz_lines = ar0130_50hz_lines;
			app_param_image_sensor.p_60hz_lines = ar0130_60hz_lines;
			app_param_image_sensor.p_tile_config = &ar0130_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = ar0130_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = ar0130_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = ar0130_dlight;
			app_param_image_sensor.p_manual_LE = ar0130_manual_LE;
			sprintf(sensor_name, "ar0130");
			break;
		case SENSOR_S5K5B3GX:
			sensor_lb = SAM_S5K5B3;
			app_param_image_sensor.p_adj_param = &s5k5b3gx_adj_param;
			app_param_image_sensor.p_rgb2yuv = s5k5b3gx_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &s5k5b3gx_chroma_scale;
			app_param_image_sensor.p_awb_param = &s5k5b3gx_awb_param;
			app_param_image_sensor.p_50hz_lines = s5k5b3gx_50hz_lines;
			app_param_image_sensor.p_60hz_lines = s5k5b3gx_60hz_lines;
			app_param_image_sensor.p_tile_config = &s5k5b3gx_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = s5k5b3gx_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = s5k5b3gx_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = s5k5b3gx_dlight;
			app_param_image_sensor.p_manual_LE = s5k5b3gx_manual_LE;
			sprintf(sensor_name, "s5k5b3gx");
			break;
		case SENSOR_MN34041PL:
			sensor_lb = MN_34041PL;
			app_param_image_sensor.p_adj_param = &mn34041pl_adj_param;
			app_param_image_sensor.p_rgb2yuv = mn34041pl_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &mn34041pl_chroma_scale;
			app_param_image_sensor.p_awb_param = &mn34041pl_awb_param;
			app_param_image_sensor.p_50hz_lines = mn34041pl_50hz_lines;
			app_param_image_sensor.p_60hz_lines = mn34041pl_60hz_lines;
			app_param_image_sensor.p_tile_config = &mn34041pl_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = mn34041pl_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = mn34041pl_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = mn34041pl_dlight;
			app_param_image_sensor.p_manual_LE = mn34041pl_manual_LE;
			sprintf(sensor_name, "mn34041pl");
			break;
		case SENSOR_IMX104:
			sensor_lb = IMX_104;
			app_param_image_sensor.p_adj_param = &imx104_adj_param;
			app_param_image_sensor.p_rgb2yuv = imx104_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &imx104_chroma_scale;
			app_param_image_sensor.p_awb_param = &imx104_awb_param;
			app_param_image_sensor.p_50hz_lines = imx104_50hz_lines;
			app_param_image_sensor.p_60hz_lines =imx104_60hz_lines;
			app_param_image_sensor.p_tile_config = &imx104_tile_config;
			app_param_image_sensor.p_ae_agc_dgain =imx104_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = imx104_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range =imx104_dlight;
			app_param_image_sensor.p_manual_LE = imx104_manual_LE;
			sprintf(sensor_name, "imx104");
			break;
				
		default:
			if (yuv_type_vin_flag) {
				printf("YUV video input type, skip sensor related config \n");
			} else {
				return -1;
			}
	}

	if (!yuv_type_vin_flag) {
		if (img_config_sensor_info(sensor_lb) < 0) {
			return -1;
		}
		img_config_lens_info(LENS_CMOUNT_ID);
		img_lens_init();
	}

	if(awb_speed_flag){
		img_awb_set_speed(awb_speed);
	}
	if(blc_flag) {
		img_dsp_set_global_blc(fd_iav, &blc, GR);
	}
	if(wb_flag) {
		img_dsp_set_rgb_gain(fd_iav, &wb_gain, dgain);
	}
	if(set_noise_filter_flag){
		img_dsp_filter_configuration(fd_iav, noise_filter); // please note this filter do NOT need calling!
	}

	if(cc_flag) {
		load_dsp_cc_table();
		img_dsp_enable_color_correction(fd_iav, cc_enable);
		img_dsp_set_tone_curve(fd_iav,&tone_curve);
	}

	if(chroma_scale_flag) {
		int i;
		u32 chroma_scale_cell;
		cs.enable = 1;
		memcpy(cs.gain_curve, gain_curve, NUM_CHROMA_GAIN_CURVE*2);
		for (i=0;i<NUM_CHROMA_GAIN_CURVE;i++){
			chroma_scale_cell =  cs.gain_curve[i]*chroma_scale/UNIT_chroma_scale;
			if(chroma_scale_cell>4095){
				printf(">_< Exceeded chroma scale 4096!\n");
				return -1;
			}
			cs.gain_curve[i] = chroma_scale_cell;
		}
		img_dsp_set_chroma_scale(fd_iav,&cs);
	}
	if(mctf_flag) {
		img_dsp_set_video_mctf(fd_iav, &mctf_info);
	}

	if(shutter_flag) {
		img_set_sensor_shutter_index(fd_iav, shutter_index);
	}
	if(agc_flag) {
		img_set_sensor_agc_index(fd_iav, agc_index, 16);
	}

	if(sharpeness_flag){
		if (sharpeness_strength == 1){
			sharpen_setting.low = 0;
			sharpen_setting.low_delta = 1;
			sharpen_setting.low_strength = 250;
			sharpen_setting.mid_strength = 250;
			sharpen_setting.high = 180;
			sharpen_setting.high_delta = 5;
			sharpen_setting.high_strength = 250;
			img_dsp_set_sharpening_level_min(fd_iav,&sharpen_setting);
			sharpen_setting.low = 10;
			sharpen_setting.low_delta = 5;
			sharpen_setting.low_strength = 250;
			sharpen_setting.mid_strength = 250;
			sharpen_setting.high = 254;
			sharpen_setting.high_delta = 0;
			sharpen_setting.high_strength = 250;
			img_dsp_set_sharpening_level_overall(fd_iav,&sharpen_setting);
		}else{
			sharpen_setting.low = 0;
			sharpen_setting.low_delta = 1;
			sharpen_setting.low_strength = 0;
			sharpen_setting.mid_strength = 0;
			sharpen_setting.high = 180;
			sharpen_setting.high_delta = 5;
			sharpen_setting.high_strength = 0;
			img_dsp_set_sharpening_level_min(fd_iav,&sharpen_setting);
			sharpen_setting.low = 10;
			sharpen_setting.low_delta = 5;
			sharpen_setting.low_strength = 0;
			sharpen_setting.mid_strength = 0;
			sharpen_setting.high = 254;
			sharpen_setting.high_delta = 0;
			sharpen_setting.high_strength = 0;
			img_dsp_set_sharpening_level_overall(fd_iav,&sharpen_setting);
		}
	}
	if(zoom_flag){
		zoom_factor_info_t zoomparam;
		zoomparam.x_center_offset = 0;
		zoomparam.y_center_offset = 0;
		zoomparam.zoom_x = (int)(zoom_factor);
		zoomparam.zoom_y = (int)(zoom_factor);
		img_dsp_set_dzoom_factor(fd_iav, &zoomparam);
	}
	if(dbp_flag){
		img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
	}
	if(anti_aliasing_flag){
		u8 strength = anti_aliasing_strength;
		img_dsp_set_anti_aliasing(fd_iav, strength);
	}
	if(cfa_filter_flag){
		cfa_noise_filter.iso_center_weight_red = 1;
		cfa_noise_filter.iso_center_weight_green = 1;
		cfa_noise_filter.iso_center_weight_blue = 1;
		cfa_noise_filter.iso_thresh_k0_red = 10000;
		cfa_noise_filter.iso_thresh_k0_green = 10000;
		cfa_noise_filter.iso_thresh_k0_blue = 10000;
		cfa_noise_filter.iso_thresh_k0_close = 10000;
		cfa_noise_filter.iso_thresh_k1_red = 1;
		cfa_noise_filter.iso_thresh_k1_green = 1;
		cfa_noise_filter.iso_thresh_k1_blue = 1;
		cfa_noise_filter.direct_thresh_k1_red = 1;
		cfa_noise_filter.direct_thresh_k1_green = 1;
		cfa_noise_filter.direct_thresh_k1_blue = 1;
		cfa_noise_filter.direct_grad_thresh = 341;
		img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
	}
	if(local_exposure_flag){
		img_dsp_set_local_exposure(fd_iav, &local_exposure);
	}
	if(chroma_median_flag){
		img_dsp_set_chroma_median_filter(fd_iav, &chroma_median_setup);
	}
	if(high_freq_flag){
		img_dsp_set_luma_high_freq_noise_reduction(fd_iav, luma_high_freq_noise_reduction_strength);
	}
	if(rgb2yuv_flag){
		rgb2yuv_matrix.u_offset = 0;
		rgb2yuv_matrix.v_offset = 128;
		rgb2yuv_matrix.y_offset = 128;
		img_dsp_set_rgb2yuv_matrix(fd_iav, &rgb2yuv_matrix);
	}
	if(cfa_leakage_flag){
		cfa_leakage_filter.alpha_bb = 32;
		cfa_leakage_filter.alpha_br = 32;
		cfa_leakage_filter.alpha_rb = 32;
		cfa_leakage_filter.alpha_rr = 32;
		cfa_leakage_filter.saturation_level = 1024;
		img_dsp_set_cfa_leakage_filter(fd_iav, &cfa_leakage_filter);
	}
	if(spatial_filter_flag){
		// only in ENCODE mode
		img_dsp_set_spatial_filter(fd_iav, 0, &spatial_filter_info); // mode == 0 for video mode
	}
	if(sharpening_fir_flag){
		img_dsp_set_sharpening_fir(fd_iav, &fir, 0); // mode == 0 for video mode
	}
	if(coring_table_flag){
		img_dsp_set_sharpening_fir_coring(fd_iav, &coring, retain_level);
	}
	if(max_change_flag){
		img_dsp_set_sharpening_max_change(fd_iav, &max_change);
	}
	if(vig_to_awb_flag){
		aaa_api_t custom_aaa_api = {0};

		int fd_lenshading = -1;
		int read_count;
		static u32 raw_width, raw_height;
		static u32 lookup_shift;
		static u16 vignette_table[VIGNETTE_MAX_WIDTH * VIGNETTE_MAX_HEIGHT * 4] = {0};
		static u16 vig_awb_gain_table[24*16 * 4] = {0};
		// 3a preparation
		load_dsp_cc_table();

		load_adj_cc_table(sensor_name);

		// read vignette gain table info
		if((fd_lenshading = open("./lens_shading.bin", O_RDONLY, 0))<0) {
			printf("lens_shading.bin cannot be opened\n");
			return -1;
		}
		if((read_count = read(fd_lenshading, vignette_table, 4*VIGNETTE_MAX_SIZE*sizeof(u16))) != 4*VIGNETTE_MAX_SIZE*sizeof(u16)){
			printf("read lens_shading.bin error\n");
			return -1;
		}
		if((read_count = read(fd_lenshading, &lookup_shift, sizeof(u32))) != sizeof(u32)){
			printf("read lens_shading.bin error\n");
			return -1;
		}
		if((read_count = read(fd_lenshading,&raw_width,sizeof(raw_width))) != sizeof(raw_width)){
			printf("read lens_shading.bin error\n");
			return -1;
		}
		if((read_count = read(fd_lenshading,&raw_height,sizeof(raw_height))) != sizeof(raw_height)){
			printf("read lens_shading.bin error\n");
			return -1;
		}
		close(fd_lenshading);
		// config and start 3a
		img_load_image_sensor_param(&app_param_image_sensor);
		img_register_aaa_algorithm(custom_aaa_api);

		if(img_start_aaa(fd_iav))
			return -1;
		sleep(1);
		// apply vignette gain before awb statis
		convert_vig_table_to_awb_table(vignette_table,vig_awb_gain_table,raw_width,raw_height,24,16);
		img_vig_set_awb_statis_gain(1,vig_awb_gain_table,lookup_shift, 24,16);
		while(1){sleep(100);}
	}

	if(vignette_compensation_flag){
		int fd_lenshading = -1;
		int count;
		static u32 gain_shift;
		static u16 vignette_table[33*33*4] = {0};
		if((fd_lenshading = open("./lens_shading.bin", O_RDONLY, 0))<0) {
			printf("lens_shading.bin cannot be opened\n");
			return -1;
		}
		count = read(fd_lenshading, vignette_table, 4*VIGNETTE_MAX_SIZE*sizeof(u16));
		if(count != 4*VIGNETTE_MAX_SIZE*sizeof(u16)){
			printf("read lens_shading.bin error\n");
			return -1;
		}
		count = read(fd_lenshading, &gain_shift, sizeof(u32));
		if(count != sizeof(u32)){
			printf("read lens_shading.bin error\n");
			return -1;
		}
		printf("gain_shift is %d\n",gain_shift);
		vignette_info.enable = 1;
		vignette_info.gain_shift = (u8)gain_shift;
		vignette_info.vignette_red_gain_addr = (u32)(vignette_table + 0*VIGNETTE_MAX_SIZE);
		vignette_info.vignette_green_even_gain_addr = (u32)(vignette_table + 1*VIGNETTE_MAX_SIZE);
		vignette_info.vignette_green_odd_gain_addr = (u32)(vignette_table + 2*VIGNETTE_MAX_SIZE);
		vignette_info.vignette_blue_gain_addr = (u32)(vignette_table + 3*VIGNETTE_MAX_SIZE);
		img_dsp_set_vignette_compensation(fd_iav, &vignette_info);
		return 0;
	}
	if(cal_lens_shading){
		still_cap_info_t still_cap_info;
		vignette_cal_t vig_detect_setup;
		int fd_lenshading = -1;
		int rval = -1;
		u16* raw_buff;
//		int shutter_idx;
		u32 lookup_shift;
//		iav_mmap_info_t mmap_info;
		iav_raw_info_t raw_info;
		static u16 tab_buf[33*33*4] = {0};
		blc_level_t blc;
		//TODO: start 3A here to do AE
#if 0
		img_start_aaa(fd_iav);
		sleep(5);
		img_enable_ae(0);
		img_enable_awb(0);
		img_enable_af(0);
		img_enable_adj(0);
		shutter_idx = img_get_sensor_shutter_index();
		if (flicker_mode == 50){
			if (shutter_idx!=1106 || shutter_idx!=1234){ // 1/50s, 1/100s
				printf("Error: Lighting too strong!\n");
				return -1;
			}
		}else if (flicker_mode == 60){
			if (shutter_idx!=1012 || shutter_idx!=1140 || shutter_idx!=1268){ // 1/30s, 1/60s, 1/120s
				printf("Error: Lighting too strong!\n");
				return -1;
			}
		}else{
			printf("Anti-flicker mode can not be recognized\n");
			return -1;
		}
#endif
		//Capture raw here
		printf("Raw capture started\n");
		memset(&still_cap_info, 0, sizeof(still_cap_info));
		still_cap_info.capture_num = 1;
		still_cap_info.need_raw = 1;
		img_init_still_capture(fd_iav, 95);
		img_start_still_capture(fd_iav, &still_cap_info);
//		rval = ioctl(fd_iav, IAV_IOC_MAP_DSP, &mmap_info);
//		if (rval < 0) {
//			perror("IAV_IOC_MAP_DSP");
//			return rval;
//		}
		rval = ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info);
		if (rval < 0) {
			perror("IAV_IOC_READ_RAW_INFO");
			return rval;
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
		vig_detect_setup.threshold = 4095;
		vig_detect_setup.compensate_ratio = 896;
		vig_detect_setup.lookup_shift = 255;
		/*output*/
		vig_detect_setup.r_tab = tab_buf + 0*VIGNETTE_MAX_SIZE;
		vig_detect_setup.ge_tab = tab_buf + 1*VIGNETTE_MAX_SIZE;
		vig_detect_setup.go_tab = tab_buf + 2*VIGNETTE_MAX_SIZE;
		vig_detect_setup.b_tab = tab_buf + 3*VIGNETTE_MAX_SIZE;
		img_dsp_get_global_blc(&blc);
		vig_detect_setup.blc.r_offset = blc.r_offset;
		vig_detect_setup.blc.gr_offset = blc.gr_offset;
		vig_detect_setup.blc.gb_offset = blc.gb_offset;
		vig_detect_setup.blc.b_offset = blc.b_offset;
		rval = img_cal_vignette(&vig_detect_setup);
		if (rval<0){
			printf("img_cal_vignette error!\n");
			goto vignette_cal_exit;
		}
		lookup_shift = vig_detect_setup.lookup_shift;
		if((fd_lenshading = open("./lens_shading.bin", O_CREAT | O_TRUNC | O_WRONLY, 0777))<0) {
			printf("vignette table file open error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, tab_buf, (4*VIGNETTE_MAX_SIZE*sizeof(u16)));
		if (rval<0){
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, &lookup_shift, sizeof(lookup_shift));
		if (rval<0){
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading,&raw_info.width,sizeof(raw_info.width));
		if(rval<0){
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading,&raw_info.height,sizeof(raw_info.height));
		if(rval<0){
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
	if(fpn_flag){
		int fd_bpc = -1;
		int height = 0, width = 0;
		u8* fpn_map_addr = NULL;
		u32 bpc_num = 0;
		u32 raw_pitch;

		memset(&cfa_noise_filter,0,sizeof(cfa_noise_filter));
		img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);

		bad_corr.dark_pixel_strength = 0;
		bad_corr.hot_pixel_strength = 0;
		bad_corr.enable = 0;
		img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
		img_dsp_set_anti_aliasing(fd_iav,0);


		width = badpixel_detect_algo.cap_width;
		height = badpixel_detect_algo.cap_height;

		raw_pitch = ROUND_UP(width/8, 32);
		fpn_map_addr = malloc(raw_pitch*height);
		if (fpn_map_addr<0){
			printf("can not malloc memory for bpc map\n");
			return -1;
		}
		memset(fpn_map_addr,0,(raw_pitch*height));

		badpixel_detect_algo.badpixmap_buf = fpn_map_addr;
		badpixel_detect_algo.cali_mode = 0; // 0 for video, 1 for still

		bpc_num = img_cali_bad_pixel(fd_iav, &badpixel_detect_algo);
		printf("Totol number is %d\n",bpc_num);

		if((fd_bpc = open("./bpcmap.bin", O_CREAT | O_TRUNC | O_WRONLY, 0777))<0) {
			printf("map file open error!\n");
		}
		write(fd_bpc, fpn_map_addr, (raw_pitch*height));
		free(fpn_map_addr);
	}
	if(load_fpn_flag){
		int file = -1, count = -1;
		u8* fpn_map_addr = NULL;
		u32 raw_pitch;
		int height = 0, width = 0;
		u32 fpn_map_size = 0;
//		int i,j;
//		i = 629;j = 678;

		memset(&cfa_noise_filter,0,sizeof(cfa_noise_filter));
		img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
		bad_corr.dark_pixel_strength = 0;
		bad_corr.hot_pixel_strength = 0;
		bad_corr.enable = 0;
		img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
		img_dsp_set_anti_aliasing(fd_iav,0);

		width = load_fpn_param[0];
		height = load_fpn_param[1];
		raw_pitch = ROUND_UP((width/8), 32);
		fpn_map_size = raw_pitch*height;
		fpn_map_addr = malloc(fpn_map_size);
		memset(fpn_map_addr,0,(raw_pitch*height));

		if((file = open("./bpcmap.bin", O_RDONLY, 0))<0) {
			printf("bpcmap.bin cannot be opened\n");
			return -1;
		}
		if((count = read(file, fpn_map_addr, fpn_map_size)) != fpn_map_size) {
			printf("read bpcmap.bin error\n");
			return -1;
		}
//		*(fpn_map_addr+j*raw_pitch+(i/8)) = 1<<(i%8);

		fpn.enable = 3;
		fpn.fpn_pitch = raw_pitch;
		fpn.pixel_map_height = height;
		fpn.pixel_map_width = width;
		fpn.pixel_map_size = fpn_map_size;
		fpn.pixel_map_addr = (u32)fpn_map_addr;
		img_dsp_set_static_bad_pixel_correction(fd_iav,&fpn);
		free(fpn_map_addr);
	}
	if(still_cap) {
		still_cap_info_t still_cap_info;
		memset(&still_cap_info, 0, sizeof(still_cap_info));
		still_cap_info.capture_num = 1;
		still_cap_info.need_raw = 1;

		img_init_still_capture(fd_iav, 95);
		img_start_still_capture(fd_iav, &still_cap_info);
		save_raw();
		img_stop_still_capture(fd_iav);
		save_jpeg_stream();
		if (ioctl(fd_iav, IAV_IOC_LEAVE_STILL_CAPTURE) < 0) {
			perror("IAV_IOC_LEAVE_STILL_CAPTURE");
			return -1;
		}
	}
	if(hdr_flag){

		int raw_size;
		still_hdr_proc_t hdr;
		int cur_shutter;
		u16 dgain;
		img_dsp_get_rgb_gain(&wb_gain, &dgain);
		cur_shutter = img_get_sensor_shutter_index();

		hdr.height = 1080;
		hdr.width = 1920;
		raw_size = hdr.height * hdr.width * 2;
		hdr.short_exp_raw = malloc(raw_size);
		if (!hdr.short_exp_raw){
			printf("can not malloc memory for short exposure raw\n");
			return -1;}
		hdr.long_exp_raw = malloc(raw_size);
		if (!hdr.long_exp_raw){
			printf("can not malloc memory for long exposure raw\n");
			return -1;}
		hdr.raw_buff = NULL;
		hdr.long_shutter_idx = cur_shutter;
		hdr.short_shutter_idx = cur_shutter+512;

		hdr.r_gain = wb_gain.r_gain;
		hdr.b_gain = wb_gain.b_gain;
		img_still_hdr_proc(fd_iav, &hdr);
		save_jpeg_stream();
		free(hdr.short_exp_raw);
		free(hdr.long_exp_raw);
		if (ioctl(fd_iav, IAV_IOC_ENABLE_PREVIEW) < 0) {
			perror("IAV_IOC_ENABLE_PREVIEW");
			return -1;
		}
		img_dsp_set_rgb_gain(fd_iav, &wb_gain, 1024);
	}
	if(raw_proc_flag) {/*
		int fd_raw = -1;
		iav_mmap_info_t mmap_info;
		int rval = -1, count = 0;
		int height = 0, width = 0;
		u8* raw_buf = NULL;
		u32 raw_size = 0;

		if((fd_raw = open(pic_file_name, O_RDONLY, 0))<0) {
			printf("raw file open error!\n");
			goto raw_proc_exit;
		}

		height = 1080;
		width = 1920;
		raw_size = height*width*2;
		raw_buf = malloc(raw_size);
		if(raw_buf < 0){
			printf("malloc raw_buffer error!\n");
			goto raw_proc_exit;
		}

		count = read(fd_raw, raw_buf, raw_size);
		if(count != raw_size){
			printf("raw file read error!\n");
			goto raw_proc_exit;
		}

		rval = ioctl(fd_iav, IAV_IOC_MAP_DSP, &mmap_info);
		if (rval < 0) {
			perror("IAV_IOC_MAP_DSP");
			return -1;
		}

		proc_init_info.height = height;
		proc_init_info.width = width;
		proc_init_info.__user_raw_addr = raw_buf;
		proc_info.height = height;
		proc_info.width = width;

		img_init_still_proc_from_memory(fd_iav, &proc_init_info);
		img_still_proc_from_memory(fd_iav,&proc_info);
		img_stop_still_proc_mem(fd_iav);
		save_jpeg_stream();

raw_proc_exit:
		free(raw_buf);
		close(fd_raw);
		return -1;*/
		return -1;
	}
	if(msg_send_flag) {
		img_set_extra_blc(msg_param);
	}
	if(eis_flag){
#ifdef BUILD_AMBARELLA_EIS
		u16 eis_eis_str,eis_rsc_str;
		eis_eis_str = eis_eis_str_x|(eis_eis_str_y<<8);
		eis_rsc_str = eis_rsc_str_x|(eis_rsc_str_y<<8);
		struct eis_setup eis_setup_obj = {
			/*gyro sensor related*/
			.gyro_id = 0x68, // gyro sensor id
			.gyro_x_chan = 0, // gyro sensor x adc channel
			.gyro_y_chan = 1, // gyro sensor y adc channel
			.gyro_x_polar = 1, // gyro sensor x polarity
			.gyro_y_polar = 1, // gyro sensor y polarity
			.vol_div_num = 1, // numerator of voltage divider
			.vol_div_den = 1, // denominator of voltage divider
			.max_rms_noise = 20, // gyro sensor rms noise level
			.start_up_time = 50, // gyro sensor start-up time, ms
			.full_scale_range = 250, // gyro full scale range, deg/sec
			.mean_x = 8388608,
			.mean_y = 8388608,
			.sense_x = 0x8300, // LSB/(deg/sec)
			.sense_y = 0x8300, // LSB/(deg/sec)
			/*encoder related*/
			.main_w = 1920,
			.main_h = 1080,
			/*image sensor related*/
			.sensor_cell_size = 300, // 3um, ov2710
			.fps = 30,
			/*lens related*/
			.focal_value = 400, // >_< for 5mm lens
			.eis_range_perct = 10,
			/*algo param*/
			.sampling_rate = 2,
		};
		if(ioctl(fd_iav, IAV_IOC_EIS_SETUP_INFO,&eis_setup_obj)<0){
			perror("IAV_IOC_EIS_SETUP_INFO");
			return -1;
		}
		if(ioctl(fd_iav,IAV_IOC_EIS_SET_EIS_STRENGTH,eis_eis_str)<0){
			perror("IAV_IOC_EIS_SET_EIS_STRENGTH");
			return -1;
		}
		if(ioctl(fd_iav,IAV_IOC_EIS_SET_RSC_STRENGTH,eis_rsc_str)<0){
			perror("IAV_IOC_EIS_SET_RSC_STRENGTH");
			return -1;
		}
		if(ioctl(fd_iav, IAV_IOC_EIS_INIT)<0){
			perror("IAV_IOC_EIS_INIT");
			return -1;
		}
		sleep(2);
 		if(ioctl(fd_iav, IAV_IOC_EIS_ENABLE,eis_enable)<0){
			perror("IAV_IOC_EIS_ENABLE");
			return -1;
		}
#endif
	}
	if(eis_log_flag){
#ifdef BUILD_AMBARELLA_EIS
		int i;
		u16* raw_x;
		u16* raw_y;
		int* bias_x;
		int* bias_y;
		int* mv_x;
		int* mv_y;
		struct eis_gyro_log eis_log = {0};
		eis_log.raw_x = NULL;
		eis_log.raw_y = NULL;
		eis_log.bias_x = NULL;
		eis_log.bias_y = NULL;
		eis_log.mv_x = NULL;
		eis_log.mv_y = NULL;
		eis_log.enable = 1;
		printf("start to log\n");
		if(ioctl(fd_iav,IAV_IOC_EIS_LOG_GYRO,&eis_log)<0)
			perror("IAV_IOC_EIS_LOG_GYRO");
		if (eis_log.buff_size == -1){
			perror("get log error!\n");
			return -1;
		}
		raw_x = malloc(eis_log.buff_size*sizeof(u16));
		if(raw_x < 0){
			printf("malloc raw_x error!\n");
			return -1;
		}
		raw_y = malloc(eis_log.buff_size*sizeof(u16));
		if(raw_y < 0){
			printf("malloc raw_y error!\n");
			return -1;
		}
		bias_x = malloc(eis_log.buff_size*sizeof(int));
		if(bias_x < 0){
			printf("malloc bias_x error!\n");
			return -1;
		}
		bias_y = malloc(eis_log.buff_size*sizeof(int));
		if(bias_y < 0){
			printf("malloc bias_y error!\n");
			return -1;
		}
		mv_x = malloc(eis_log.buff_size*sizeof(int));
		if(mv_x < 0){
			printf("malloc mv_x error!\n");
			return -1;
		}
		mv_y = malloc(eis_log.buff_size*sizeof(int));
		if(mv_y < 0){
			printf("malloc mv_y error!\n");
			return -1;
		}
		eis_log.raw_x = raw_x;
		eis_log.raw_y = raw_y;
		eis_log.bias_x = bias_x;
		eis_log.bias_y = bias_y;
		eis_log.mv_x = mv_x;
		eis_log.mv_y = mv_y;
		eis_log.enable = 0;
		for (i = 15;i>0;i--){
			printf("%d\n",i);
			sleep(1);
		}
		if(ioctl(fd_iav,IAV_IOC_EIS_LOG_GYRO,&eis_log)<0)
			perror("IAV_IOC_EIS_LOG_GYRO");
		for (i =2000;i<3000;i+=1){
			printf("%6d:(%6d),(%6d),(%6d)\n",\
				i,(raw_x[i]-32768),((bias_x[i]>>8)-32768),(mv_x[i]>>8));
		}
		free(raw_x);
		free(raw_y);
		free(bias_x);
		free(bias_y);
		free(mv_x);
		free(mv_y);
#endif
		return 0;
	}
	if(gyro_calib_flag){
#ifdef BUILD_AMBARELLA_EIS
		static u16 buf_x[SAMPLE_NUM_MAX];
		static u16 buf_y[SAMPLE_NUM_MAX];
		cali_gyro_setup_t input = {0};
		gyro_calib_info_t out = {0};
		int i;
		static struct eis_gyro_log eis_log = {0};
//TODO: firstly, we turn off EIS
/*
		struct eis_setup gyro_dev_info = {
			.gyro_id = 0x68, // gyro sensor id
			.gyro_x_chan = 0, // gyro sensor x adc channel
			.gyro_y_chan = 1, // gyro sensor y adc channel
			.gyro_x_polar = 1, // gyro sensor x polarity
			.gyro_y_polar = 1, // gyro sensor y polarity
			.vol_div_num = 1, // numerator of voltage divider
			.vol_div_den = 1, // denominator of voltage divider
			.max_rms_noise = 20, // gyro sensor rms noise level
			.start_up_time = 50, // gyro sensor start-up time, ms
			.full_scale_range = 250, // gyro full scale range, deg/sec
			.mean_x = 8388608,
			.mean_y = 8388608,
			.sense_x = 0x8300, // LSB/(deg/sec)
			.sense_y = 0x8300, // LSB/(deg/sec)
			.sensor_cell_size = 300, // 3um, ov2710
			.focal_value = 500, // >_< for 5mm lens
			.eis_range_perct = 10,
			.sampling_rate = 1,
		};
		if(ioctl(fd_iav, IAV_IOC_EIS_SETUP_INFO,&gyro_dev_info)<0)
			perror("IAV_IOC_EIS_SETUP_INFO");
		if(ioctl(fd_iav, IAV_IOC_EIS_INIT)<0)
			perror("IAV_IOC_EIS_INIT");
		sleep(2);
 		if(ioctl(fd_iav, IAV_IOC_EIS_ENABLE,0)<0)
			perror("IAV_IOC_EIS_ENABLE");
*/
		eis_log.raw_x = buf_x;
		eis_log.raw_y = buf_y;
		eis_log.bias_x = NULL;
		eis_log.bias_y = NULL;
		eis_log.mv_x = NULL;
		eis_log.mv_y = NULL;
		eis_log.enable = 1;
		printf("--- Gyro Acquire START! ---\n");
		sleep(1);
		if(ioctl(fd_iav,IAV_IOC_EIS_LOG_GYRO,&eis_log)<0)
			perror("IAV_IOC_EIS_LOG_GYRO");
		if (eis_log.buff_size == -1){
			perror("get log error!\n");
			return -1;
		}
		for (i = 15;i>0;i--){
			printf("%d\n",i);
			sleep(1);
		}
		printf("--- Gyro Acquire FINISH! ---\n");
		eis_log.enable = 0;
		if(ioctl(fd_iav,IAV_IOC_EIS_LOG_GYRO,&eis_log)<0)
			perror("IAV_IOC_EIS_LOG_GYRO");
		if(gyro_calib_mode == GYRO_BIAS_CALIB){
			input.time =  10;//10s
			input.amp = 0;
			input.freq = 0;
			input.buf_x = buf_x;
			input.buf_y = buf_y;
			input.buf_size = SAMPLE_NUM_MAX;
			input.mode = GYRO_BIAS_CALIB;
			input.sampling_rate = 1;
			if ((img_cal_gyro(&input,&out))<0){
				printf("img_cal_gyro error\n");
			}else{
				printf("%d,%d,%d,%d\n", out.mean_x, out.mean_y, out.sense_x, out.sense_y);
			}
		}else if(gyro_calib_mode == GYRO_FUNC_CALIB){
			input.time =  10;//10s
			input.amp = 5;//0.5 degree
			input.freq = 4;//4hz
			input.buf_x = buf_x;
			input.buf_y = buf_y;
			input.buf_size = SAMPLE_NUM_MAX;
			input.mode = GYRO_FUNC_CALIB;
			input.sampling_rate = 1;
			if ((img_cal_gyro(&input,&out))<0){
				printf("img_cal_gyro error\n");
			}else{
				printf("%d,%d,%d,%d\n", out.mean_x, out.mean_y, out.sense_x, out.sense_y);
			}
		}else{
			printf("unknow gyro calib mode\n");
			return -1;
		}
		return 0;
#endif
	}

	if(wb_cali_org_flag){
		char is_env_light_28;
		char is_env_light_65;
		aaa_api_t custom_aaa_api = {0};

		load_dsp_cc_table();
		load_adj_cc_table(sensor_name);

		img_load_image_sensor_param(&app_param_image_sensor);
		img_register_aaa_algorithm(custom_aaa_api);

		if(img_start_aaa(fd_iav))
			return -1;

		sleep(1);
		//default awb work method is AWB_NORMAL
		//change awb work method to AWB_CUSTOM
		img_awb_set_method(AWB_CUSTOM);

		printf("\nPls put the grey card infront of the camera.\n");
		printf("\nPls set the envirnmental light to LOW Color Temperature.\n");
		do{
			if(is_env_light_28 != '\n'){
				printf("Is it ready? (y/n) : ");
			}
			scanf("%c",&is_env_light_28);
		}while(is_env_light_28 != 'y' && is_env_light_28 != 'Y');
		img_awb_get_wb_cal(&org_gain_cali[0]);

		printf("Pls set the envirnmental light to HIGH Color Temperature.\n");
		do{
			if(is_env_light_65 != '\n'){
				printf("Is it ready? (y/n) : ");
			}
			scanf("%c",&is_env_light_65);
		}while(is_env_light_65 != 'y' && is_env_light_65 != 'Y');
		img_awb_get_wb_cal(&org_gain_cali[1]);

		printf("Original wb gains calculating finished.\n");
		printf("\norg low : %4d %4d %4d\n",org_gain_cali[0].r_gain,org_gain_cali[0].g_gain,org_gain_cali[0].b_gain);
		printf("org high: %4d %4d %4d\n\n",org_gain_cali[1].r_gain,org_gain_cali[1].g_gain,org_gain_cali[1].b_gain);
		img_awb_set_method(AWB_NORMAL);
	}

	if(wb_shift_flg){
		img_awb_set_wb_shift(wb_cali_org, wb_cali_ref);
	}

	if(wb_cali_thr_flg){
		img_awb_set_cali_diff_thr(wb_cali_thr_r,wb_cali_thr_b);
	}
	if(yuv_extra_bright_flg){
		img_enable_adj_yuv_extra_brightness(yuv_extra_bright_enable);
	}
	if(hdr_operation_mode_flg){
		img_set_sensor_hdr_operation_mode(hdr_operation_mode);
	}
	if(start_aaa)
	{
		aaa_api_t custom_aaa_api = {0};

		load_dsp_cc_table();
		load_adj_cc_table(sensor_name);

		img_load_image_sensor_param(&app_param_image_sensor);
		img_register_aaa_algorithm(custom_aaa_api);

		if(img_start_aaa(fd_iav))
			return -1;

		while(1){sleep(100);}
	}
	return 0;

}

