/*
 *
 */
#include <config.h>

#include "Ir_Cut_Adc.h"
#include "Ir_Cut_Ae.h"
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
#include <pthread.h>
#include <math.h>

#include "ambas_vin.h"
#include "img_struct.h"
#include "img_api.h"
#include "img_dsp_interface.h"

#include "../idsp_test/arch_a5s/adj_params/imx035_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/imx035_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/imx036_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/imx036_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/imx072_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/imx072_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/ov2710_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/ov2710_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/ov9715_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/ov9715_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/ov5653_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/ov5653_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/ov14810_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/ov14810_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/mt9m033_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/mt9m033_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/mt9p006_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/mt9p006_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/mt9t002_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/mt9t002_aeb_param.c"

#include "../idsp_test/arch_a5s/adj_params/s5k5b3gx_adj_param.c"
#include "../idsp_test/arch_a5s/adj_params/s5k5b3gx_aeb_param.c"


#define REG_SIZE			18752
#define MATRIX_SIZE			17536
#define IRCUT_TEST_MODE_ADC 1
#define IRCUT_TEST_MODE_AE 0
static int g_ircut_test_mode = IRCUT_TEST_MODE_ADC; //0:using ae  1:using adc 


static int fd_iav;
statistics_config_t   TileConfig= {
	1,
	1,

	24,
	16,
	128,
	48,
	160,
	250,
	160,
	250,
	0,
	16383,

	12,
	8,
	0,
	8,
	340,
	512,

	8,
	5,
	256,
	10,
	448,
	816,
	448,
	816,

	0,
	16383,
};


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
	      	sprintf(sensor_name, "ov2710");
		printf("ov2710");
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
		sprintf(sensor_name, "ov9715");
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
		sprintf(sensor_name, "imx072");
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
		sprintf(sensor_name, "s5k5b3gx");
		break;
	default:
		printf("Unknow sensor id: %d \n", vin_src_info.sensor_id);
		return -1;
	}
	if (img_config_sensor_info(sensor_lb) < 0) {
		printf("img_config_sensor_info error!\n");
		return -1;
	}

	return 0;
}

