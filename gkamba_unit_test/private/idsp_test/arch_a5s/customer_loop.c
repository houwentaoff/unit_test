#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

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
#include "basetypes.h"

#include "iav_drv.h"
#include "iav_drv_ex.h"

#define CUSTOMER_MAINLOOP
#include "ambas_imgproc_arch.h"
#include "ambas_vin.h"
#include "img_struct.h"
#include "img_dsp_interface.h"
#include "img_api.h"

#include "adj_params/mt9t002_adj_param.c"
#include "adj_params/mt9t002_aeb_param.c"

#include "adj_params/mn34041pl_adj_param.c"
#include "adj_params/mn34041pl_aeb_param.c"

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

static image_sensor_param_t app_param_image_sensor;

static aaa_tile_info_t tile_info;
static ae_data_t ae_info[96];
static awb_data_t awb_info[1024];
static af_stat_t af_info[40];
static af_stat_t af_info_yuv[40];
static embed_hist_stat_t hist_info;
static histogram_stat_t dsp_histo_info;

static awb_gain_t awb_gain;
static pipeline_control_t adj_pipe_ctl;
static adj_aeawb_control_t adj_aeawb_ctl;
static adj_param_t is_adj_param;
static rgb_to_yuv_t is_rgb2yuv[4];
static chroma_scale_filter_t is_chroma_scale;
static statistics_config_t is_tile_config;
static img_awb_param_t is_awb_param;

bayer_pattern pat;
u32 max_sensor_gain, gain_step, timing_mode;

static pthread_t id;
pthread_cond_t img_cond;

static awb_control_mode_t awb_mode = WB_AUTOMATIC;
static awb_work_method_t awb_method = AWB_NORMAL;
static img_color_style rgb2yuv_style = IMG_COLOR_PC;

static af_statistics_ex_t af_eng_cof = {
		0,					// af_horizontal_filter1_mode
		0,					// af_horizontal_filter1_stage1_enb
		1,					// af_horizontal_filter1_stage2_enb
		0,					// af_horizontal_filter1_stage3_enb
		{200, 0, 0, 0, -55, 0, 0},			// af_horizontal_filter1_gain[7]
		{6, 0, 0, 0},				// af_horizontal_filter1_shift[4]
		0,					// af_horizontal_filter1_bias_off
		0,					// af_horizontal_filter1_thresh
		0,					// af_horizontal_filter2_mode
		1,					// af_horizontal_filter2_stage1_enb
		1,					// af_horizontal_filter2_stage2_enb
		1,					// af_horizontal_filter2_stage3_enb
		{188, 476, -235, 375, -184, 276, -206},	// af_horizontal_filter2_gain[7]
		{7, 2, 2, 0},				// af_horizontal_filter2_shift[4]
		0,					// af_horizontal_filter2_bias_off
		0,					// af_horizontal_filter2_thresh
		8,					// af_tile_fv1_horizontal_shift
		8,					// af_tile_fv1_vertical_shift
		168,					// af_tile_fv1_horizontal_weight
		87,					// af_tile_fv1_vertical_weight
		8,					// af_tile_fv2_horizontal_shift
		8,					// af_tile_fv2_vertical_shift
		123,					// af_tile_fv2_horizontal_weight
		132					// af_tile_fv2_vertical_weight
		};


