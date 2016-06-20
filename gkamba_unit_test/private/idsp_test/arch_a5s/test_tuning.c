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
#include "ambas_vin.h"
#include "img_struct.h"
#include "img_api.h"
#include <sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<pthread.h>
#include"img_dsp_interface.h"

#include "adj_params/imx036_adj_param.c"
#include "adj_params/imx036_aeb_param.c"

#include "adj_params/imx035_adj_param.c"
#include "adj_params/imx035_aeb_param.c"

#include "adj_params/ov2710_adj_param.c"
#include "adj_params/ov2710_aeb_param.c"

#include "adj_params/ov9715_adj_param.c"
#include "adj_params/ov9715_aeb_param.c"

#include "adj_params/ov9718_adj_param.c"
#include "adj_params/ov9718_aeb_param.c"

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

static image_sensor_param_t app_param_image_sensor;
static int myflag=1;

//#define TUNING_ON
#define MAX_YUV_BUFFER_SIZE		(720*480)		// 1080p
#define NUM_AAAINFO 1288
#define MAX_DUMP_BUFFER_SIZE 50*1024
#define TUNING_printf(...)
#define NO_ARG	0
#define HAS_ARG	1
#define UNIT_chroma_scale (64)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
static const char* short_options = "cn:l:m:s:t:A:S:ap:z:t:d:L:C:E:w:M:H:P:y:f:F:Bb:Vv:i:q:";
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
	{0, 0, 0, 0},
};

u8 still_cap;
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
u32 shutter_index, agc_index,iris_index;
u8 local_exposure_flag;
u8 cc_enable = 0;
u16 wb_flag,chroma_median_flag,high_freq_flag,rgb2yuv_flag;
u16 cfa_leakage_flag,spatial_filter_flag, sharpening_fir_flag,coring_table_flag;
u16 max_change_flag,vignette_compensation_flag;
u16 msg_send_flag;
int msg_param;
static blc_level_t blc;
static s32 black_correct_param[4];
static cfa_noise_filter_info_t cfa_noise_filter;
static int cfa_noise_filter_param[5];
static dbp_correction_t bad_corr;
static int bad_corr_param[3];
static wb_gain_t wb_gain;
static int wb_gain_param[3];
static u16 dgain;
static chroma_median_filter_t chroma_median_setup;
static int chroma_median_param[3];
u8 coring_strength;
static video_mctf_info_t mctf_info;
static u8 luma_high_freq_noise_reduction_strength;
static int mctf_param[4];
static rgb_to_yuv_t rgb2yuv_matrix;
static int rgb2yuv_param[9];
static int spatial_filter_param[3];
static sharpen_level_t sharpen_setting_min;
static sharpen_level_t sharpen_setting_overall;
static digital_sat_level_t  d_gain_satuation_level;
static dbp_correction_t dbp_correction_setting;
static statistics_config_t aaa_statistic_config;
static af_statistics_ex_t af_statistic_setup_ex;
static aaa_tile_info_t aaa_tile_get_info;
static ae_data_t aaa_ae_info[96];
static awb_data_t aaa_awb_info[1024];
static af_stat_t  aaa_af_info[40];
static af_stat_t  aaa_af_info2[40];
static embed_hist_stat_t aaa_hist_info;
static histogram_stat_t dsp_histo_info;
static aaa_tile_report_t act_tile;
static aaa_cntl_t aaa_cntl_station;
static rgb_aaa_stat_t p_rgb_stat;
static cfa_aaa_stat_t  p_cfa_stat;
char  sensor_type_option[60];
static image_mode mode;
extern u16 sensor_double_step;
static fpn_correction_t fpn;
static cali_badpix_setup_t badpixel_detect_algo;
static u32 saturation,contrast,hue,brightness;
static image_property_t image_prop;
static color_correction_t color_corr;
static color_correction_reg_t color_corr_reg;
#ifdef TUNING_ON
static int predefined_param;
static int sharpen_setting_param[7];
static int fir_param[11];
static int coring_param[257];
static int chroma_scale_param[129];
static int local_exposure_param[262];
#endif
 u8 * uv_buffer_clipped;
 u8 * y_buffer_clipped;
int uv_width, uv_height;
//static iav_mmap_info_t jpg_mmap_info;
static bs_fifo_info_t jpg_bs_info;
//static iav_mmap_info_t raw_mmap_info;
static iav_raw_info_t raw_info;
static cfa_leakage_filter_t cfa_leakage_filter;
static max_change_t max_change;
static int max_change_param[2];
static spatial_filter_t spatial_filter_info;
static fir_t fir = {-1, {0, -1, 0, 0, 0, 8, 0, 0, 0, 0}};
static u8 retain_level = 1;
u8 *bsb_mem;
u32 bsb_size;
static int vin_op_mode = AMBA_VIN_LINEAR_MODE;	//VIN is linear or HDR
/*
extern int adj_set_hue(int hue);
extern int adj_set_brightness(int bright);
extern int adj_set_contrast(int contrast);
extern int adj_set_saturation(int saturation);
extern int adj_get_img_property(image_property_t * p_image_property);
extern int adj_set_color_conversion(rgb_to_yuv_t * rgb_to_yuv_matrix);
*/
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
int Int32ToArray(int data,int* argarr)
{
	int i,j;
	i=1;
	int datacpy=0;
	int temp=0;
	if(data>0)
	{
		datacpy=data;
		argarr[0]=0;
	}
	else
	{
		datacpy=-data;
		argarr[0]=1;
	}
	while(datacpy/10)
	{
		temp=datacpy%10;
		argarr[i]=temp;
		datacpy=datacpy/10;
		i++;
	}
	argarr[i]=datacpy;


	for(j=1;j<i/2+1;j++)
	{
		temp=argarr[j];
		argarr[j]=argarr[i-j+1];
		argarr[i-j+1]=temp;

	}
	return i+1;

}
int U32ToCharArray(int data,char* argarr)
{
	int i,j;
	i=0;
	int datacpy=0;
	int temp=0;
	datacpy=data;

	while(datacpy/10)
	{
		temp=datacpy%10;
		argarr[i]=temp;
		datacpy=datacpy/10;
		i++;
	}
	argarr[i]=datacpy;

	for(j=0;j<(i+1)/2;j++)
	{
		temp=argarr[j];
		argarr[j]=argarr[i-j];
		argarr[i-j]=temp;

	}

	return i+1;
}
int Int32ToCharArray(int data,char* argarr)
{
	int i,j;
	i=1;
	int datacpy=0;
	int temp=0;
	if(data>0)
	{
		datacpy=data;
		argarr[0]=0;
	}
	else
	{
		datacpy=-data;
		argarr[0]=1;
	}
	while(datacpy/10)
	{
		temp=datacpy%10;
		argarr[i]=temp;
		datacpy=datacpy/10;
		i++;
	}
	argarr[i]=datacpy;


	for(j=1;j<i/2+1;j++)
	{
		temp=argarr[j];
		argarr[j]=argarr[i-j+1];
		argarr[i-j+1]=temp;

	}
return i+1;
}
int TCP_get_param( char* param_buffer, int* argarr)
{

	char* ptr = NULL;
	char item[16];
	int i,item_length = 0;
	memset(item,'\0',16);
	ptr=param_buffer+1;

	for( i=0;*ptr!='\0';i++)
	{
		while(*ptr!=' '&&*ptr!='\0')
		{
			ptr++;
			item_length++;
		}
		memcpy(item,ptr-item_length,item_length);
		argarr[i]=atoi(item);
		memset(item,'\0',16);
		item_length=0;
		if(*ptr!='\0')
			ptr++;
	}

	return 1;
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


int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int i;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
	{

		switch (ch) {
		case	'c':
			config_flag = 1;
			break;
		case	'n':
			set_noise_filter_flag = 1;
			noise_filter = atoi(optarg);
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

*/
//			printf("%s",optarg);
			chroma_scale_flag = 1;
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
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}
	return 0;
}

int fd_iav;
int map_buffer(void)
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

	//printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
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
	u8 * output_v = output_uv;
	u8 * output_u = output_uv + width * height;	//without padding

	for (row = 0; row < height; row++) { 		//row
		for (i = 0; i < width; i++) {			//interlaced UV row
			*output_u++ = *input_uv++;   	//U Buffer
			*output_v++ =  *input_uv++;	//V buffer
		}
		input_uv += (pitch - width) * 2;		//skip padding
	}
	return 0;
}
int get_preview_buffer(int fd)
{
	iav_yuv_buffer_info_ex_t preview_buffer_info;
	int uv_pitch;
	const int yuv_format = 1;  // 0 is yuv422, 1 is yuv420
	int quit_yuv_stream=0;
	while (!quit_yuv_stream)
	{
		preview_buffer_info.source= 1;
		if (ioctl(fd_iav, IAV_IOC_READ_YUV_BUFFER_INFO_EX, &preview_buffer_info) < 0)
		{
			if (errno == EINTR)
			{
				continue;		/* back to for() */
			} else
			{
				perror("IAV_IOC_READ_YUV_BUFFER_INFO_EX");
				return -1;
			}
		}
		if (preview_buffer_info.pitch == preview_buffer_info.width)
		{
			memcpy(y_buffer_clipped, preview_buffer_info.y_addr, preview_buffer_info.width * preview_buffer_info.height);
		}
		else if (preview_buffer_info.pitch > preview_buffer_info.width)
		{
			remove_padding_from_pitched_y(y_buffer_clipped, preview_buffer_info.y_addr, preview_buffer_info.pitch, preview_buffer_info.width, preview_buffer_info.height);
		} else
		{
			printf("pitch size smaller than width!\n");
			return -1;
		}

		//convert uv data from interleaved into planar format
		if (yuv_format == 1)
		{
			uv_pitch = preview_buffer_info.pitch / 2;
			uv_width = preview_buffer_info.width / 2;
			uv_height = preview_buffer_info.height / 2;
		}
		else
		{
			uv_pitch = preview_buffer_info.pitch / 2;
			uv_width = preview_buffer_info.width / 2;
			uv_height = preview_buffer_info.height;
		}
		remove_padding_and_deinterlace_from_pitched_uv(uv_buffer_clipped,
			preview_buffer_info.uv_addr, uv_pitch, uv_width, uv_height);
		quit_yuv_stream=1;
	}
//	printf("get_preview_buffer done\n");
//	printf("Output YUV resolution %d x %d in YV12 format\n", preview_buffer_info.width, preview_buffer_info.height);
	return 0;
}
static int send_preview_buffer(int fd_sock)
{
	int rev;
	char char_w_len[10];
	char char_h_len[10];

	int y_buffer_w = 2*uv_width ;
	int w_len = Int32ToCharArray(y_buffer_w,char_w_len);
	send(fd_sock,char_w_len,w_len,0);
	send(fd_sock," ",1,0);

	int y_buffer_h=2*uv_height ;
	int h_len=Int32ToCharArray(y_buffer_h,char_h_len);
	send(fd_sock,char_h_len,h_len,0);
	send(fd_sock," ",1,0);
	send(fd_sock,"!",1,0);

	rev=send(fd_sock,y_buffer_clipped,y_buffer_w*y_buffer_h,0);
	if(rev<0)
		printf("send y fail\n");
	rev=send(fd_sock,uv_buffer_clipped, uv_width*uv_height*2, 0);
	if(rev<0)
		printf("send uv fail\n");
	return rev;
}
static int get_raw(void)
{
	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info) < 0)
	{
		perror("IAV_IOC_READ_RAW_INFO");
		return -1;
	}
	printf("raw_addr = %p\n", raw_info.raw_addr);
	printf("bit_resolution = %d\n", raw_info.bit_resolution);
	printf("resolution: %dx%d\n", raw_info.width, raw_info.height);

	return 0;
}
static int send_raw_stream(int fd_sock)
{
	int rev;
	rev=send(fd_sock,raw_info.raw_addr, raw_info.width * raw_info.height * 2,0);
	if(rev<=0)
	{
		printf("send raw stream fail\n");
	}
	return rev;
}
static int send_jpeg_stream(int fd_sock)
{
	int rev;
	rev=send(fd_sock,bsb_mem,bsb_size,0);
	if(rev<=0)
	{
		printf("send jpeg stream fail!\n");
	}
	return rev;
}
static void send_pic_stream(int fd_sock)
{
	char char_raw_len[10];
	int stream_len=raw_info.width * raw_info.height * 2;
	int len=Int32ToCharArray(stream_len,char_raw_len);
	send(fd_sock,char_raw_len,len,0);
	send(fd_sock," ",1,0);
	//int rev1,rev2;
	//rev1=send_raw_stream(fd_sock);
	send_raw_stream(fd_sock);

	//rev2=send_jpeg_stream(fd_sock);
	send_jpeg_stream(fd_sock);
	send(fd_sock,"!",1,0);

}
static int get_jpeg_stream(void)
{
	int	rval = 0;
	memset(&jpg_bs_info, 0, sizeof(jpg_bs_info));

	/* Read bit stream descriptiors */
	rval = ioctl(fd_iav, IAV_IOC_READ_BITSTREAM, &jpg_bs_info);
	if (rval < 0) {
		perror("IAV_IOC_READ_BITSTREAM");
		goto save_jpeg_stream_exit;
	}

	if (jpg_bs_info.count) {
		printf("Read bitstream #: %d\n", jpg_bs_info.count);
	} else {
		printf("No bitstream available!\n");
		goto save_jpeg_stream_exit;
	}
save_jpeg_stream_exit:
	return rval;
}
static int send_dump_info(int fd_sock,u8*dump_buffer,int buffer_size)
{
	char char_dump_size[10];
	int len=Int32ToCharArray(buffer_size,char_dump_size);
	send(fd_sock,char_dump_size,len,0);
	send(fd_sock," ",1,0);

	int rev;
	rev=send(fd_sock,dump_buffer, buffer_size,0);
	if(rev<=0)
	{
		printf("send idsp dump info fail\n");
	}
	return rev;
}
static int send_buffer(int fd_sock, u8* buffer,int buffer_size)
{
	char char_buffer_size[10];
	int len=Int32ToCharArray(buffer_size,char_buffer_size);
	send(fd_sock,char_buffer_size,len,0);
	send(fd_sock," ",1,0);

	int rev;
	rev=send(fd_sock,buffer, buffer_size,0);
	if(rev<=0)
	{
		printf("send buffer fail\n");
	}
	return rev;
}
u16 gain_curve[NUM_CHROMA_GAIN_CURVE] = {256, 299, 342, 385, 428, 471, 514, 557, 600, 643, 686, 729, 772, 815, 858, 901,
	 936, 970, 990,1012,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1012, 996, 956, 916, 856, 796, 736, 676, 616, 556, 496, 436, 376, 316, 256};
