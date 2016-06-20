#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include "basetypes.h"
#include "img_struct.h"
#include "img_dsp_interface.h"
#include "img_api.h"
#include "ambas_vin.h"
#include "af_algo.h"
#include "tamronDF003_drv.h"

#include "image_sensor_adj_param.c"
#include "image_sensor_aeb_param.c"

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

extern lens_dev_drv_t tamronDF003_drv;

int fd_iav;

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
	int file, count;
	char sensor_name[32];
	sensor_label sensor_lb;
	image_sensor_param_t app_param_image_sensor = {0};
	void* p_af_param = NULL;
	void* p_zoom_map = NULL;
	struct amba_vin_source_info vin_info;
	aaa_api_t custom_aaa_api = {0};

	custom_aaa_api.p_ae_control = NULL;
	custom_aaa_api.p_ae_control_init = NULL;
	custom_aaa_api.p_af_control = custom_af_control;
	custom_aaa_api.p_af_control_init = custom_af_control_init;
	custom_aaa_api.p_af_set_calib_param = custom_af_set_calib_param;
	custom_aaa_api.p_af_set_range = custom_af_set_range;

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
			sprintf(sensor_name, "ov9715");
			break;
		case SENSOR_OV2710:
			sensor_lb = OV_2710;
		      	sprintf(sensor_name, "ov2710");
			break;
		case SENSOR_OV5653:
			sensor_lb = OV_5653;
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
			sprintf(sensor_name, "imx035");
			break;
		case SENSOR_IMX036:
			sensor_lb = IMX_036;
			sprintf(sensor_name, "imx036");
			break;
		default:
			return -1;
	}
	app_param_image_sensor.p_adj_param = &is_adj_param;
	app_param_image_sensor.p_rgb2yuv = &is_rgb2yuv;
	app_param_image_sensor.p_chroma_scale = &is_chroma_scale;
	app_param_image_sensor.p_awb_param = &is_awb_param;
	app_param_image_sensor.p_50hz_lines = is_50hz_lines;
	app_param_image_sensor.p_60hz_lines = is_60hz_lines;
	app_param_image_sensor.p_tile_config = &is_tile_config;
	app_param_image_sensor.p_ae_agc_dgain = is_ae_agc_dgain;
	app_param_image_sensor.p_ae_sht_dgain = is_ae_sht_dgain;
 	app_param_image_sensor.p_dlight_range = is_dlight;
	app_param_image_sensor.p_manual_LE = is_manual_LE;
	if (img_config_sensor_info(sensor_lb) < 0) {
		return -1;
	}
	if (load_dsp_cc_table() < 0)
		return -1;
	if (load_adj_cc_table(sensor_name) < 0)
		return -1;

	img_load_image_sensor_param(app_param_image_sensor);
	img_register_lens_drv(tamronDF003_drv);
	img_register_aaa_algorithm(custom_aaa_api);
	img_af_load_param(p_af_param, p_zoom_map);
	img_config_lens_info(LENS_CUSTOM_ID);
	img_lens_init();

	if(img_start_aaa(fd_iav))
		return -1;
	while (1) {
		sleep(1000);
	}
	return 0;
}