void cus_main_loop(void* arg)
{
	int fd_iav;
	fd_iav = (int)arg;
	aaa_tile_report_t act_tile;
	u32 table_index;


	adj_video_init(&is_adj_param, &adj_pipe_ctl);
	awb_control_init(WB_AUTOMATIC, is_awb_param.menu_gain, &is_awb_param.wr_table, is_awb_param.awb_lut_idx);

	while(1) {
		
		if(img_dsp_get_statistics(fd_iav, &tile_info, ae_info, awb_info, af_info, &hist_info, &act_tile, af_info_yuv,&dsp_histo_info))
			continue;
		//AE API

		table_index = 128;// table_index = gain_db/6*128
		//AF API

		//AWB control
		awb_control(awb_method, awb_mode, act_tile.awb_tile, awb_info, &awb_gain);

		{// ADJ control
			adj_video_aeawb_control(table_index, &is_adj_param.awbae, &adj_aeawb_ctl);
			adj_video_black_level_control(table_index, awb_gain.video_wb_gain, &is_adj_param, &adj_pipe_ctl);
			adj_video_ev_img(table_index, awb_gain.video_wb_gain, &is_adj_param, &adj_pipe_ctl, &is_chroma_scale);
			adj_video_noise_control(table_index, awb_gain.video_wb_gain, 0, &is_adj_param, &adj_pipe_ctl);
			adj_yuv_extra_brightness(table_index,&adj_pipe_ctl);

			awb_set_wb_ratio(&adj_aeawb_ctl);
		}

		//use your own driver/img_dsp APIs to control the following filters/sensor
		if(awb_gain.video_gain_update==1)
		{
			img_dsp_set_wb_gain(fd_iav,&awb_gain.video_wb_gain);
			awb_gain.video_gain_update = 0;
		}
		if(adj_pipe_ctl.local_exposure_update) {
			img_dsp_set_local_exposure(fd_iav, &adj_pipe_ctl.local_exposure);
			adj_pipe_ctl.local_exposure_update = 0;
		}
		if(adj_pipe_ctl.black_corr_update) {
			img_dsp_set_global_blc(fd_iav, &adj_pipe_ctl.black_corr, pat);
			adj_pipe_ctl.black_corr_update = 0;
		}
		if(adj_pipe_ctl.chroma_scale_update) {
			img_dsp_set_chroma_scale(fd_iav, &adj_pipe_ctl.chroma_scale);
			adj_pipe_ctl.chroma_scale_update = 0;
		}
		if(adj_pipe_ctl.cc_matrix_update) {
			img_dsp_set_color_correction(fd_iav, &adj_pipe_ctl.color_corr);
			adj_pipe_ctl.cc_matrix_update = 0;
		}
		if(adj_pipe_ctl.badpix_corr_update) {
			img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &adj_pipe_ctl.badpix_corr_strength);
			adj_pipe_ctl.badpix_corr_update = 0;
		}
		if(adj_pipe_ctl.chroma_median_filter_update){
			img_dsp_set_chroma_median_filter(fd_iav, &adj_pipe_ctl.chroma_median_filter);
			adj_pipe_ctl.chroma_median_filter_update = 0;
		}
		if(adj_pipe_ctl.cfa_filter_update) {
			img_dsp_set_cfa_noise_filter(fd_iav, &adj_pipe_ctl.cfa_filter);
			adj_pipe_ctl.cfa_filter_update = 0;
		}
		if(adj_pipe_ctl.mctf_update) {
			img_dsp_set_video_mctf(fd_iav, &adj_pipe_ctl.mctf_param);
			adj_pipe_ctl.mctf_update = 0;
		}
		if(adj_pipe_ctl.spatial_filter_update) {
			img_dsp_set_spatial_filter(fd_iav, 0, &adj_pipe_ctl.spatial_filter);
			adj_pipe_ctl.spatial_filter_update = 0;
		}
		if(adj_pipe_ctl.sharp_level_minimum_update) {
			img_dsp_set_sharpen_level_min(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_level_minimum);
			adj_pipe_ctl.sharp_level_minimum_update = 0;
		}
		if(adj_pipe_ctl.sharp_level_overall_update) {
			img_dsp_set_sharpen_level_overall(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_level_overall);
			adj_pipe_ctl.sharp_level_overall_update = 0;
		}
		if(adj_pipe_ctl.high_freq_noise_reduc_update) {
			img_dsp_set_luma_high_freq_noise_reduction(fd_iav, adj_pipe_ctl.high_freq_noise_reduc);
			adj_pipe_ctl.high_freq_noise_reduc_update = 0;
		}
		if(adj_pipe_ctl.sharp_retain_update) {
			adj_pipe_ctl.sharp_retain_update = 0;
		}
		if(adj_pipe_ctl.sharp_max_change_update) {
			img_dsp_set_sharpen_max_change(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_max_change);
			adj_pipe_ctl.sharp_max_change_update = 0;
		}
		if(adj_pipe_ctl.sharp_fir_update) {
			img_dsp_set_sharpen_fir(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_fir);
			adj_pipe_ctl.sharp_fir_update = 0;
		}
		if(adj_pipe_ctl.sharp_coring_update) {
			img_dsp_set_sharpen_coring(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_coring);
			adj_pipe_ctl.sharp_coring_update = 0;
		}

		if(adj_pipe_ctl.rgb_yuv_matrix_update) {
			rgb_to_yuv_t r2y_matrix = is_rgb2yuv[rgb2yuv_style];
			adj_set_color_conversion(&r2y_matrix);
			img_dsp_set_rgb2yuv_matrix(fd_iav, &r2y_matrix);
			adj_pipe_ctl.rgb_yuv_matrix_update = 0;
		}
		if(adj_pipe_ctl.gamma_update) {
			if(!adj_get_contrast_tone(adj_pipe_ctl.gamma_table.tone_curve_blue)) {
				memcpy(adj_pipe_ctl.gamma_table.tone_curve_red, adj_pipe_ctl.gamma_table.tone_curve_blue, 256*2);
				memcpy(adj_pipe_ctl.gamma_table.tone_curve_green, adj_pipe_ctl.gamma_table.tone_curve_blue, 256*2);
				img_dsp_set_tone_curve(fd_iav, &adj_pipe_ctl.gamma_table);
			}
			adj_pipe_ctl.gamma_update = 0;
		}



	}
}