static int info_mode =0;
int s_info_recv(int sock,char* buffer)
{
	recv(sock,buffer,2,0);
	if(buffer[0] =='2')
	{
		if(buffer[1]=='1')
		{
			info_mode =1;
			return 2;
		}
		else if(buffer[1]=='2')
		{
			info_mode =2;
			return 2;
		}
		else
		{
			info_mode = 2;
			return 2;
		}
	}
	else if(buffer[0]=='1')
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


static int situation=0;

void handle_pipe(int sig)
{
        situation=1;
}
void* threadproc_2()
{

/*	struct sigaction action;
	action.sa_handler = handle_pipe;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGPIPE, &action, NULL);*/
//	signal(SIGPIPE, SIG_IGN);
//	printf("threadproc_2\n");
	int sock_thread = -1,newfd_thread = -1;
	fd_set rfds;
	struct timeval tv;
	int retval,maxfd=-1;
//	int IsFirst=1;
	if((sock_thread=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		exit(1);
	}
	int on=2048;//ret;
	struct sockaddr_in  my_addr_thread,their_addr_thread;

	//ret = setsockopt( sock_thread, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int) );
	setsockopt( sock_thread, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int) );
	memset( &my_addr_thread, 0, sizeof(my_addr_thread) );
	my_addr_thread.sin_family=AF_INET;
	my_addr_thread.sin_port=htons(8000);
	my_addr_thread.sin_addr.s_addr=INADDR_ANY;
	bzero(&(my_addr_thread.sin_zero),0);

	if(bind(sock_thread,(struct sockaddr*)&my_addr_thread,sizeof(struct sockaddr))==-1)
	{
		perror("bind");
		exit(1);
	}
	if(listen(sock_thread,10)==-1)
	{
		perror("listen");
		exit(1);
	}
//	int flag=0;
	while(1)
	{

WaitForConnection:
		printf("Waiting for connection!\n");
		socklen_t s_size=sizeof(struct sockaddr_in);
		if((newfd_thread=accept(sock_thread,(struct sockaddr*)&their_addr_thread,&s_size))==-1)
		{
			perror("accapt");
			continue;
		}
		situation=0;
		printf("Connect successfully!\n");
		char pre_rev_thread[4];
		int r=0;
		//int flag=0;
		while(1)
		{
			struct sigaction action;
			action.sa_handler = handle_pipe;
			sigemptyset(&action.sa_mask);
			action.sa_flags = 0;
			sigaction(SIGPIPE, &action, NULL);

//			printf("situation %d\n",situation);
			if(situation==1)
			{
				if (newfd_thread != -1)
					close(newfd_thread);
				break;
			}
			FD_ZERO(&rfds);
			maxfd=0;
			FD_SET(newfd_thread,&rfds);
			if(newfd_thread>maxfd)
				maxfd=newfd_thread;
			tv.tv_sec=1;
			tv.tv_usec=0;
			retval=select(maxfd+1,&rfds,NULL,NULL,&tv);
//			printf("select retval %d\n",retval);
			if(retval==-1)
			{
				printf("select error!%s!\n",strerror(errno));
				break;
			}
			else if(retval==0)
			{
				//printf("no recv now!\n");
				continue;
			//	goto haha;
			}
			else
			{
				if (FD_ISSET(newfd_thread, &rfds))
				{
					r=s_info_recv(newfd_thread,pre_rev_thread);
//					printf("r=%d",r);
					if(r==0)
					{
						if(1)//////////////////to be improved
						{
							printf("Get Recv, and stop!\n");
							break;
						}
					//	continue;

					}
					else if(r==1)
					{
						printf("3A info disvisible!!\n");
						goto WaitForConnection;

					}
					else if(r==2)
					{
					//	printf("Get recv, and begin to send info...\n");
						img_dsp_get_statistics(fd_iav,& aaa_tile_get_info, aaa_ae_info, aaa_awb_info, aaa_af_info, &aaa_hist_info,&act_tile,aaa_af_info2,&dsp_histo_info);
					//	printf("info_mode =%d\n",info_mode);
						if(info_mode ==1)
						{
							int i=0,j=0,k=0,t=0,n=0;
							int thread_aaainfo[2000];
							for( k=0;k<40;k++)
							{
								thread_aaainfo[k]=aaa_af_info[k].sum_fv2;
							}
							for( i=0;i<96;i++)
							{
								thread_aaainfo[k+i]=aaa_ae_info[i].lin_y;
							}
							for(j=0;j<384;j++)
							{
								thread_aaainfo[k+i+j]=aaa_awb_info[j].r_avg;
							}
							for(t=0;t<384;t++)
							{
								thread_aaainfo[k+i+j+t]=aaa_awb_info[t].g_avg;
							}
							for(n=0;n<384;n++)
							{
								thread_aaainfo[k+i+j+t+n]=aaa_awb_info[n].b_avg;
							}

							char aaainfo[10];
						       	for(i=0;i<NUM_AAAINFO;i++)
							{
								int k;
								k=U32ToCharArray(thread_aaainfo[i],aaainfo);
								char n_aaainfo[1];
								n_aaainfo[0]=k;
								if (situation == 1)
									break;
								send(newfd_thread,n_aaainfo,1,0);
								send(newfd_thread,aaainfo,k,0);
							}
						//	printf("send info time:%d\n",++k_num);
							//flag=1;
						}
						else if(info_mode ==2)
						{
						//	send(newfd_thread,aaa_hist_info.hist_bin_data,4*sizeof(aaa_hist_info.hist_bin_data),0);
						//	printf("HAHA\n");
							int i=0;
							char sendhehe[10];
							for(i=0;i<256;i++)
							{
								int k;
								k=Int32ToCharArray(aaa_hist_info.hist_bin_data[i],sendhehe);
							//	printf("%d,",aaa_hist_info.hist_bin_data[i]);
								send(newfd_thread,sendhehe,k,0);
								send(newfd_thread," ",1,0);
							}
							send(newfd_thread,"!",1,0);
						}
					}
					else
					{
						printf("some thing superised!\n");

					}

				}
			}

		}
	}
	if (sock_thread != -1)
		close(sock_thread);
	return NULL;
}
void* preview_proc()
{
	int sock_pre=-1,sock_client;
	if((sock_pre=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket\n");
		exit(1);
	}
	int on=2048;//ret;
	struct sockaddr_in  here_addr,their_addr;

	//ret = setsockopt( sock_pre, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int) );
	setsockopt( sock_pre, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int) );
	memset( &here_addr, 0, sizeof(here_addr) );
	here_addr.sin_family=AF_INET;
	here_addr.sin_port=htons(7000);
	here_addr.sin_addr.s_addr=INADDR_ANY;
	bzero(&(here_addr.sin_zero),0);

	if(bind(sock_pre,(struct sockaddr*)&here_addr,sizeof(struct sockaddr))==-1)
	{
		perror("bind");
		exit(1);
	}
	if(listen(sock_pre,10)==-1)
	{
		perror("listen");
		exit(1);
	}
	while(1)
	{
		signal(SIGFPE, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		socklen_t s_size=sizeof(struct sockaddr_in);
		if((sock_client=accept(sock_pre,(struct sockaddr*)&their_addr,&s_size))==-1)
		{
			perror("accapt");
			continue;
		}
/*		if(map_buffer()<0)
		{
			printf("map buffer fail!\n");
		}*/
		printf("open preview!\n");
//		int n=1;
		myflag=1;
		y_buffer_clipped = malloc(MAX_YUV_BUFFER_SIZE);
		uv_buffer_clipped = malloc(MAX_YUV_BUFFER_SIZE);
		while(1&&myflag)
		{
		//	printf("%d frame\n",n++);
			memset(y_buffer_clipped,0,MAX_YUV_BUFFER_SIZE);
			memset(uv_buffer_clipped,0,MAX_YUV_BUFFER_SIZE);

			if(get_preview_buffer(fd_iav)==-1)
				continue;
			if(send_preview_buffer(sock_client)<0)
				break;
		//	printf("%d frame\n",n++);
			usleep(1000*50);
			continue;
		//	break;
		/*	reval=s_info_recv(sock_client,re);
			printf("reval=%d\n",reval);
			if(reval>0)
				continue;
			else
				printf("not preview recv!\n");
		*/
		}
		free(y_buffer_clipped);
		free(uv_buffer_clipped);
		close(sock_client);
	}
	return NULL;
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
	color_corr_reg.reg_setting_addr=(u32)reg;
	color_corr.matrix_3d_table_addr =(u32)matrix;
//	img_dsp_load_color_correction_table((u32)reg, (u32)matrix);
	img_dsp_set_color_correction_reg(&color_corr_reg);
	img_dsp_set_color_correction(fd_iav,&color_corr);
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
int main(int argc, char **argv)
{
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


	if (init_param(argc, argv) < 0)
		return -1;

	u32  frame_rate;
	u32 vin_mode;

	sensor_label sensor_lb;
	int yuv_type_vin_flag;
	if(start_aaa)
	{
		char sensor_name[32];
		struct amba_vin_source_info vin_info;
		struct amba_video_info video_info;
		if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
			printf("IAV_IOC_VIN_SRC_GET_INFO error\n");
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
			if (vin_mode == AMBA_VIDEO_MODE_720P || vin_mode == AMBA_VIDEO_MODE_AUTO) {
				if (frame_rate == AMBA_VIDEO_FPS_25 || frame_rate == AMBA_VIDEO_FPS_30)
					app_param_image_sensor.p_ae_sht_dgain = ov9715_ae_sht_dgain_720p_30;
				else if (frame_rate == AMBA_VIDEO_FPS_29_97)
					app_param_image_sensor.p_ae_sht_dgain = ov9715_ae_sht_dgain_720p_29_97;
			} else
				app_param_image_sensor.p_ae_sht_dgain = ov9715_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = ov9715_dlight;
			app_param_image_sensor.p_manual_LE = ov9715_manual_LE;
			sprintf(sensor_name, "ov9715");
			break;
                case SENSOR_OV9718:
                        sensor_lb = OV_9718;
                        app_param_image_sensor.p_adj_param = &ov9718_adj_param;
                        app_param_image_sensor.p_rgb2yuv = ov9718_rgb2yuv;
                        app_param_image_sensor.p_chroma_scale = &ov9718_chroma_scale;
                        app_param_image_sensor.p_awb_param = &ov9718_awb_param;
                        app_param_image_sensor.p_50hz_lines = ov9718_50hz_lines;
                        app_param_image_sensor.p_60hz_lines = ov9718_60hz_lines;
                        app_param_image_sensor.p_tile_config = &ov9718_tile_config;
                        app_param_image_sensor.p_ae_agc_dgain = ov9718_ae_agc_dgain;
                        app_param_image_sensor.p_ae_sht_dgain = ov9718_ae_sht_dgain;
                        app_param_image_sensor.p_dlight_range = ov9718_dlight;
                        app_param_image_sensor.p_manual_LE = ov9718_manual_LE;
                        sprintf(sensor_name, "ov9718");
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
			app_param_image_sensor.p_awb_param = &imx122_awb_param;
			app_param_image_sensor.p_50hz_lines = imx122_50hz_lines;
			app_param_image_sensor.p_60hz_lines = imx122_60hz_lines;
			app_param_image_sensor.p_tile_config = &imx122_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = imx122_ae_agc_dgain;
			if (vin_mode == AMBA_VIDEO_MODE_AUTO || vin_mode == AMBA_VIDEO_MODE_1080P) {
				if (frame_rate == AMBA_VIDEO_FPS_29_97 || frame_rate == AMBA_VIDEO_FPS_59_94)
					app_param_image_sensor.p_ae_sht_dgain = imx122_ae_sht_dgain_1080p29_97;
				else if (frame_rate == AMBA_VIDEO_FPS_25 || frame_rate == AMBA_VIDEO_FPS_30 || frame_rate == AMBA_VIDEO_FPS_60)
					app_param_image_sensor.p_ae_sht_dgain = imx122_ae_sht_dgain_1080p25;
				else
					app_param_image_sensor.p_ae_sht_dgain = imx122_ae_sht_dgain;
			} else
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
			if (vin_mode == AMBA_VIDEO_MODE_QXGA) {
				app_param_image_sensor.p_ae_sht_dgain =mt9t002_ae_sht_dgain_2048x1536_25;
			} else if (vin_mode == AMBA_VIDEO_MODE_1080P) {
				app_param_image_sensor.p_ae_sht_dgain = mt9t002_ae_sht_dgain_1080p30;
			} else if(frame_rate == AMBA_VIDEO_FPS_29_97&&vin_mode == AMBA_VIDEO_MODE_2304x1296) {
				printf("3m30\n");
				app_param_image_sensor.p_ae_sht_dgain = mt9t002_ae_sht_dgain_2304x1296;
			} else
				app_param_image_sensor.p_ae_sht_dgain = mt9t002_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = mt9t002_dlight;
			app_param_image_sensor.p_manual_LE = mt9t002_manual_LE;
			sprintf(sensor_name, "mt9t002");
			break;
		case SENSOR_AR0331:
			sensor_lb = AR_0331;
			amba_vin_sensor_op_mode op_mode;
			if (ioctl(fd_iav, IAV_IOC_VIN_GET_OPERATION_MODE, &op_mode) < 0) {
				perror("IAV_IOC_VIN_GET_OPERATION_MODE");
				return -1;
			}
			if(op_mode == AMBA_VIN_LINEAR_MODE)
			{
				app_param_image_sensor.p_adj_param = &ar0331_linear_adj_param;
				vin_op_mode = 0;
				app_param_image_sensor.p_awb_param = &ar0331_linear_awb_param;
				sprintf(sensor_name, "ar0331_linear");
			}
			else
			{
				app_param_image_sensor.p_adj_param = &ar0331_adj_param;
				vin_op_mode = 1;	//HDR mode
				app_param_image_sensor.p_awb_param = &ar0331_awb_param;
				sprintf(sensor_name, "ar0331");
			}
			app_param_image_sensor.p_rgb2yuv = ar0331_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &ar0331_chroma_scale;

			app_param_image_sensor.p_50hz_lines = ar0331_50hz_lines;
			app_param_image_sensor.p_60hz_lines = ar0331_60hz_lines;
			app_param_image_sensor.p_tile_config = &ar0331_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = ar0331_ae_agc_dgain;
			app_param_image_sensor.p_ae_sht_dgain = ar0331_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = ar0331_dlight;
			app_param_image_sensor.p_manual_LE = ar0331_manual_LE;

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
			app_param_image_sensor.p_ae_sht_dgain = ar0130_ae_sht_dgain_720p30;
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

			if (frame_rate == AMBA_VIDEO_FPS_29_97) {
				app_param_image_sensor.p_ae_sht_dgain = imx104_ae_sht_dgain_29_97;
			} else if (frame_rate == AMBA_VIDEO_FPS_25) {
				app_param_image_sensor.p_ae_sht_dgain = imx104_ae_sht_dgain_25;
			} else
				app_param_image_sensor.p_ae_sht_dgain = imx104_ae_sht_dgain;

			app_param_image_sensor.p_dlight_range =imx104_dlight;
			app_param_image_sensor.p_manual_LE = imx104_manual_LE;
			sprintf(sensor_name, "imx104");
			break;
		default:
			if (yuv_type_vin_flag)
			{
				printf("YUV video input type, skip sensor related config \n");
			}
			else
			{
				return -1;
			}
	}

	if (!yuv_type_vin_flag) {
		if (img_config_sensor_info(sensor_lb) < 0) {
			return -1;
		}
		printf("vin_op_mode = %d\n", vin_op_mode);
		if (img_config_sensor_hdr_mode(vin_op_mode) < 0) {
			return -1;
		}
		img_config_lens_info(LENS_CMOUNT_ID);
		img_lens_init();
	}
	load_dsp_cc_table();
	load_adj_cc_table(sensor_name);

	img_load_image_sensor_param(&app_param_image_sensor);
	if(img_start_aaa(fd_iav))
		return -1;
	}

	u8 reg[18752], matrix[17536];
	enum step1
	{
		OnlineTuning=0,
		Caribration,
		AutoTest
	};
	enum step2
	{
		Color=0,
		Noise,
		AAA
	};
	enum colorprocessing
	{
		BlackLevelCorrection=0,
		ColorCorrection,
		ToneCurve,
		RGBtoYUVMatrix,
		WhiteBalanceGains,
		DGainSaturaionLevel,
		LocalExposure,
		ChromaScale
	};
	enum noiseprocessing
	{
		FPNCorrection=0,
		BadPixelCorrection,
		CFALeakageFilter,
		AntiAliasingFilter,
		CFANoiseFilter,
		ChromaMedianFiler,
		SharpeningControl,
		MCTFControl
	};
	enum aaaprocessing
	{
		ConfigAAAControl=0,
		AETileConfiguration,
		AWBTilesConfiguration,
		AFTileConfiguration,
		AFStatisticSetupEx,
		ExposureControl
	};
	enum load_apply
	{
		Load=0,
		Apply=1
	};

	int sockfd,on=2048;//ret;
	int new_fd;
	struct dataBuffer{
		int  layer1;
		int layer2;
		int layer3;

	}data;

	if(map_buffer()<0)
	{
		printf("map buffer fail!\n");
	}
	sockfd=-1;
	new_fd=-1;
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		exit(1);
	}
	//on = 2048;
	//ret = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int) );
	setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int) );

	struct sockaddr_in  my_addr,their_addr;
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(6000);
	my_addr.sin_addr.s_addr=INADDR_ANY;
	bzero(&(my_addr.sin_zero),0);

	if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr))==-1)
	{
		perror("bind");
		exit(1);
	}
	if(listen(sockfd,10)==-1)
	{
		perror("listen");
		exit(1);
	}
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	//Create a new thread
	pthread_t thread_3A_S;
	int err;
	int r_get_param=0;
	if((err=pthread_create(&thread_3A_S,NULL,threadproc_2,NULL)!=0))
		printf("create a new thread fail!\n");