static int load_dsp_cc_table(void)
{
	u8 reg[REG_SIZE], matrix[MATRIX_SIZE];
	int file, count;

	if ((file = open("/usr/local/bin/reg.bin", O_RDONLY, 0)) < 0) {
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

	if ((file = open("/usr/local/bin/3D.bin", O_RDONLY, 0)) < 0) {
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
	if (img_dsp_load_color_correction_table((u32)reg, (u32)matrix) < 0) {
		printf("img_dsp_load_color_correction_table error!\n");
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
		sprintf(filename, "/usr/local/bin/%s_0%d_3D.bin", sensor_name, (i+1));
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

	if (img_lib_init(0, 0) < 0) {
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

	img_dsp_config_statistics_info(fd_iav, &TileConfig);

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

int g_Auto=0;
int g_DNMode=0;
static pthread_mutex_t g_CtMsgLock	= PTHREAD_MUTEX_INITIALIZER;
int CallBackDNMode(unsigned char *pSetDNMode,unsigned char *pGetDNMode)
{	
	pthread_mutex_lock(&g_CtMsgLock);
	if(g_Auto==1&&pSetDNMode)
	{
		if(img_set_bw_mode(*pSetDNMode)<0){
			printf("img_set_bw_mode error!\n");
		//	pthread_mutex_unlock(&g_CtMsgLock);
		//	return -1;
		}else
			g_DNMode = *pSetDNMode;
		
		if (*pSetDNMode){// night
			system("/usr/local/bin/a5s_debug -g 83 -d 0x00");
			system("/usr/local/bin/a5s_debug -g 84 -d 0x01");
		}else{
			system("/usr/local/bin/a5s_debug -g 83 -d 0x01");
			system("/usr/local/bin/a5s_debug -g 84 -d 0x00");
		}
	}
	if(pGetDNMode)
	{
		*pGetDNMode = g_DNMode;
	}
	if(pSetDNMode)
	{
		printf("g_Auto=%d\n",g_Auto);
		printf("g_DNMode=%d\n",g_DNMode);
		printf("SetDNMode=%d\n",*pSetDNMode);
	}
	pthread_mutex_unlock(&g_CtMsgLock);
	if(pSetDNMode)
	{
		sleep(2);
	}
	return 0;
}

int CallBackGetAeGain(unsigned int *pAeGain)
{
	*pAeGain = img_ae_get_system_gain();
	if((*pAeGain)<0) {
		printf("img_ae_get_system_gain error!\n");
		return -1;
	}
	return 0;
}


int CallBackGetIrLightStatus()
{
	return 0;
}

#ifdef CONFIG_APP_IPCAM_HAVE_EXTERNAL_IR_LIGHT
void CallBackGetShutterIndex(int *pIndex)
{
	*pIndex = img_get_sensor_shutter_index();
	if((*pIndex)<0)
		printf("img_get_sensor_shutter_index error!\n");
}

static int CallBackGetMagnification(double *pMagnification)
{
	int nSystemGain;
 	if (pMagnification) {
		nSystemGain = img_ae_get_system_gain();
		if(nSystemGain<0)
		{
			printf("img_ae_get_system_gain error!\n");
			return -1;	
		}
		*pMagnification = pow(10,(((double)nSystemGain*6)/128)/20);
	}else
		return -1;
	
	return 0;
}

static int CallBackGetShutterTime(double *pShutterTime)
{
	int nShutterTime;
 	if (pShutterTime) {
		if(ioctl(fd_iav,IAV_IOC_VIN_SRC_GET_SHUTTER_TIME,&nShutterTime)<0)
		{
			printf("ioctl IAV_IOC_VIN_SRC_GET_SHUTTER_TIME error!\n");
			return -1;	
		}
		*pShutterTime = nShutterTime/512000000.0;
	}else
		return -1;
	
	return 0;
}

static int CallBackGetAeLuma(int *pAeLuma)
{
 	if (pAeLuma) {
		*pAeLuma = img_get_ae_luma_value();
		if((*pAeLuma)<0)
		{
			printf("img_get_ae_luma_value error!\n");
			return -1;	
		}
	}else
		return -1;
	
	return 0;
}

static int CallBackGetCFA_RGB(int *pR,int *pG,int *pB)
{
	int i;
	rgb_aaa_stat_t rgb_stat;
	cfa_aaa_stat_t cfa_stat;	
	int tile_num;	
	if(img_dsp_get_statistics_raw (fd_iav, &rgb_stat, &cfa_stat))
	{
		printf("img_dsp_get_statistics_raw error!\n");
		return -1;		
	}
	tile_num = cfa_stat.tile_info.awb_tile_num_col*cfa_stat.tile_info.awb_tile_num_row;
	for(i=0;i<tile_num;i++)
	{
		*pR += cfa_stat.awb_stat[i].sum_r;
		*pG += cfa_stat.awb_stat[i].sum_g;
		*pB += cfa_stat.awb_stat[i].sum_b;
	}
	*pR /= tile_num;
	*pG /= tile_num;
	*pB /= tile_num;	
	return 0;
}
#endif


int main(int argc, char **argv)
{
	int i;
	int dbg_level = 0;// 0:error 1:warning 2:debug
	unsigned int nDurTime;
	unsigned int nDarkThr;
	unsigned int nNightToDayThr;
	unsigned int nDayToNightThr;

	int shutterIndex[21]={256,384,512,640,756,768,809,850,884,937,978,1012,1106,1140,1234,1268,1396,1524,1652,1664,2046};
	int nIndex=0;
	if (start_aaa() < 0)
		return -1;

	if(img_set_bw_mode(0)<0)
		printf("img_set_bw_mode error!\n");
	g_Auto = 1;
	g_DNMode = 0;



	#ifdef CONFIG_APP_IPCAM_HAVE_EXTERNAL_IR_LIGHT
	if(IrCut_Enable_Ae(CallBackDNMode,CallBackGetMagnification,CallBackGetShutterTime,CallBackGetAeLuma,CallBackGetCFA_RGB)<0)
		printf("IrCut_Enable_Ae error!\n");
	#else
	if( g_ircut_test_mode == IRCUT_TEST_MODE_AE )
	{
		if(IrCut_Enable_Ae(CallBackDNMode,CallBackGetAeGain)<0){
			printf("IrCut_Enable_Ae error!\n");
		}
	}
	else
	{
		if(IrCut_Enable_Adc(CallBackDNMode,CallBackGetAeGain, CallBackGetIrLightStatus,NULL,NULL )<0)
		printf("IrCut_Enable_Adc error!\n");
	}
	#endif

	printf(" 0: SetDayMode\n 1: SetNeightMode\n 2: SetAutoMode\n 3: EnableIrCut\n 4: DisabelIrCut\n 5:SetPara\n 6:ShutterUp\n 7:ShutterDown\n 8: quit\n 9:SetDebugLevel\n ");
	while(1)
	{
		scanf("%d", &i);
		switch(i){
			case(0):
				if(img_set_bw_mode(0)<0)
					printf("img_set_bw_mode error!\n");
				else
				{
					pthread_mutex_lock(&g_CtMsgLock);
					g_Auto = 0;
					g_DNMode = 0;
					pthread_mutex_unlock(&g_CtMsgLock);
					system("/usr/local/bin/a5s_debug -g 83 -d 0x01");
					system("/usr/local/bin/a5s_debug -g 84 -d 0x00");
				}
				break;
			case(1):
				if(img_set_bw_mode(1)<0)
					printf("img_set_bw_mode error!\n");
				else
				{
					pthread_mutex_lock(&g_CtMsgLock);
					g_Auto = 0;
					g_DNMode = 1;
					pthread_mutex_unlock(&g_CtMsgLock);
					system("/usr/local/bin/a5s_debug -g 83 -d 0x00");
					system("/usr/local/bin/a5s_debug -g 84 -d 0x01");
				}
				break;
			case(2):
				if(img_set_bw_mode(0)<0)
					printf("img_set_bw_mode error!\n");
				else
				{
					pthread_mutex_lock(&g_CtMsgLock);
					g_Auto = 1;
					g_DNMode = 0;
					pthread_mutex_unlock(&g_CtMsgLock);
					system("/usr/local/bin/a5s_debug -g 83 -d 0x01");
					system("/usr/local/bin/a5s_debug -g 84 -d 0x00");
				}
				break;
			case(3):

			#ifdef CONFIG_APP_IPCAM_HAVE_EXTERNAL_IR_LIGHT
				if(IrCut_Enable_Ae(CallBackDNMode,CallBackGetMagnification,CallBackGetShutterTime,CallBackGetAeLuma,CallBackGetCFA_RGB)<0)
					printf("IrCut_Enable_Ae error!\n");
			#else
				//if(IrCut_Enable_Ae(CallBackDNMode,CallBackGetAeGain)<0){
				//	printf("IrCut_Enable_Ae error!\n");
				//}
				if(IrCut_Enable_Adc(CallBackDNMode,CallBackGetAeGain,CallBackGetIrLightStatus,NULL,NULL)<0)
					printf("IrCut_Enable_Adc error!\n");
			#endif

				break;
			case(4):
				if( g_ircut_test_mode == IRCUT_TEST_MODE_AE )
				{
					if(IrCut_Disable_Ae(CallBackDNMode,CallBackGetAeGain)<0)
					printf("IrCut_Disable_Ae error!\n");
				}
				else
				{
					if(IrCut_Disable_Adc(CallBackDNMode,CallBackGetAeGain)<0)
						printf("IrCut_Disable_Adc error!\n");
				}
				break;
			case(5):
				printf("Input DurTime,default 10");
				scanf("%d",&nDurTime);
				if( g_ircut_test_mode == IRCUT_TEST_MODE_AE )
				{
					printf("Input DarkThr,default 600");
					scanf("%d",&nDarkThr);
					IrCut_SetParameter(nDurTime, nDarkThr);
				}
				printf("Input nDayToNightThr,default 30");
				scanf("%d",&nDayToNightThr);
				printf("Input nNightToDayThr,default 10");
				scanf("%d",&nNightToDayThr);
				if( g_ircut_test_mode == IRCUT_TEST_MODE_AE )
				{
					IrCut_SetParameter_Ae(nDurTime, nDayToNightThr, nNightToDayThr);
				}
				else
				{
					IrCut_SetParameter_Adc(nDurTime, nDayToNightThr, nNightToDayThr);
				}
				break;
			case(6):
				nIndex++;
				if(nIndex>=21)
					nIndex = 20;
				img_set_sensor_shutter_index(fd_iav,shutterIndex[nIndex]);
				break;
			case(7):
				nIndex--;
				if(nIndex<0)
					nIndex = 0;
				img_set_sensor_shutter_index(fd_iav,shutterIndex[nIndex]);
				break;
			case (9):
				printf("Input dbg_level(default 0): >");
				scanf("%d",&dbg_level);
				if( g_ircut_test_mode == IRCUT_TEST_MODE_AE )
				{
					IrCut_SetDebugLevel_Ae(dbg_level);
				}
				else
				{
				IrCut_SetDebugLevel_Adc(dbg_level);
				}
				break;
			default:
				break;	
		}
		if(i==8)
			break;
	}

	return 0;
}