int load_adj_cc_table(char *sensor_name)
{
	int file, count;
	char filename[128];
	u8 matrix[17536], reg[18752];
	u8 i, adj_mode = 4;
	static color_correction_reg_t color_corr_reg;

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
	color_corr_reg.reg_setting_addr=(u32)reg;
	img_dsp_set_color_correction_reg(&color_corr_reg);

	return 0;
}


int main(int argc, char **argv)
{
	int fd_iav;
	char sensor_name[32];
	struct amba_vin_source_info vin_info;
	sem_t sem;
	digital_sat_level_t dgain_sat = {16383, 16383, 16383, 16383};


	img_lib_init(1, 0);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}


	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}

	switch (vin_info.sensor_id) {
		case SENSOR_MT9T002:
			pat = GR;
			app_param_image_sensor.p_adj_param = &mt9t002_adj_param;
			app_param_image_sensor.p_rgb2yuv = mt9t002_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &mt9t002_chroma_scale;
			app_param_image_sensor.p_awb_param = &mt9t002_awb_param;
			app_param_image_sensor.p_tile_config = &mt9t002_tile_config;
			sprintf(sensor_name, "mt9t002");
			break;
		case SENSOR_MN34041PL:
			pat = BG;
			app_param_image_sensor.p_adj_param = &mn34041pl_adj_param;
			app_param_image_sensor.p_rgb2yuv = mn34041pl_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &mn34041pl_chroma_scale;
			app_param_image_sensor.p_awb_param = &mn34041pl_awb_param;
			app_param_image_sensor.p_tile_config = &mn34041pl_tile_config;
			sprintf(sensor_name, "mn34041pl");
			break;
		default:
			return -1;
	}

	load_adj_cc_table(sensor_name);
	memcpy(&is_awb_param, app_param_image_sensor.p_awb_param, sizeof(is_awb_param));
	memcpy(&is_adj_param, app_param_image_sensor.p_adj_param, sizeof(is_adj_param));
	memcpy(is_rgb2yuv, app_param_image_sensor.p_rgb2yuv, sizeof(is_rgb2yuv));
	memcpy(&is_chroma_scale, app_param_image_sensor.p_chroma_scale, sizeof(is_chroma_scale));
	memcpy(&is_tile_config, app_param_image_sensor.p_tile_config, sizeof(is_tile_config));
	
	//re-init idsp config start, use your own img_dsp_xxx and dsp driver instead of below ones.
	img_dsp_set_dgain_saturation_level(fd_iav, &dgain_sat);
	img_dsp_config_statistics_info(fd_iav, &is_tile_config);
	img_dsp_set_af_statistics_ex(fd_iav, &af_eng_cof,1);
	img_dsp_enable_color_correction(fd_iav);
	img_dsp_set_rgb2yuv_matrix(fd_iav, &is_rgb2yuv[rgb2yuv_style]);	//PC RGB2YUV matrix by default
	//re-init idsp config end

	pthread_create(&id, NULL, (void*)cus_main_loop, (void*)fd_iav);
	sem_init(&sem, 0, 0);
	sem_wait(&sem);

	return 0;
}

