#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include "types.h"
#include "img_struct.h"
#include "img_dsp_interface.h"
#include "img_api.h"
#include "ambas_vin.h"

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

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

#define NO_ARG 0
#define HAS_ARG 1
#define UNIT_chroma_scale (64)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
u8 start_aaa;
float zoom_factor;
int fd_iav;
af_range_t af_range = {-100, 800};
s8 zoom_idx = 0;
s16 focus_idx = 0;

static void usage(void){
	printf("AF configuration: \n");
	printf("c - Lens type\n");
	printf("m - AF mode\n");
	printf("t - zoom tele\n");
	printf("w - zoom wide\n");
	printf("f - focus far(only avalable in MF mode)\n");
	printf("n - focus near(only avalable in MF mode)\n");
	printf("r - Set focus range(only avalable in CAF and SAF mode)\n");
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

int load_adj_cc_table(char *sensor_name)
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
	int i;
	char ch;
	char sensor_name[32];
	sensor_label sensor_lb;
	image_sensor_param_t app_param_image_sensor = {0};
	struct amba_vin_source_info vin_info;

	if(img_lib_init(0,0)<0) {
		perror("/dev/iav");
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
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

			sprintf(sensor_name, "ov5653");
			break;
		case SENSOR_MT9J001:
			sensor_lb = MT_9J001;
			sprintf(sensor_name, "mt9j001");
			break;
		case SENSOR_MT9P001:
			sensor_lb = MT_9P031;
			sprintf(sensor_name, "mt9p031");
			break;
		case SENSOR_MT9M033:
			sensor_lb = MT_9M033;
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
			sprintf(sensor_name, "imx036");
			break;
		default:
			return -1;
	}
	if (img_config_sensor_info(sensor_lb) < 0) {
		return -1;
	}
	if (load_dsp_cc_table() < 0)
		return -1;
	if (load_adj_cc_table(sensor_name) < 0)
		return -1;

	img_load_image_sensor_param(&app_param_image_sensor);
	img_config_lens_info(LENS_CMOUNT_ID);
	if(img_start_aaa(fd_iav))
		return -1;
	while((ch = getchar())) {
		  usage();
		  switch(ch) {
		  case	'c':
		  	  printf("1: C-mount MF lens\n");
		  	  printf("2: tamron 18x\n");
		  	  scanf("%d", &i);
		  	  switch(i){
		  	  	  case	1:
		  	  	  	  img_config_lens_info(LENS_CMOUNT_ID);
		  	  	  break;
		  	  	  case	2:
		  	  	  	  img_config_lens_info(LENS_TAMRON18X_ID);
		  	  	  break;
		  	  }
		  break;
		  case	't':
		  	  zoom_idx ++;
		  	  if(zoom_idx>18)
		  	  	  zoom_idx = 18;
		  	  img_af_set_zoom_idx(zoom_idx);
		  break;
		  case	'w':
		  	  zoom_idx--;
		  	  if(zoom_idx<0)
		  	  	  zoom_idx = 0;
		  	  img_af_set_zoom_idx(zoom_idx);
		  break;
		  case	'm':
		  	  printf("1: CAF\n");
		  	  printf("2: SAF\n");
		  	  printf("3: Manual AF\n");
		  	  scanf("%d",&i);
		  	  switch(i){
		  	  	  case	1:
		  	  	  	  img_af_set_mode(CAF);
		  	  	  break;
		  	  	  case	2:
		  	  	  	  img_af_set_mode(SAF);
		  	  	  break;
		  	  	  case	3:
		  	  	  	  img_af_set_mode(MANUAL);
		  	  	  break;
		  	  }
		  break;
		  case	'f':
		  	  focus_idx -= 100;
		  	  if(focus_idx < -100)
		  	  	  focus_idx = -100;
		  	  img_af_set_focus_idx(focus_idx);
		  break;
		  case	'n':
		  	  focus_idx += 100;
		  	  if(focus_idx>1000)
		  	  	  focus_idx = 1000;
		  	  img_af_set_focus_idx(focus_idx);
		  break;
		  case	'r':
		  	  printf("1: near bandary\n");
		  	  printf("2: far bandary\n");
		  	  scanf("%d",&i);
		  	  switch(i){
		  	  	  case 1:
		  	  	  	scanf("%d",&i);
		  	  	  	af_range.far_bd = i;
		  	  	  break;
		  	  	  case 2:
		  	  	  	scanf("%d",&i);
		  	  		af_range.near_bd = i;
		  		break;
			}
			if ((af_range.near_bd < af_range.far_bd)||(af_range.near_bd>1500)||(af_range.far_bd<-100)){
				printf("Sorry!\n");
				break;
			}
		img_af_set_range(&af_range);
		break;
		}
	}
	return 0;
}


