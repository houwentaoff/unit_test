/**************************
* Main file
*
***************************/
#include         <stdlib.h>
#include         <stdio.h>
#include         <string.h>
#include         <fcntl.h>

#include         "demo_main.h"
#include         "img_struct.h"
#include         "img_dsp_interface.h"
#include         "ambas_imgproc.h"

#include         "customer_ae_algo.h"
#include         "customer_awb_algo.h"
#include         "img_api.h"

static      aaa_tile_info_t     TileInfo;
static      ae_data_t           AEInfo[96];
static      awb_data_t         AWBInfo[384];
static      af_stat_t             AFInfo[40];
static 	embed_hist_stat_t hist_info;
static 	aaa_tile_report_t act_tile;
static 	af_stat_t rgb_af_info[40];
static      awb_gain_t         AWBGain;
static      ae_info_t            AEVideo;

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

int main( )
{
      int       fd_iav;
      static int shutter_change = 0;
	extern  u16 sensor_double_step;

      if(img_lib_init(0,0)<0) {
		perror("/dev/iav");
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	img_config_sensor_info(OV_2710);                                                      // OV_5653, OV_9715, MT_9J001, MT_9M033, MT_9P031, IMX_035, IMX_036;
	img_dsp_config_statistics_info(fd_iav, &TileConfig);

	customer_ae_control_init( );
	customer_awb_control_init();

	while( 1 ) {
		embed_hist_stat_t hist_info;
		aaa_tile_report_t act_tile;
		af_stat_t rgb_af_info;
		istogram_stat_t tmp_histogram_stat;
		img_dsp_get_statistics(fd_iav, &TileInfo, AEInfo, AWBInfo, AFInfo, &hist_info, &act_tile, &rgb_af_info, &tmp_histogram_stat);
		
		customer_ae_control(AEInfo, &AEVideo); 
		
		customer_awb_control(AWBInfo, &AWBGain);

		img_dsp_set_rgb_gain(fd_iav, &AWBGain.video_wb_gain, 1024);

		if (AEVideo.shutter_update == 1) {
			img_set_sensor_shutter_index(fd_iav, AEVideo.shutter_index);
			shutter_change = 1;
			AEVideo.shutter_update = 0;
		}

		if ((AEVideo.agc_update == 1)&&(!shutter_change)) {
			img_set_sensor_agc_index(fd_iav, AEVideo.agc_index, sensor_double_step);
			AEVideo.agc_update = 0;
		}

		if (shutter_change-- < 0)
			shutter_change = 0;
		
		AWBGain.video_gain_update = 0;
		AEVideo.dgain_update = 0;

	}
	return 0;
}