/*	int pid = fork();
	if (pid == 0)
	{
		threadproc_2();
	}
*/
//	pthread_exit(thread);
	pthread_t thread_preview;
	if((err=pthread_create(&thread_preview,NULL,preview_proc,NULL)!=0))
		printf("create a new thread fail!\n");

	////////create over
	//////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////
	while(1)
	{
		if(new_fd!=-1)
			close(new_fd);
		socklen_t sin_size=sizeof(struct sockaddr_in);
		signal(SIGFPE, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		if((new_fd=accept(sockfd,(struct sockaddr*)&their_addr,&sin_size))==-1)
		{
			perror("accapt");
			continue;
		}

		char pre_revBuffer[4];
		recv(new_fd,pre_revBuffer,4,0);
//		printf("%s\n",pre_revBuffer);

		int id_Require;
		char ch;
		ch=pre_revBuffer[0];
		id_Require=atoi(&ch);

		ch=pre_revBuffer[1];
		data.layer1=atoi(&ch);
		ch=pre_revBuffer[2];
		data.layer2=atoi(&ch);
		ch=pre_revBuffer[3];
		data.layer3=atoi(&ch);
		if(id_Require == 8)//for ip testing
		{
			if(new_fd!=-1)
				close(new_fd);
			printf("Available IP addess!\n");
			continue;
		}
		if(id_Require == 7)// for still capture
		{
			still_cap_info_t still_cap_info;
			memset(&still_cap_info, 0, sizeof(still_cap_info));
			still_cap_info.capture_num = 1;
			still_cap_info.need_raw = 1;
			img_init_still_capture(fd_iav, 95);
			img_start_still_capture(fd_iav, &still_cap_info);
			get_raw();
			get_jpeg_stream();
			img_stop_still_capture(fd_iav);
			send_pic_stream(new_fd);
			if (ioctl(fd_iav, IAV_IOC_LEAVE_STILL_CAPTURE) < 0)
			{
				perror("IAV_IOC_LEAVE_STILL_CAPTURE");
				return -1;
			}
			printf("Still capture over!\n");
			continue;
		}
		if(id_Require==6)//for idsp_dump
		{
			int id_section;
			if(data.layer1<8)
				id_section=data.layer1;
			else if(data.layer1==8)
				id_section=100;
			else
			{
				printf("unknow id_section!\n");
				continue;
			}
			printf("id_section=%d\n",id_section);
			u8* dump_buffer=malloc(MAX_DUMP_BUFFER_SIZE);	//MAX_DUMP_BUFFER_SIZE
			int buffer_size=0;
			buffer_size=img_dsp_dump( fd_iav, id_section,dump_buffer,MAX_DUMP_BUFFER_SIZE);
			if(buffer_size==-1)
			{
				printf("malloc too few buffer!\n");
				continue;
			}
			int fp = open("dump_info.bin", O_WRONLY | O_CREAT, 0666);
			if (write(fp, dump_buffer, buffer_size) < 0)
			{
				perror("write(5)");
				return -1;
			}
			close(fp);
			send_dump_info(new_fd,dump_buffer,buffer_size);
			free(dump_buffer);
			printf("Idsp_dump over!\n");
			continue;
		}
		if(id_Require==4)
		{
			myflag=0;
			printf("Close Preview!\n");
			continue;
		}
		if(id_Require ==2)//bw <=>color
		{
			if(data.layer1 ==0)
			{

				img_set_bw_mode(0);
				printf("bw disable\n");
			}
			else if(data.layer1 ==1)
			{
				img_set_bw_mode(1);
				printf("bw enable\n");

			}
			continue;
		}

		char recvBuffer[2000];
		int dataArray[1000];
		int sendBuffer[1000];
		char sendCharArray[100];

		int i;
		if(id_Require==Apply)//apply
		{
			printf("Apply!\n");
			int IsCC=0;
			int IsFPN = 0;
			if(data.layer1==0&&data.layer2==0&&data.layer3==1)
			{
				IsCC=1;
			}
			else if(data.layer1==0&&data.layer2==1&&data.layer3==0)
			{
				IsFPN = 1;
				int flag =1;
				char temp[15];
				int bytes = 0;
				u32 raw_pitch;
				while( flag)
				{
					  int rbyte = recv(new_fd,temp+bytes, 1, 0);
					  bytes += rbyte;
				                if (temp[bytes - 1] != 33)
				                    continue;
				                else
				                    break;
				}
				TCP_get_param( temp,  dataArray);
				bad_corr.dark_pixel_strength = 0;
				bad_corr.hot_pixel_strength = 0;
				bad_corr.enable = 0;
				memset(&cfa_noise_filter, 0, sizeof(cfa_noise_filter));
				img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
				img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
				img_dsp_set_anti_aliasing(fd_iav, 0);

				if(bytes <5)//Detection
				{
					recv(new_fd,recvBuffer,2000,0);
					TCP_get_param( recvBuffer,  dataArray);
					badpixel_detect_algo.cap_width = dataArray[0];
					u32 width = dataArray[0];
					badpixel_detect_algo.cap_height= dataArray[1];
					u32 height = dataArray[1];
					badpixel_detect_algo.block_w= dataArray[2];
					badpixel_detect_algo.block_h = dataArray[3];
					badpixel_detect_algo.upper_thres = dataArray[4];
					badpixel_detect_algo.lower_thres= dataArray[5];
					badpixel_detect_algo.agc_idx= dataArray[6];
					badpixel_detect_algo.shutter_idx = dataArray[7];
					badpixel_detect_algo.badpix_type= dataArray[8];
					badpixel_detect_algo.detect_times= dataArray[9];

					raw_pitch = ROUND_UP(width/8, 32);
					u32 fpn_map_size = raw_pitch*height;
					u8* fpn_map_addr = malloc(fpn_map_size);
					memset(fpn_map_addr,0,(fpn_map_size));
					badpixel_detect_algo.badpixmap_buf = fpn_map_addr;
					badpixel_detect_algo.cali_mode = 0; // 0 for video, 1 for still

					int bpc_num = img_cali_bad_pixel(fd_iav, &badpixel_detect_algo);
					printf("Total number is %d\n", bpc_num);
					send_buffer(new_fd,fpn_map_addr,fpn_map_size);
					free(fpn_map_addr);
					if (ioctl(fd_iav, IAV_IOC_ENABLE_PREVIEW) < 0)
					{
						perror("IAV_IOC_ENABLE_PREVIEW");
						return -1;
					}
					printf("FPN Detection!\n");
					continue;

				}
				else//Correction
				{
					fpn.pixel_map_width = dataArray[0];
					fpn.pixel_map_height = dataArray[1];
					raw_pitch = ROUND_UP((fpn.pixel_map_width/8), 32);
					u32 fpn_map_size = raw_pitch*fpn.pixel_map_height ;
					fpn.enable = 3;
					fpn.fpn_pitch = raw_pitch;
					fpn.pixel_map_size = fpn_map_size;
					printf("pixel map w = %d,h =%d,%d\n",dataArray[0],dataArray[1],fpn_map_size);
				}
			}
			if(IsCC!=1&& IsFPN != 1)
			{
				recv(new_fd,recvBuffer,2000,0);
			//	printf("%s\n",recvBuffer);

			}
			switch(data.layer1)
			{

			case OnlineTuning:
			//	printf("Online Tuning!\n");
				switch(data.layer2)
				{

					case Color:
					//	printf("Color Processing!\n");
						switch(data.layer3)
						{
							case BlackLevelCorrection:
								if((r_get_param=TCP_get_param( recvBuffer,  dataArray))!=1)
									printf("Tcp get param error!\n");
								blc.r_offset = dataArray[0];
								blc.gr_offset =dataArray[1];
								blc.gb_offset =dataArray[2];
								blc.b_offset = dataArray[3];

								img_dsp_set_global_blc(fd_iav, &blc,GB);
								printf("Black Level Correction!\n");
								break;
							case ColorCorrection:
								recv(new_fd,reg,18752,0);
								recv(new_fd,matrix,17536,0);
								color_corr_reg.reg_setting_addr=(u32)reg;
								color_corr.matrix_3d_table_addr =(u32)matrix;
							   	img_dsp_set_color_correction_reg(&color_corr_reg);
								img_dsp_set_color_correction(fd_iav,&color_corr);
								img_dsp_enable_color_correction(fd_iav);
							//	img_dsp_set_tone_curve(fd_iav,&tone_curve);
								printf("Color Correction!\n");

								break;
							case ToneCurve:
								TCP_get_param( recvBuffer,  dataArray);

								for( i=0;i<NUM_EXPOSURE_CURVE;i++)
								{
									tone_curve.tone_curve_red[i]= dataArray[i];
									tone_curve.tone_curve_green[i]= dataArray[i];
									tone_curve.tone_curve_blue[i]= dataArray[i];

								}

								img_dsp_set_tone_curve(fd_iav,&tone_curve);
								printf("Tone Curve!\n ");

								break;
							case RGBtoYUVMatrix:
								TCP_get_param( recvBuffer,  dataArray);
								rgb2yuv_matrix.matrix_values[0]=dataArray[0];
								rgb2yuv_matrix.matrix_values[1]=dataArray[1];
								rgb2yuv_matrix.matrix_values[2]=dataArray[2];
								rgb2yuv_matrix.matrix_values[3]=dataArray[3];
								rgb2yuv_matrix.matrix_values[4]=dataArray[4];
								rgb2yuv_matrix.matrix_values[5]=dataArray[5];
								rgb2yuv_matrix.matrix_values[6]=dataArray[6];
								rgb2yuv_matrix.matrix_values[7]=dataArray[7];
								rgb2yuv_matrix.matrix_values[8]=dataArray[8];
								rgb2yuv_matrix.y_offset = dataArray[9];
								rgb2yuv_matrix.u_offset = dataArray[10];
								rgb2yuv_matrix.v_offset = dataArray[11];
								brightness  = dataArray[12];
								contrast  = dataArray[13];
								saturation  = dataArray[14];
								hue  = dataArray[15];
								adj_set_saturation(saturation);
								adj_set_contrast( contrast);
								adj_set_brightness(brightness);
								adj_set_hue(hue);
								adj_set_color_conversion(&rgb2yuv_matrix);
								img_dsp_set_rgb2yuv_matrix(fd_iav, &rgb2yuv_matrix);
								printf("RGB to YUV Matrix!\n");


								break;

							case WhiteBalanceGains:

								TCP_get_param( recvBuffer,  dataArray);
								wb_gain.r_gain = dataArray[0];
								wb_gain.g_gain =dataArray[1];
								wb_gain.b_gain = dataArray[2];

								img_dsp_set_wb_gain(fd_iav, &wb_gain);
		 						printf("White Balance !\n");

								break;
							case DGainSaturaionLevel:
								TCP_get_param( recvBuffer,  dataArray);
	                                          				d_gain_satuation_level.level_red=dataArray[0];
								d_gain_satuation_level.level_green_even=dataArray[1];
								d_gain_satuation_level.level_green_odd=dataArray[2];
								d_gain_satuation_level.level_blue=dataArray[3];

								img_dsp_set_dgain_saturation_level( fd_iav, &d_gain_satuation_level);
								printf("DGain Saturation Level!\n");
								break;
							case LocalExposure:
								TCP_get_param( recvBuffer,  dataArray);
								local_exposure.enable = dataArray[0];
								local_exposure.radius = dataArray[1];
								local_exposure.luma_weight_shift = dataArray[2];
								local_exposure.luma_weight_red = dataArray[3];
								local_exposure.luma_weight_green = dataArray[4];
								local_exposure.luma_weight_blue = dataArray[5];

								for( i=0;i<NUM_EXPOSURE_CURVE;i++)
									local_exposure.gain_curve_table[i] = dataArray[i+6];
								img_dsp_set_local_exposure(fd_iav, &local_exposure);
								printf("Local Exposure!\n");


								break;
							case ChromaScale:
								TCP_get_param( recvBuffer,  dataArray);
								cs.enable = dataArray[0];

								for (i = 0;i<NUM_CHROMA_GAIN_CURVE;i++)
									cs.gain_curve[i] =dataArray[i+1];

								img_dsp_set_chroma_scale(fd_iav,&cs);
								printf("Chroma Scale!\n");
								break;

						}

						break;

					case Noise:
						switch(data.layer3)
							{
								case FPNCorrection:
								{
									u8* fpn_map_addr = NULL;
									fpn_map_addr = malloc(fpn.pixel_map_size);
									memset(fpn_map_addr,0, fpn.pixel_map_size);
									int bytes =0;
									while( bytes <fpn.pixel_map_size)
									{
										  int rbyte = recv(new_fd,fpn_map_addr+bytes, 2000, 0);
										  bytes += rbyte;
									}

									fpn.pixel_map_addr = (u32)fpn_map_addr;
									img_dsp_set_static_bad_pixel_correction(fd_iav,&fpn);
									free(fpn_map_addr);
									printf("FPN Correction!\n");
								}

								break;
								case BadPixelCorrection:
									TCP_get_param( recvBuffer,  dataArray);
									dbp_correction_setting.enable=dataArray[0];
									dbp_correction_setting.hot_pixel_strength=dataArray[1];
									dbp_correction_setting.dark_pixel_strength=dataArray[2];
									img_dsp_set_dynamic_bad_pixel_correction(fd_iav,&dbp_correction_setting);
									printf("Bad Pixel Correction!\n");

									break;
								case CFALeakageFilter:
									TCP_get_param( recvBuffer,  dataArray);

								      cfa_leakage_filter.enable=dataArray[0];
									cfa_leakage_filter.alpha_rr = dataArray[1];
									cfa_leakage_filter.alpha_rb = dataArray[2];
									cfa_leakage_filter.alpha_br = dataArray[3];
									cfa_leakage_filter.alpha_bb = dataArray[4];
									cfa_leakage_filter.saturation_level = dataArray[5];

									img_dsp_set_cfa_leakage_filter(fd_iav, &cfa_leakage_filter);
									printf("CFA Leakage Filter!\n");

									break;
								case AntiAliasingFilter:
									TCP_get_param( recvBuffer,  dataArray);
									anti_aliasing_strength=dataArray[0];
									img_dsp_set_anti_aliasing(fd_iav, anti_aliasing_strength);
									printf("Anti-Aliasing Filter!\n");

									break;
								case CFANoiseFilter:
									TCP_get_param( recvBuffer,  dataArray);
									cfa_noise_filter.enable = dataArray[0];

									cfa_noise_filter.iso_center_weight_red = dataArray[1];
									cfa_noise_filter.iso_center_weight_green = dataArray[2];
									cfa_noise_filter.iso_center_weight_blue = dataArray[3];
									cfa_noise_filter.iso_thresh_k0_red = dataArray[4];
									cfa_noise_filter.iso_thresh_k0_green = dataArray[5];
									cfa_noise_filter.iso_thresh_k0_blue = dataArray[6];
									cfa_noise_filter.iso_thresh_k0_close = dataArray[7];
									cfa_noise_filter.iso_thresh_k1_red = dataArray[8];
									cfa_noise_filter.iso_thresh_k1_green = dataArray[9];
									cfa_noise_filter.iso_thresh_k1_blue = dataArray[10];
									cfa_noise_filter.direct_center_weight_red = dataArray[11];
									cfa_noise_filter.direct_center_weight_green = dataArray[12];
									cfa_noise_filter.direct_center_weight_blue = dataArray[13];
									cfa_noise_filter.direct_thresh_k0_red = dataArray[14];
									cfa_noise_filter.direct_thresh_k0_green = dataArray[15];
									cfa_noise_filter.direct_thresh_k0_blue = dataArray[16];
									cfa_noise_filter.direct_thresh_k1_red = dataArray[17];
									cfa_noise_filter.direct_thresh_k1_green = dataArray[18];
									cfa_noise_filter.direct_thresh_k1_blue = dataArray[19];
									cfa_noise_filter.direct_grad_thresh = dataArray[20];
					//				printf("%d,%d,%d,%d,\n",cfa_noise_filter.enable,cfa_noise_filter.iso_center_weight_red,cfa_noise_filter.direct_thresh_k1_blue );
									img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
									printf("CFA Noise Filter!\n");
									break;
								case ChromaMedianFiler:
									TCP_get_param( recvBuffer,  dataArray);
									chroma_median_setup.enable = dataArray[0];
									chroma_median_setup.cb_str = dataArray[1];
									chroma_median_setup.cr_str = dataArray[2];
									img_dsp_set_chroma_median_filter(fd_iav, &chroma_median_setup);
									printf("Chroma Median Filer!\n");
									break;
								case SharpeningControl:
									TCP_get_param( recvBuffer,  dataArray);
									fir.fir_strength=dataArray[0];
									int i=0;
									int k=0;
									for(i = 0; i<10; i++)
										fir.fir_coeff[i] =dataArray[i+1];

									for(k = 0; k<256; k++){
										coring.coring[k]= dataArray[k+11];
									}
									luma_high_freq_noise_reduction_strength = dataArray[267];

									spatial_filter_info.isotropic_strength = dataArray[268];
									spatial_filter_info.directional_strength = dataArray[269];
									spatial_filter_info.edge_threshold = dataArray[270];

									retain_level = dataArray[271];

									max_change.max_change_down = dataArray[272];
									max_change.max_change_up = dataArray[273];

									sharpen_setting_min.low = dataArray[274];
									sharpen_setting_min.low_delta = dataArray[275];
									sharpen_setting_min.low_strength = dataArray[276];
									sharpen_setting_min.mid_strength = dataArray[277];
									sharpen_setting_min.high = dataArray[278];
									sharpen_setting_min.high_delta = dataArray[279];
									sharpen_setting_min.high_strength = dataArray[280];

									sharpen_setting_overall.low =dataArray[281];
									sharpen_setting_overall.low_delta = dataArray[282];
									sharpen_setting_overall.low_strength = dataArray[283];
									sharpen_setting_overall.mid_strength = dataArray[284];
									sharpen_setting_overall.high = dataArray[285];
									sharpen_setting_overall.high_delta = dataArray[286];
									sharpen_setting_overall.high_strength = dataArray[287];
									mode =dataArray[288];
									spatial_filter_info.max_change =dataArray[289];

									img_dsp_set_sharpen_fir_mode(fd_iav,mode);
									if(mode ==0)//make sure 0 for mode 0
									{
										fir.fir_coeff[6]=0;fir.fir_coeff[7]=0;
										fir.fir_coeff[8]=0;fir.fir_coeff[9]=0;
									}

									img_dsp_set_sharpen_fir(fd_iav,0, &fir); // mode == 0 for video mode
			                      			img_dsp_set_luma_high_freq_noise_reduction(fd_iav,luma_high_freq_noise_reduction_strength);
									img_dsp_set_sharpen_coring(fd_iav, 0,&coring);
									img_dsp_set_sharpen_signal_retain_level(fd_iav,0,retain_level);
									img_dsp_set_sharpen_max_change(fd_iav, 0,&max_change);
									img_dsp_set_sharpen_level_min(fd_iav,0,&sharpen_setting_min);
									img_dsp_set_sharpen_level_overall(fd_iav,0,&sharpen_setting_overall);
									img_dsp_set_spatial_filter(fd_iav, 0, &spatial_filter_info); // mode == 0 for video mode
									printf("Sharpening Control!\n");

									break;
								case MCTFControl:
									TCP_get_param( recvBuffer,  dataArray);
									mctf_info.enable = dataArray[0];
									mctf_info.alpha = dataArray[1];
									mctf_info.alpha_2 = dataArray[2];
									mctf_info.threshold_1 = dataArray[3];
									mctf_info.threshold_2 = dataArray[4];
									mctf_info.threshold_3 = dataArray[5];
									mctf_info.threshold_4 = dataArray[6];
									mctf_info.y_max_change = dataArray[7];
									mctf_info.u_max_change = dataArray[8];
									mctf_info.v_max_change = dataArray[9];
									mctf_info.padding = 0;
									img_dsp_set_video_mctf(fd_iav, &mctf_info);
									printf("MCTF Control!\n");

									break;

							}

						break;
					case AAA:
						switch(data.layer3)
							{
								case ConfigAAAControl:
									TCP_get_param( recvBuffer,  dataArray);
									img_enable_ae(dataArray[0]);
									img_enable_awb(dataArray[1]);

									img_enable_af(dataArray[2]);
									img_enable_adj(dataArray[3]);
								//	printf("%d,%d,%d,%d\n",dataArray[0],dataArray[1],dataArray[2],dataArray[3]);

									printf(" Config AAA Control!\n");
									break;
								case AETileConfiguration:
									TCP_get_param( recvBuffer,  dataArray);

									img_dsp_get_config_statistics_info( &aaa_statistic_config);
									aaa_statistic_config.ae_tile_num_col=dataArray[0];
									aaa_statistic_config.ae_tile_num_row=dataArray[1];
									aaa_statistic_config.ae_tile_col_start=dataArray[2];
									aaa_statistic_config.ae_tile_row_start=dataArray[3];
									aaa_statistic_config.ae_tile_width=dataArray[4];
									aaa_statistic_config.ae_tile_height=dataArray[5];
									aaa_statistic_config.ae_pix_min_value=dataArray[6];
									aaa_statistic_config.ae_pix_max_value=dataArray[7];

									aaa_statistic_config.enable=1;

									img_dsp_config_statistics_info(fd_iav,&aaa_statistic_config);
									printf("AE Tile Configuration!\n");
									break;
								case AWBTilesConfiguration:
									TCP_get_param( recvBuffer,  dataArray);

									img_dsp_get_config_statistics_info(&aaa_statistic_config);
									aaa_statistic_config.awb_tile_num_col=dataArray[0];
									aaa_statistic_config.awb_tile_num_row=dataArray[1];
									aaa_statistic_config.awb_tile_col_start=dataArray[2];
									aaa_statistic_config.awb_tile_row_start=dataArray[3];
									aaa_statistic_config.awb_tile_width=dataArray[4];
									aaa_statistic_config.awb_tile_height=dataArray[5];
									aaa_statistic_config.awb_pix_min_value=dataArray[6];
									aaa_statistic_config.awb_pix_max_value=dataArray[7];
									aaa_statistic_config.awb_tile_active_width=dataArray[8];
									aaa_statistic_config.awb_tile_active_height=dataArray[9];

									aaa_statistic_config.enable=1;

									img_dsp_config_statistics_info(fd_iav,&aaa_statistic_config	);
									printf("AWB Tiles Configuration!\n");
									break;
								case AFTileConfiguration:
									TCP_get_param( recvBuffer,  dataArray);

									img_dsp_get_config_statistics_info(&aaa_statistic_config);
									aaa_statistic_config.af_tile_num_col=dataArray[0];
									aaa_statistic_config.af_tile_num_row=dataArray[1];
									aaa_statistic_config.af_tile_col_start=dataArray[2];
									aaa_statistic_config.af_tile_row_start=dataArray[3];
									aaa_statistic_config.af_tile_width=dataArray[4];
									aaa_statistic_config.af_tile_height=dataArray[5];
									aaa_statistic_config.af_tile_active_width=dataArray[6];
									aaa_statistic_config.af_tile_active_height=dataArray[7];

									aaa_statistic_config.enable=1;

									img_dsp_config_statistics_info(fd_iav,&aaa_statistic_config);

									printf("AF Tile Configuration!\n");
									break;
								case AFStatisticSetupEx:
									TCP_get_param( recvBuffer,  dataArray);
									af_statistic_setup_ex.af_horizontal_filter1_mode=dataArray[0];
									af_statistic_setup_ex.af_horizontal_filter1_stage1_enb=dataArray[1];
									af_statistic_setup_ex.af_horizontal_filter1_stage2_enb=dataArray[2];
									af_statistic_setup_ex.af_horizontal_filter1_stage3_enb=dataArray[3];
									af_statistic_setup_ex.af_horizontal_filter1_gain[0]=dataArray[4];
									af_statistic_setup_ex.af_horizontal_filter1_gain[1]=dataArray[5];
									af_statistic_setup_ex.af_horizontal_filter1_gain[2]=dataArray[6];
									af_statistic_setup_ex.af_horizontal_filter1_gain[3]=dataArray[7];
									af_statistic_setup_ex.af_horizontal_filter1_gain[4]=dataArray[8];
									af_statistic_setup_ex.af_horizontal_filter1_gain[5]=dataArray[9];
									af_statistic_setup_ex.af_horizontal_filter1_gain[6]=dataArray[10];
									af_statistic_setup_ex.af_horizontal_filter1_shift[0]=dataArray[11];
									af_statistic_setup_ex.af_horizontal_filter1_shift[1]=dataArray[12];
									af_statistic_setup_ex.af_horizontal_filter1_shift[2]=dataArray[13];
									af_statistic_setup_ex.af_horizontal_filter1_shift[3]=dataArray[14];
									af_statistic_setup_ex.af_horizontal_filter1_bias_off=dataArray[15];
									af_statistic_setup_ex.af_vertical_filter1_thresh=dataArray[16];
									af_statistic_setup_ex.af_tile_fv1_horizontal_shift=dataArray[17];
									af_statistic_setup_ex.af_tile_fv1_horizontal_weight=dataArray[18];
									af_statistic_setup_ex.af_tile_fv1_vertical_shift=dataArray[19];
									af_statistic_setup_ex.af_tile_fv1_vertical_weight=dataArray[20];

									af_statistic_setup_ex.af_horizontal_filter2_mode=dataArray[21];
									af_statistic_setup_ex.af_horizontal_filter2_stage1_enb=dataArray[22];
									af_statistic_setup_ex.af_horizontal_filter2_stage2_enb=dataArray[23];
									af_statistic_setup_ex.af_horizontal_filter2_stage3_enb=dataArray[24];
									af_statistic_setup_ex.af_horizontal_filter2_gain[0]=dataArray[25];
									af_statistic_setup_ex.af_horizontal_filter2_gain[1]=dataArray[26];
									af_statistic_setup_ex.af_horizontal_filter2_gain[2]=dataArray[27];
									af_statistic_setup_ex.af_horizontal_filter2_gain[3]=dataArray[28];
									af_statistic_setup_ex.af_horizontal_filter2_gain[4]=dataArray[29];
									af_statistic_setup_ex.af_horizontal_filter2_gain[5]=dataArray[30];
									af_statistic_setup_ex.af_horizontal_filter2_gain[6]=dataArray[31];
									af_statistic_setup_ex.af_horizontal_filter2_shift[0]=dataArray[32];
									af_statistic_setup_ex.af_horizontal_filter2_shift[1]=dataArray[33];
									af_statistic_setup_ex.af_horizontal_filter2_shift[2]=dataArray[34];
									af_statistic_setup_ex.af_horizontal_filter2_shift[3]=dataArray[35];
									af_statistic_setup_ex.af_horizontal_filter2_bias_off=dataArray[36];
									af_statistic_setup_ex.af_vertical_filter2_thresh=dataArray[37];
									af_statistic_setup_ex.af_tile_fv2_horizontal_shift=dataArray[38];
									af_statistic_setup_ex.af_tile_fv2_horizontal_weight=dataArray[39];
									af_statistic_setup_ex.af_tile_fv2_vertical_shift=dataArray[40];
									af_statistic_setup_ex.af_tile_fv2_vertical_weight=dataArray[41];

									img_dsp_set_af_statistics_ex(fd_iav,&af_statistic_setup_ex,1);//??
									printf("AF Statistic Setup Ex!\n");


									break;
								case ExposureControl://not finished
									TCP_get_param( recvBuffer,  dataArray);
									agc_index=dataArray[0];
									shutter_index=dataArray[1];
									iris_index=dataArray[2];
									dgain=dataArray[3];
								       img_set_sensor_shutter_index(fd_iav,shutter_index);
								  	img_set_sensor_agc_index(fd_iav,agc_index,sensor_double_step);

									img_dsp_set_rgb_gain(fd_iav, &wb_gain, dgain);
									printf("Exposure Control!\n");
									break;
							}

				  	break;
					}

					break;
				case Caribration:
					printf("break layer1\n");
					break;
				case AutoTest:
					printf("break layer1\n");
					break;

			}

		}
		else if(id_Require==Load)//load
		{
		printf("Load!\n");

		switch(data.layer1)
			{

			case OnlineTuning:
			//	printf("Online Tuning!\n");
				switch(data.layer2)
				{

					case Color:
				//		printf("Color Processing!\n");
						switch(data.layer3)
						{
							case BlackLevelCorrection:

								img_dsp_get_global_blc(&blc);
								sendBuffer[0]=blc.r_offset;
								sendBuffer[1]=blc.gr_offset;
								sendBuffer[2]=blc.gb_offset;
								sendBuffer[3]=blc.b_offset;
								char sendBlc[10];
								for(i=0;i<4;i++)
								{
									int k;
									k=Int32ToCharArray(sendBuffer[i],sendBlc);
									send(new_fd,sendBlc,k,0);
									send(new_fd," ",1,0);
								}
							//	send(new_fd,&blc,sizeof(blc_level_t),0);
								send(new_fd,"!",1,0);
								printf("Black Level Correction!\n");


								break;
							case ColorCorrection:

								break;
							case ToneCurve:
								img_dsp_get_tone_curve(&tone_curve);
								for( i=0;i<TONE_CURVE_SIZE;i++)
									sendBuffer[i]=tone_curve.tone_curve_blue[i];
								char sendToneCurve[10];
								for(i=0;i<TONE_CURVE_SIZE;i++)
								{
									int k;
									k=Int32ToCharArray(sendBuffer[i],sendToneCurve);
									send(new_fd,sendToneCurve,k,0);
									send(new_fd," ",1,0);
								}
								send(new_fd,"!",1,0);
								printf("Tone Curve!\n ");

								break;
							case RGBtoYUVMatrix:
								adj_get_img_property(&image_prop);
								img_dsp_get_rgb2yuv_matrix(&rgb2yuv_matrix);
								sendBuffer[0]=rgb2yuv_matrix.matrix_values[0];
								sendBuffer[1]=rgb2yuv_matrix.matrix_values[1];
								sendBuffer[2]=rgb2yuv_matrix.matrix_values[2];
								sendBuffer[3]=rgb2yuv_matrix.matrix_values[3];
								sendBuffer[4]=rgb2yuv_matrix.matrix_values[4];
								sendBuffer[5]=rgb2yuv_matrix.matrix_values[5];
								sendBuffer[6]=rgb2yuv_matrix.matrix_values[6];
								sendBuffer[7]=rgb2yuv_matrix.matrix_values[7];
								sendBuffer[8]=rgb2yuv_matrix.matrix_values[8];
								sendBuffer[9]=rgb2yuv_matrix.y_offset;
								sendBuffer[10]=rgb2yuv_matrix.u_offset ;
								sendBuffer[11]=rgb2yuv_matrix.v_offset ;
								sendBuffer[12]=image_prop.brightness;
								sendBuffer[13]=image_prop.contrast;
								sendBuffer[14]=image_prop.saturation;
								sendBuffer[15]=image_prop.hue;
								char sendRGBtoYUVMatrix[10];
								for(i=0;i<16;i++)
								{
									int k;
									k=Int32ToCharArray(sendBuffer[i],sendRGBtoYUVMatrix);
									send(new_fd,sendRGBtoYUVMatrix,k,0);
									send(new_fd," ",1,0);
								}
								send(new_fd,"!",1,0);
								printf("RGB to YUV Matrix!\n");
								break;

							case WhiteBalanceGains:


								img_dsp_get_rgb_gain(&wb_gain, &dgain);
								sendBuffer[0]=wb_gain.r_gain;
								sendBuffer[1]=wb_gain.g_gain;
								sendBuffer[2]=wb_gain.b_gain;

								char sendWB[10];
								for(i=0;i<3;i++)
								{
									int k;
									k=Int32ToCharArray(sendBuffer[i],sendWB);
									send(new_fd,sendWB,k,0);
									send(new_fd," ",1,0);
								}
								send(new_fd,"!",1,0);
		 						printf("White Balance Gains!\n");

								break;
							case DGainSaturaionLevel:
								img_dsp_get_dgain_saturation_level(&d_gain_satuation_level);
								sendBuffer[0]=d_gain_satuation_level.level_red;
								sendBuffer[1]=d_gain_satuation_level.level_green_even;
								sendBuffer[2]=d_gain_satuation_level.level_green_odd;
								sendBuffer[3]=d_gain_satuation_level.level_blue;

								char sendDGainSaturaionLevel[10];
								for(i=0;i<4;i++)
								{
									int k;
									k=Int32ToCharArray(sendBuffer[i],sendDGainSaturaionLevel);
									send(new_fd,sendDGainSaturaionLevel,k,0);
									send(new_fd," ",1,0);
								}
								send(new_fd,"!",1,0);
								printf("DGain Saturation Level!\n");
								break;

							case LocalExposure:
								img_dsp_get_local_exposure(&local_exposure);
								sendBuffer[0]=local_exposure.enable;
								sendBuffer[1]=local_exposure.radius;
								sendBuffer[2]=local_exposure.luma_weight_shift ;
								sendBuffer[3]=local_exposure.luma_weight_red ;
								sendBuffer[4]=local_exposure.luma_weight_green ;
								sendBuffer[5]=local_exposure.luma_weight_blue ;

								for( i=0;i<NUM_EXPOSURE_CURVE;i++)
									sendBuffer[6+i]=local_exposure.gain_curve_table[i] ;


							//	send(new_fd,sendCharArray,6,0);
							//	printf("%d,%d,%d,%d,%d,%d\n",sendCharArray[0],sendCharArray[1],sendCharArray[2],sendCharArray[3],sendCharArray[4],sendCharArray[5]);

								char sendLocalExposure[10];
								for(i=0;i<NUM_EXPOSURE_CURVE+6;i++)
								{
									int k;
									k=Int32ToCharArray(sendBuffer[i],sendLocalExposure);

									send(new_fd,sendLocalExposure,k,0);
									send(new_fd," ",1,0);
								}
								send(new_fd,"!",1,0);

								printf("Local Exposure!\n");


								break;
							case ChromaScale:
								 img_dsp_get_chroma_scale(&cs);
								sendBuffer[0]=cs.enable;

								for (i = 0;i<NUM_CHROMA_GAIN_CURVE;i++)
									sendBuffer[i+1]=cs.gain_curve[i] ;

								char sendChromaScale[10];
								int num_cs=NUM_CHROMA_GAIN_CURVE+1;
								for(i=0;i<num_cs;i++)
								{
									int k;
									k=Int32ToCharArray(sendBuffer[i],sendChromaScale);
									send(new_fd,sendChromaScale,k,0);
									send(new_fd," ",1,0);
								}
								send(new_fd,"!",1,0);

								printf("Chroma Scale!\n");


								break;

						}

						break;

					case Noise:
					//	printf("Noise processing!\n");
						switch(data.layer3)
							{
								case FPNCorrection://not finished



									break;
								case BadPixelCorrection:
									img_dsp_get_dynamic_bad_pixel_correction(&dbp_correction_setting);
									sendBuffer[0]=dbp_correction_setting.enable;

									sendBuffer[1]=dbp_correction_setting.hot_pixel_strength;
									sendBuffer[2]=dbp_correction_setting.dark_pixel_strength;
									char sendBPC[10];
									for(i=0;i<3;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendBPC);
										send(new_fd,sendBPC,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);

									printf("Bad Pixel Correction!\n");

									break;
								case CFALeakageFilter:
									img_dsp_get_cfa_leakage_filter( &cfa_leakage_filter);
									sendBuffer[0]=cfa_leakage_filter.enable;

									sendBuffer[1]=cfa_leakage_filter.alpha_rr;
									sendBuffer[2]=cfa_leakage_filter.alpha_rb;
									sendBuffer[3]=cfa_leakage_filter.alpha_br;
									sendBuffer[4]=cfa_leakage_filter.alpha_bb;
									sendBuffer[5]=cfa_leakage_filter.saturation_level;
									char sendLKG[10];
									for(i=0;i<6;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendLKG);
										send(new_fd,sendLKG,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);


									printf("CFA Leakage Filter!\n");


									break;
								case AntiAliasingFilter:


									sendBuffer[0]=img_dsp_get_anti_aliasing();
									printf("%d\n",sendBuffer[0]);
									send(new_fd,sendBuffer,1,0);
									send(new_fd,"!",1,0);


									printf("Anti-Aliasing Filter!\n");

									break;
								case CFANoiseFilter:
									img_dsp_get_cfa_noise_filter(& cfa_noise_filter);

									sendBuffer[0]=cfa_noise_filter.enable ;

									sendBuffer[1]=cfa_noise_filter.iso_center_weight_red ;
									sendBuffer[2]=cfa_noise_filter.iso_center_weight_green ;
									sendBuffer[3]=cfa_noise_filter.iso_center_weight_blue ;
									sendBuffer[4]=cfa_noise_filter.iso_thresh_k0_red ;
									sendBuffer[5]=cfa_noise_filter.iso_thresh_k0_green ;
									sendBuffer[6]=cfa_noise_filter.iso_thresh_k0_blue ;
									sendBuffer[7]=cfa_noise_filter.iso_thresh_k0_close;
									sendBuffer[8]=cfa_noise_filter.iso_thresh_k1_red;
									sendBuffer[9]=cfa_noise_filter.iso_thresh_k1_green;
									sendBuffer[10]=cfa_noise_filter.iso_thresh_k1_blue ;

									sendBuffer[11]=cfa_noise_filter.direct_center_weight_red ;
									sendBuffer[12]=cfa_noise_filter.direct_center_weight_green ;
									sendBuffer[13]=cfa_noise_filter.direct_center_weight_blue;
									sendBuffer[14]=cfa_noise_filter.direct_thresh_k0_red;
									sendBuffer[15]=cfa_noise_filter.direct_thresh_k0_green;
									sendBuffer[16]=cfa_noise_filter.direct_thresh_k0_blue ;
									sendBuffer[17]=cfa_noise_filter.direct_thresh_k1_red ;
									sendBuffer[18]=cfa_noise_filter.direct_thresh_k1_green;
									sendBuffer[19]=cfa_noise_filter.direct_thresh_k1_blue ;
									sendBuffer[20]=cfa_noise_filter.direct_grad_thresh ;
									char sendCFA[10];
									for(i=0;i<21;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendCFA);
										send(new_fd,sendCFA,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);
									printf("CFA Noise Filter!\n");
									break;
								case ChromaMedianFiler:
									img_dsp_get_chroma_median_filter(&chroma_median_setup);
									sendBuffer[0]=chroma_median_setup.enable;
									sendBuffer[1]=chroma_median_setup.cb_str;
									sendBuffer[2]=chroma_median_setup.cr_str;
							//		printf("%d,%d,%d\n",chroma_median_setup.enable,chroma_median_setup.cb_str,chroma_median_setup.cb_str);

									char sendCMF[10];
									for(i=0;i<3;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendCMF);
										send(new_fd,sendCMF,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);

									printf("Chroma Median Filer!\n");
									break;
								case SharpeningControl:

									img_dsp_get_spatial_filter(0,&spatial_filter_info);
									img_dsp_get_sharpen_fir(  0,&fir );
									mode=img_dsp_get_sharpen_fir_mode();
									retain_level=img_dsp_get_sharpen_signal_retain_level(0);
									luma_high_freq_noise_reduction_strength=img_dsp_get_luma_high_freq_noise_reduction();
									img_dsp_get_sharpen_coring(0,&coring);
									img_dsp_get_sharpen_max_change(0, &max_change);
									img_dsp_get_sharpen_level_min(0, &sharpen_setting_min);
									img_dsp_get_sharpen_level_overall(0,&sharpen_setting_overall);

									sendBuffer[0]=fir.fir_strength;
									for(i=0;i<10;i++)
										sendBuffer[1+i]=fir.fir_coeff[i];
									for(i=0;i<256;i++)
										sendBuffer[11+i]=coring.coring[i];
									sendBuffer[267]=luma_high_freq_noise_reduction_strength;
									sendBuffer[268]=spatial_filter_info.isotropic_strength;
									sendBuffer[269]=spatial_filter_info.directional_strength;
									sendBuffer[270]=spatial_filter_info.edge_threshold;
									sendBuffer[271]=retain_level;
									sendBuffer[272]=max_change.max_change_down;
									sendBuffer[273]=max_change.max_change_up;
									sendBuffer[274]=sharpen_setting_min.low;
									sendBuffer[275]=sharpen_setting_min.low_delta;
									sendBuffer[276]=sharpen_setting_min.low_strength;
									sendBuffer[277]=sharpen_setting_min.mid_strength;
									sendBuffer[278]=sharpen_setting_min.high;
									sendBuffer[279]=sharpen_setting_min.high_delta;
									sendBuffer[280]=sharpen_setting_min.high_strength;
									sendBuffer[281]=sharpen_setting_overall.low;
									sendBuffer[282]=sharpen_setting_overall.low_delta;
									sendBuffer[283]=sharpen_setting_overall.low_strength;
									sendBuffer[284]=sharpen_setting_overall.mid_strength;
									sendBuffer[285]=sharpen_setting_overall.high;
									sendBuffer[286]=sharpen_setting_overall.high_delta;
									sendBuffer[287]=sharpen_setting_overall.high_strength;
									sendBuffer[288] =mode;
									sendBuffer[289] =spatial_filter_info.max_change;

									char sendSC[10];
									for(i=0;i<290;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendSC);
										send(new_fd,sendSC,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);

									printf("Sharpening Control!\n");

									break;
								case MCTFControl:
									img_dsp_get_video_mctf(&mctf_info);
									sendBuffer[0]=mctf_info.enable;
									sendBuffer[1]=mctf_info.alpha;
									sendBuffer[2]=mctf_info.alpha_2;
									sendBuffer[3]=mctf_info.threshold_1;
									sendBuffer[4]=mctf_info.threshold_2;
									sendBuffer[5]=mctf_info.threshold_3;
									sendBuffer[6]=mctf_info.threshold_4;
									sendBuffer[7]=mctf_info.y_max_change;
									sendBuffer[8]=mctf_info.u_max_change;
									sendBuffer[9]=mctf_info.v_max_change;
									char sendMCTF[10];
									for(i=0;i<10;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendMCTF);
										send(new_fd,sendMCTF,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);

									printf("MCTF Control!\n");

									break;

							}

						break;
					case AAA:
					//	printf("AAA processing!\n");
						switch(data.layer3)
							{
								case ConfigAAAControl:
									img_get_3a_cntl_status(&aaa_cntl_station);
									sendCharArray[0]=aaa_cntl_station.ae_enable;
									sendCharArray[1]=aaa_cntl_station.awb_enable;
									sendCharArray[2]=aaa_cntl_station.af_enable;
									sendCharArray[3]=aaa_cntl_station.adj_enable;
							//		printf("%d,%d,%d,%d\n",sendCharArray[0],sendCharArray[1],sendCharArray[2],sendCharArray[3]);

									send(new_fd,sendCharArray,4,0);
									send(new_fd,"!",1,0);

									printf(" Config AAA Control!\n");
									break;
								case AETileConfiguration:

									img_dsp_get_config_statistics_info(&aaa_statistic_config);
									img_dsp_get_statistics_raw( fd_iav, &p_rgb_stat,& p_cfa_stat);

									sendBuffer[0]=p_cfa_stat.tile_info.ae_tile_num_col;//aaa_tile_get_info.ae_tile_num_col;
									sendBuffer[1]=p_cfa_stat.tile_info.ae_tile_num_row;
									sendBuffer[2]=p_cfa_stat.tile_info.ae_tile_col_start;
									sendBuffer[3]=p_cfa_stat.tile_info.ae_tile_row_start;
									sendBuffer[4]=p_cfa_stat.tile_info.ae_tile_width;
									sendBuffer[5]=p_cfa_stat.tile_info.ae_tile_height;
									sendBuffer[6]=p_cfa_stat.tile_info.ae_linear_y_shift;
									sendBuffer[7]=p_cfa_stat.tile_info.ae_y_shift;

									sendBuffer[8]=aaa_statistic_config.ae_tile_num_col;
									sendBuffer[9]=aaa_statistic_config.ae_tile_num_row;
									sendBuffer[10]=aaa_statistic_config.ae_tile_col_start;
									sendBuffer[11]=aaa_statistic_config.ae_tile_row_start;
									sendBuffer[12]=aaa_statistic_config.ae_tile_width;
									sendBuffer[13]=aaa_statistic_config.ae_tile_height;
									sendBuffer[14]=aaa_statistic_config.ae_pix_min_value;
									sendBuffer[15]=aaa_statistic_config.ae_pix_max_value;

									char sendAETileConfiguration[10];
									for(i=0;i<16;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendAETileConfiguration);
										send(new_fd,sendAETileConfiguration,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);
									printf("AETileConfiguration!\n");
									break;
								case AWBTilesConfiguration:

									img_dsp_get_statistics_raw( fd_iav,&p_rgb_stat,&p_cfa_stat);
									img_dsp_get_config_statistics_info(&aaa_statistic_config);

									sendBuffer[0]=p_cfa_stat.tile_info.awb_tile_num_col;
									sendBuffer[1]=p_cfa_stat.tile_info.awb_tile_num_row;
									sendBuffer[2]=p_cfa_stat.tile_info.awb_tile_col_start;
									sendBuffer[3]=p_cfa_stat.tile_info.awb_tile_row_start;
									sendBuffer[4]=p_cfa_stat.tile_info.awb_tile_width;
									sendBuffer[5]=p_cfa_stat.tile_info.awb_tile_height;
									sendBuffer[6]=p_cfa_stat.tile_info.awb_y_shift;
									sendBuffer[7]=p_cfa_stat.tile_info.awb_rgb_shift;
									sendBuffer[8]=p_cfa_stat.tile_info.awb_tile_active_width;
									sendBuffer[9]=p_cfa_stat.tile_info.awb_tile_active_height;
									sendBuffer[10]=p_cfa_stat.tile_info.awb_min_max_shift;

									sendBuffer[11]=aaa_statistic_config.awb_tile_num_col;
									sendBuffer[12]=aaa_statistic_config.awb_tile_num_row;
									sendBuffer[13]=aaa_statistic_config.awb_tile_col_start;
									sendBuffer[14]=aaa_statistic_config.awb_tile_row_start;
									sendBuffer[15]=aaa_statistic_config.awb_tile_width;
									sendBuffer[16]=aaa_statistic_config.awb_tile_height;
									sendBuffer[17]=aaa_statistic_config.awb_pix_min_value;
									sendBuffer[18]=aaa_statistic_config.awb_pix_max_value;
									sendBuffer[19]=aaa_statistic_config.awb_tile_active_width;
									sendBuffer[20]=aaa_statistic_config.awb_tile_active_height;


									char sendAWBTilesConfiguration[10];
									for(i=0;i<21;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendAWBTilesConfiguration);
										send(new_fd,sendAWBTilesConfiguration,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);

									printf("AWB Tiles Configuration!\n");
									break;
								case AFTileConfiguration:

									img_dsp_get_statistics_raw( fd_iav,&p_rgb_stat,&p_cfa_stat);
									img_dsp_get_config_statistics_info(&aaa_statistic_config);

									sendBuffer[0]=p_cfa_stat.tile_info.af_tile_num_col;
									sendBuffer[1]=p_cfa_stat.tile_info.af_tile_num_row;
									sendBuffer[2]=p_cfa_stat.tile_info.af_tile_col_start;
									sendBuffer[3]=p_cfa_stat.tile_info.af_tile_row_start;
									sendBuffer[4]=p_cfa_stat.tile_info.af_tile_width;
									sendBuffer[5]=p_cfa_stat.tile_info.af_tile_height;
									sendBuffer[6]=p_cfa_stat.tile_info.af_y_shift;
									sendBuffer[7]=p_cfa_stat.tile_info.af_cfa_y_shift;
									sendBuffer[8]=p_cfa_stat.tile_info.af_tile_active_width;
									sendBuffer[9]=p_cfa_stat.tile_info.af_tile_active_height;

									sendBuffer[10]=aaa_statistic_config.af_tile_num_col;
									sendBuffer[11]=aaa_statistic_config.af_tile_num_row;
									sendBuffer[12]=aaa_statistic_config.af_tile_col_start;
									sendBuffer[13]=aaa_statistic_config.af_tile_row_start;
									sendBuffer[14]=aaa_statistic_config.af_tile_width;
									sendBuffer[15]=aaa_statistic_config.af_tile_height;
									sendBuffer[16]=aaa_statistic_config.af_tile_active_width;
									sendBuffer[17]=aaa_statistic_config.af_tile_active_height;

									char sendAFTileConfiguration[10];
									for(i=0;i<18;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendAFTileConfiguration);
										send(new_fd,sendAFTileConfiguration,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);
									printf("AF Tile Configuration!\n");
									break;
								case AFStatisticSetupEx:

									img_dsp_get_af_statistics_ex(&af_statistic_setup_ex);

									sendBuffer[0]=af_statistic_setup_ex.af_horizontal_filter1_mode;
									sendBuffer[1]=af_statistic_setup_ex.af_horizontal_filter1_stage1_enb;
									sendBuffer[2]=af_statistic_setup_ex.af_horizontal_filter1_stage2_enb;
									sendBuffer[3]=af_statistic_setup_ex.af_horizontal_filter1_stage3_enb;
									sendBuffer[4]=af_statistic_setup_ex.af_horizontal_filter1_gain[0];
									sendBuffer[5]=af_statistic_setup_ex.af_horizontal_filter1_gain[1];
									sendBuffer[6]=af_statistic_setup_ex.af_horizontal_filter1_gain[2];
									sendBuffer[7]=af_statistic_setup_ex.af_horizontal_filter1_gain[3];
									sendBuffer[8]=af_statistic_setup_ex.af_horizontal_filter1_gain[4];
									sendBuffer[9]=af_statistic_setup_ex.af_horizontal_filter1_gain[5];
									sendBuffer[10]=af_statistic_setup_ex.af_horizontal_filter1_gain[6];
									sendBuffer[11]=af_statistic_setup_ex.af_horizontal_filter1_shift[0];
									sendBuffer[12]=af_statistic_setup_ex.af_horizontal_filter1_shift[1];
									sendBuffer[13]=af_statistic_setup_ex.af_horizontal_filter1_shift[2];
									sendBuffer[14]=af_statistic_setup_ex.af_horizontal_filter1_shift[3];
									sendBuffer[15]=af_statistic_setup_ex.af_horizontal_filter1_bias_off;
									sendBuffer[16]=af_statistic_setup_ex.af_vertical_filter1_thresh;
									sendBuffer[17]=af_statistic_setup_ex.af_tile_fv1_horizontal_shift;
									sendBuffer[18]=af_statistic_setup_ex.af_tile_fv1_horizontal_weight;
									sendBuffer[19]=af_statistic_setup_ex.af_tile_fv1_vertical_shift;
									sendBuffer[20]=af_statistic_setup_ex.af_tile_fv1_vertical_weight;

									sendBuffer[21]=af_statistic_setup_ex.af_horizontal_filter2_mode;
									sendBuffer[22]=af_statistic_setup_ex.af_horizontal_filter2_stage1_enb;
									sendBuffer[23]=af_statistic_setup_ex.af_horizontal_filter2_stage2_enb;
									sendBuffer[24]=af_statistic_setup_ex.af_horizontal_filter2_stage3_enb;
									sendBuffer[25]=af_statistic_setup_ex.af_horizontal_filter2_gain[0];
									sendBuffer[26]=af_statistic_setup_ex.af_horizontal_filter2_gain[1];
									sendBuffer[27]=af_statistic_setup_ex.af_horizontal_filter2_gain[2];
									sendBuffer[28]=af_statistic_setup_ex.af_horizontal_filter2_gain[3];
									sendBuffer[29]=af_statistic_setup_ex.af_horizontal_filter2_gain[4];
									sendBuffer[30]=af_statistic_setup_ex.af_horizontal_filter2_gain[5];
									sendBuffer[31]=af_statistic_setup_ex.af_horizontal_filter2_gain[6];
									sendBuffer[32]=af_statistic_setup_ex.af_horizontal_filter2_shift[0];
									sendBuffer[33]=af_statistic_setup_ex.af_horizontal_filter2_shift[1];
									sendBuffer[34]=af_statistic_setup_ex.af_horizontal_filter2_shift[2];
									sendBuffer[35]=af_statistic_setup_ex.af_horizontal_filter2_shift[3];
									sendBuffer[36]=af_statistic_setup_ex.af_horizontal_filter2_bias_off;
									sendBuffer[37]=af_statistic_setup_ex.af_vertical_filter2_thresh;
									sendBuffer[38]=af_statistic_setup_ex.af_tile_fv2_horizontal_shift;
									sendBuffer[39]=af_statistic_setup_ex.af_tile_fv2_horizontal_weight;
									sendBuffer[40]=af_statistic_setup_ex.af_tile_fv2_vertical_shift;
									sendBuffer[41]=af_statistic_setup_ex.af_tile_fv2_vertical_weight;

									char sendAFS[10];
									for(i=0;i<42;i++)
									{

										int k;
										k=Int32ToCharArray(sendBuffer[i],sendAFS);
										send(new_fd,sendAFS,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);
									printf("AF Statistic Setup Ex!\n");


									break;
								case ExposureControl:
									agc_index=img_get_sensor_agc_index();
									shutter_index=img_get_sensor_shutter_index();
									img_dsp_get_rgb_gain( &wb_gain,  &dgain);

									sendBuffer[0]=agc_index;
									sendBuffer[1]=shutter_index;
									sendBuffer[2]=dgain;

									char sendEC[10];
									for(i=0;i<3;i++)
									{
										int k;
										k=Int32ToCharArray(sendBuffer[i],sendEC);
										send(new_fd,sendEC,k,0);
										send(new_fd," ",1,0);
									}
									send(new_fd,"!",1,0);

								//	printf("agc_index=%d,shutter_index=%d,dgain=%d\n",agc_index,shutter_index,dgain);

									printf("Exposure Control!\n");
									break;
							}

				  	break;
					}

					break;
				case Caribration:
					printf("break layer1\n");
					break;
				case AutoTest:
					printf("break layer1\n");
					break;

			}

		}
		else
		{
			printf("Get undefined id_require,please check!\n");

		}
	}
	if(sockfd!=-1)
		close(sockfd);

	return 0;

}
