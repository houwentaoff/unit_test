/*
 * test_encode.c
 * the program can setup VIN , preview and start encoding/stop
 * encoding for flexible multi streaming encoding.
 * after setup ready or start encoding/stop encoding, this program
 * will exit
 *
 * History:
 *	2010/12/31 - [Louis Sun] create this file base on test2.c
 *	2011/10/31 - [Jian Tang] modified this file.
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include <signal.h>


#undef  CHECK_DELAY

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif


#define MAX_ENCODE_STREAM_NUM		4
#define MAX_SOURCE_BUFFER_NUM		4
#define MAX_PREVIEW_BUFFER_NUM		2


#define ALL_ENCODE_STREAMS 0x0F		//support up to 4 streams by this design
#define ALL_SOURCE_BUFFER  0x03		//support up to 4 source buffer by this design

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )

#define COLOR_PRINT0(msg)		printf("\033[34m"msg"\033[39m")
#define COLOR_PRINT(msg, arg...)		printf("\033[34m"msg"\033[39m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)


/* osd blending mixer selection */
#define OSD_BLENDING_OFF				0
#define OSD_BLENDING_FROM_MIXER_A		1
#define OSD_BLENDING_FROM_MIXER_B		2

//static struct timeval start_enc_time;

/*
 * use following commands to enable timer1
 *

amba_debug -w 0x7000b014 -d 0xffffffff
amba_debug -w 0x7000b030 -d 0x15
amba_debug -r 0x7000b000 -s 0x40

*/

// the device file handle
static int fd_iav;

// bit stream buffer
static u8 * bsb_mem;
static u32 bsb_size;

// QP Matrix buffer
static u8 * G_qp_matrix_addr = NULL;
static int G_qp_matrix_size = 0;

// vin
#include "../../vin_test/vin_init.c"
#include "../../vout_test/vout_init.c"

#define VERIFY_STREAMID(x)   do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf ("stream id wrong %d \n", (x));			\
				return -1; 	\
			}	\
		} while (0)

#define VERIFY_BUFFERID(x)	do {		\
			if ((x) < 0 || ((x) >= MAX_SOURCE_BUFFER_NUM)) {	\
				printf ("buffer id wrong %d\n", (x));			\
				return -1;	\
			}	\
		} while (0)

#define VERIFY_TYPE_CHANGEABLE_BUFFERID(x)	do {		\
			if ((x) < 1 || ((x) >= MAX_SOURCE_BUFFER_NUM)) {	\
				printf ("buffer id wrong %d, cannot change type\n", (x));			\
				return -1;	\
			}	\
		} while (0)

//encode format
typedef struct encode_format_s {
	int type;
	int type_changed_flag;

	int width;
	int height;
	int resolution_changed_flag;

	int offset_x;
	int offset_y;
	int offset_changed_flag;

	int source;
	int source_changed_flag;

} encode_format_t;

//source buffer type
typedef struct source_buffer_type_s {
	iav_source_buffer_type_ex_t buffer_type;
	int buffer_type_changed_flag;
}  source_buffer_type_t;

//source buffer format
typedef struct source_buffer_format_s {
	int width;
	int height;
	int resolution_changed_flag;

	int input_width;
	int input_height;
	int input_window_changed_flag;

	int deintlc_option;
	int deintlc_option_changed_flag;

	int intlc_scan;
	int intlc_scan_changed_flag;

}  source_buffer_format_t;

//preview buffer format (preview buffer is a special case of source buffer)
typedef struct preview_buffer_format_s {
	int width;
	int height;
	int resolution_changed_flag;
} preview_buffer_format_t;

//max buffer size
typedef struct source_buffer_max_size_s {
	int width;
	int height;
	int resolution_changed_flag;
} source_buffer_max_size_t;

//max stream size
typedef struct encoding_stream_max_size_s {
	int width;
	int height;
	int resolution_changed_flag;
} encoding_stream_max_size_t;

//qp limit
typedef struct qp_limit_params_s {
	u8	qp_min_i;
	u8	qp_max_i;
	u8	qp_min_p;
	u8	qp_max_p;
	u8	qp_min_b;
	u8	qp_max_b;
	u8	adapt_qp;
	u8	i_qp_reduce;
	u8	p_qp_reduce;
	u8	skip_frame;

	u8	qp_i_flag;
	u8	qp_p_flag;
	u8	qp_b_flag;
	u8	adapt_qp_flag;
	u8	i_qp_reduce_flag;
	u8	p_qp_reduce_flag;
	u8	skip_frame_flag;
} qp_limit_params_t;

// qp matrix delta value
typedef struct qp_matrix_params_s {
	s8	delta[NUM_FRAME_TYPES][4];
	int	delta_flag;

	int	matrix_mode;
	int	matrix_mode_flag;
} qp_matrix_params_t;

// panic mode control paramters
typedef struct panic_control_params_s {
	int panic_div;
	int pic_size_control;
	int panic_param_flag;
} panic_control_params_t;

// h.264 config
typedef struct h264_param_s {
	int h264_M;
	int h264_N;
	int h264_idr_interval;
	int h264_gop_model;
	int h264_bitrate_control;
	int h264_cbr_avg_bitrate;
	int h264_vbr_min_bitrate;
	int h264_vbr_max_bitrate;

	int h264_deblocking_filter_alpha;
	int h264_deblocking_filter_beta;
	int h264_deblocking_filter_enable;

	int h264_hflip;
	int h264_vflip;
	int h264_rotate;
	int h264_chrome_format;			// 0: YUV420; 1: Mono
	int h264_high_profile;

	int h264_M_flag;
	int h264_N_flag;
	int h264_idr_interval_flag;
	int h264_gop_model_flag;
	int h264_bitrate_control_flag;
	int h264_cbr_bitrate_flag;
	int h264_vbr_bitrate_flag;

	int h264_deblocking_filter_alpha_flag;
	int h264_deblocking_filter_beta_flag;
	int h264_deblocking_filter_enable_flag;

	int h264_hflip_flag;
	int h264_vflip_flag;
	int h264_rotate_flag;
	int h264_chrome_format_flag;
	int h264_high_profile_flag;

	int h264_entropy_codec;
	int h264_entropy_codec_flag;

	int force_intlc_tb_iframe;
	int force_intlc_tb_iframe_flag;

	int au_type;
	int au_type_flag;

	int cpb_buf_idc;
	int cpb_cmp_idc;
	int en_panic_rc;
	int fast_rc_idc;
	int cpb_user_size;
	int panic_mode_flag;
} h264_param_t;

typedef struct jpeg_param_s{
	int quality;
	int quality_changed_flag;

	int jpeg_hflip;
	int jpeg_vflip;
	int jpeg_rotate;
	int jpeg_chrome_format;			// 1: YUV420; 2: Mono

	int jpeg_hflip_flag;
	int jpeg_vflip_flag;
	int jpeg_rotate_flag;
	int jpeg_chrome_format_flag;
} jpeg_param_t;

//use struct instead of union here, so that mis config of jpeg param does not break h264 param
typedef struct encode_param_s{
	h264_param_t h264_param;
	jpeg_param_t jpeg_param;
} encode_param_t;

// state control
static int idle_cmd = 0;
static int preview_cmd = 0;
static int nopreview_flag = 0;		// do not enter preview automatically

//stream and source buffer identifier
static int current_stream = -1; 	// -1 is a invalid stream, for initialize data only
static int current_buffer = -1;	// -1 is a invalid buffer, for initialize data only

//encode start/stop/format control variables
static iav_stream_id_t start_stream_id = 0;
static iav_stream_id_t stop_stream_id = 0;

//encoding settings
static encode_format_t encode_format[MAX_ENCODE_STREAM_NUM];
static encode_param_t encode_param[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t  encode_format_changed_id = 0;
static iav_stream_id_t encode_param_changed_id  = 0;
static iav_stream_id_t GOP_changed_id  = 0;
static iav_stream_id_t chroma_format_changed_id = 0;

//low delay cap setting
static int low_delay_cap = 0;
static int low_delay_cap_flag = 0;

//disable oversampling
static int oversampling_disable = 0;
static int oversampling_disable_flag = 0;

//hd sdi mode
static int hd_sdi_mode = 0;
static int hd_sdi_mode_flag = 0;

//system information display
static int show_iav_state_flag = 0;
static int show_encode_stream_info_flag = 0;
static int show_encode_config_flag = 0;
static int show_source_buffer_info_flag = 0;
static int show_resource_limit_info_flag = 0;
static int show_iav_driver_info_flag = 0;
static int show_chip_info_flag = 0;

//source buffer settings
static source_buffer_type_t source_buffer_type[MAX_SOURCE_BUFFER_NUM];
static source_buffer_format_t  source_buffer_format[MAX_SOURCE_BUFFER_NUM];
static int source_buffer_type_changed_flag = 0;
static iav_stream_id_t source_buffer_format_changed_id = 0;

//preview buffer settings
static preview_buffer_format_t  preview_buffer_format[MAX_PREVIEW_BUFFER_NUM];
static int preview_buffer_format_changed_flag = 0;

// system resource settings
static source_buffer_max_size_t source_buffer_max_size[MAX_SOURCE_BUFFER_NUM];
static encoding_stream_max_size_t stream_max_size[MAX_ENCODE_STREAM_NUM];
static int source_buffer_max_size_changed_id = 0;
static int stream_max_size_changed_id = 0;
static int MCTF_possible = 1;
static int MCTF_possible_changed_flag = 0;
static int stream_max_gop_m = 1;
static int stream_max_gop_m_flag = 0;
static int cavlc_max_bitrate = 1500000;
static int cavlc_max_bitrate_flag = 0;
static int system_resource_limit_changed_flag = 0;

// system info settings
static int osd_mixer = 0;
static int osd_mixer_changed_flag = 0;
static int read_encode_info_mode = 0;
static int read_encode_info_mode_flag = 0;
static int pip_size_enable = 0;
static int pip_size_enable_flag = 0;
static int audio_clk_freq = 48000;
static int audio_clk_freq_flag = 0;

//force idr generation
static iav_stream_id_t force_idr_id = 0;

//encoding frame rate settings
static iav_stream_id_t framerate_factor_change_id = 0;
static iav_stream_id_t framerate_factor_sync_id = 0;
static int framerate_factor_change[MAX_ENCODE_STREAM_NUM][2];

//encoding intrabias settings
static iav_stream_id_t intrabias_p_change_id = 0;
static iav_stream_id_t intrabias_b_change_id = 0;
static int intrabias_p_change[MAX_ENCODE_STREAM_NUM];
static int intrabias_b_change[MAX_ENCODE_STREAM_NUM];

//tune the AQP and mode bias
static iav_h264_bias_param_ex_t bias_param[MAX_ENCODE_STREAM_NUM];
static int bias_param_changed_id =0;

//enable/disable additional idr when scene change detect
static iav_stream_id_t scene_detect_id = 0;
static int enable_scene_detect[MAX_ENCODE_STREAM_NUM];

//rate control settings
static qp_limit_params_t qp_limit_param[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t qp_limit_changed_id = 0;

//intra refresh MB rows
static int intra_mb_rows[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t intra_mb_rows_changed_id = 0;

//qp matrix settings
static qp_matrix_params_t qp_matrix_param[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t qp_matrix_changed_id = 0;

//misc settings
static iav_stream_id_t restart_stream_id = 0;
static int dump_idsp_section_id = 0;
static int dump_idsp_bin_flag = 0;

//QP histogram
static u8 show_qp_hist_info_flag = 0;

//preview A framerate divisor
static u8 prev_a_framerate_div = 1;
static u8 prev_a_framerate_div_flag = 0;

//panic mode control parameters
static panic_control_params_t panic_control_param[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t panic_control_param_changed_id = 0;

struct encode_resolution_s {
	const char 	*name;
	int		width;
	int		height;
}
__encode_res[] =
{
	{"4kx3k",4016,3016},
	{"4kx2k",4096,2160},
	{"4k",4096,2160},
	{"qfhd",3840,2160},
	{"5m", 2592, 1944},
	{"5m_169", 2976, 1674},
	{"3m", 2048, 1536},
	{"3m_169", 2304, 1296},
	{"1080p", 1920, 1080},
	{"720p", 1280, 720},
	{"480p", 720, 480},
	{"576p", 720, 576},
	{"4sif", 704, 480},
	{"4cif", 704, 576},
	{"xga", 1024, 768},
	{"vga", 640, 480},
	{"wvga", 800, 480},
	{"fwvga", 854, 480},	//a 16:9 style
	{"cif", 352, 288},
	{"sif", 352, 240},
	{"qvga", 320, 240},
	{"qwvga", 400, 240},
	{"qcif", 176, 144},
	{"qsif", 176, 120},
	{"qqvga", 160, 120},
	{"svga", 800, 600},
	{"sxga", 1280, 1024},
	{"480i", 720, 480},
	{"576i", 720, 576},
	{"1080i", 1920, 1080},

	{"", 0, 0},

	{"1920x1080", 1920, 1080},
	{"1600x1200", 1600, 1200},
	{"1440x1080", 1440, 1080},
	{"1366x768", 1366, 768},
	{"1280x1024", 1280, 1024},
	{"1280x960", 1280, 960},
	{"1280x720", 1280, 720},
	{"1024x768", 1024, 768},
	{"720x480", 720, 480},
	{"720x576", 720, 576},

	{"", 0, 0},

	{"704x480", 704, 480},
	{"704x576", 704, 576},
	{"640x480", 640, 480},
	{"352x288", 352, 288},
	{"352x256", 352, 256},	//used for interlaced MJPEG 352x256 encoding ( crop to 352x240 by app)
	{"352x240", 352, 240},
	{"320x240", 320, 240},
	{"176x144", 176, 144},
	{"176x120", 176, 120},
	{"160x120", 160, 120},

	//vertical video resolution
	{"480x640", 480, 640},
	{"480x854", 480, 854},

	//for preview size only to keep aspect ratio in preview image for different VIN aspect ratio
	{"16_9_vin_ntsc_preview", 720, 360},
	{"16_9_vin_pal_preview", 720, 432},
	{"4_3_vin_ntsc_preview", 720, 480},
	{"4_3_vin_pal_preview", 720, 576},
	{"5_4_vin_ntsc_preview", 672, 480},
	{"5_4_vin_pal_preview", 672, 576 },
	{"ntsc_vin_ntsc_preview", 720, 480},
	{"pal_vin_pal_preview", 720, 576},
};

#define	NO_ARG		0
#define	HAS_ARG		1
#define	SYSTEM_OPTIONS_BASE		0
#define	VIN_OPTIONS_BASE		10
#define	VOUT_OPTIONS_BASE		20
#define	PREVIEW_OPTIONS_BASE		60
#define	ENCODING_OPTIONS_BASE		130
#define	MISC_OPTIONS_BASE				210


enum numeric_short_options {
	// System
	SYSTEM_IDLE = SYSTEM_OPTIONS_BASE,
	SPECIFY_MCTF_POSSIBLE,
	DUMP_IDSP_CONFIG,

	// VIN
	VIN_NUMVERIC_SHORT_OPTIONS,

	// Vout
	VOUT_NUMERIC_SHORT_OPTIONS,

	// Preview
	NO_PREVIEW = PREVIEW_OPTIONS_BASE,

	// Encoding
	ENCODING_RESTART = ENCODING_OPTIONS_BASE,
	ENCODING_OFFSET_X,
	ENCODING_OFFSET_Y,
	ENCODING_MAX_SIZE,
	SPECIFY_FORCE_IDR,
	BITRATE_CHANGE,
	FRAME_FACTOR,
	FRAME_FACTOR_SYNC,
	PREV_A_FRAMERATE_DIV,
	SPECIFY_GOP_IDR,
	SPECIFY_GOP_MODEL,
	BITRATE_CONTROL,
	SPECIFY_CBR_BITRATE,
	SPECIFY_VBR_BITRATE,
	DEBLOCKING_ALPHA,
	DEBLOCKING_BETA,
	DEBLOCKING_ENABLE,
	SPECIFY_CAVLC,
	SPECIFY_CAVLC_MAX_BITRATE,
	SPECIFY_HIGH_PROFILE,
	LOWDELAY_REFRESH,
	INTLC_IFRAME,
	SPECIFY_AU_TYPE,
	SPECIFY_HFLIP,
	SPECIFY_VFLIP,
	SPECIFY_ROTATE,
	SPECIFY_CHROME_FORMAT,
	MJPEG_QUALITY,
	SPECIFY_BUFFER_TYPE,
	SPECIFY_BUFFER_SIZE,
	SPECIFY_BUFFER_MAX_SIZE,
	SPECIFY_BUFFER_INPUT_SIZE,
	SPECIFY_BUFFER_DEINTLACE,
	SPECIFY_BUFFER_INTERLACE,
	SPECIFY_OSD_MIXER,
	CHANGE_QP_LIMIT_I,
	CHANGE_QP_LIMIT_P,
	CHANGE_QP_LIMIT_B,
	CHANGE_ADAPT_QP,
	CHANGE_I_QP_REDUCE,
	CHANGE_P_QP_REDUCE,
	CHANGE_SKIP_FRAME_MODE,
	CHANGE_INTRA_MB_ROWS,
	CHANGE_QP_MATRIX_DELTA_I,
	CHANGE_QP_MATRIX_DELTA_P,
	CHANGE_QP_MATRIX_DELTA_B,
	CHANGE_QP_MATRIX_MODE,
	CPB_BUF_IDC,
	CPB_CMP_IDC,
	ENABLE_PANIC_RC,
	FAST_RC_IDC,
	CPB_USER_SIZE,
	SPECIFY_MAX_GOP_M,
	SPECIFY_READOUT_MODE,
	SPECIFY_PIP_SIZE,
	SPECIFY_AUDIO_CLK_FREQ,
	PANIC_DIV,
	PIC_SIZE_CONTROL,
	LOW_DELAY_CAP_ENABLE,
	OVERSAMPLING_DISABLE,
	HD_SDI_MODE,
	SPECIFY_INTRABIAS_P,
	SPECIFY_INTRABIAS_B,
	SPECIFY_SCENE_CHANGE_DETECT,
	CHANGE_INTRA_16X16_BIAS,
	CHANGE_INTRA_4X4_BIAS,
	CHANGE_INTER_16X16_BIAS,
	CHANGE_INTER_8X8_BIAS,
	CHANGE_DIRECT_16X16_BIAS,
	CHANGE_DIRECT_8X8_BIAS,
	CHANGE_ME_LAMBDA_QP_OFFSET,

	// Misc
	SHOW_SYSTEM_STATE = MISC_OPTIONS_BASE,
	SHOW_ENCODE_CONFIG,
	SHOW_STREAM_INFO,
	SHOW_QP_HIST_INFO,
	SHOW_BUFFER_INFO,
	SHOW_RESOURCE_INFO,
	SHOW_DRIVER_INFO,
	SHOW_CHIP_INFO,
	SHOW_ALL_INFO,
};

static struct option long_options[] = {
	{"stream-A",	NO_ARG,		0,	'A' },   // -A xxxxx    means all following configs will be applied to stream A
	{"stream-B",	NO_ARG,		0,	'B' },
	{"stream-C",	NO_ARG,		0,	'C' },
	{"stream-D",	NO_ARG,		0,	'D' },

	{"h264", 		HAS_ARG,	0,	'h'},
	{"mjpeg",	HAS_ARG,	0,	'm'},
	{"none",		NO_ARG,		0,	'n'},
	{"src-buf",	HAS_ARG,	0,	'b' },	//encode source buffer
	{"offset-x",	HAS_ARG,	0,	ENCODING_OFFSET_X },	//encoding offset
	{"offset-y",	HAS_ARG,	0,	ENCODING_OFFSET_Y },	//encoding offset
	{"smaxsize",	HAS_ARG,	0,	ENCODING_MAX_SIZE},	//encoding max size

	//immediate action, configure encode stream on the fly
	{"encode",	NO_ARG,		0,	'e'},		//start encoding
	{"stop",		NO_ARG,		0,	's'},		//stop encoding
	{"force-idr",	NO_ARG,		0,	SPECIFY_FORCE_IDR },
	{"restart",		NO_ARG,		0,	ENCODING_RESTART },			//immediate stop and start encoding
	{"frame-factor",	HAS_ARG,	0,	FRAME_FACTOR },
	{"frame-factor-sync",	HAS_ARG,	0, FRAME_FACTOR_SYNC },
	{"prev-a-frame-divisor",	HAS_ARG,	0,	PREV_A_FRAMERATE_DIV },

	//H.264 encode configurations
	{"M",		HAS_ARG,	0,	'M' },
	{"N",		HAS_ARG,	0,	'N'},
	{"idr",		HAS_ARG,	0,	SPECIFY_GOP_IDR},
	{"gop",		HAS_ARG,	0,	SPECIFY_GOP_MODEL},
	{"bc",		HAS_ARG,	0,	BITRATE_CONTROL},
	{"bitrate",	HAS_ARG,	0,	SPECIFY_CBR_BITRATE},
	{"vbr-bitrate",	HAS_ARG,	0,	SPECIFY_VBR_BITRATE},
	{"qp-limit-i", 	HAS_ARG, 0, CHANGE_QP_LIMIT_I},
	{"qp-limit-p", 	HAS_ARG, 0, CHANGE_QP_LIMIT_P},
	{"qp-limit-b", 	HAS_ARG, 0, CHANGE_QP_LIMIT_B},
	{"adapt-qp",		HAS_ARG, 0, CHANGE_ADAPT_QP},
	{"i-qp-reduce",	HAS_ARG, 0, CHANGE_I_QP_REDUCE},
	{"p-qp-reduce",	HAS_ARG, 0, CHANGE_P_QP_REDUCE},
	{"skip-frame-mode",	HAS_ARG, 0, CHANGE_SKIP_FRAME_MODE},
	{"intra-mb-rows",	HAS_ARG, 0, CHANGE_INTRA_MB_ROWS},
	{"qm-delta-i",	HAS_ARG,	0,	CHANGE_QP_MATRIX_DELTA_I},
	{"qm-delta-p",	HAS_ARG,	0,	CHANGE_QP_MATRIX_DELTA_P},
	{"qm-delta-b",	HAS_ARG,	0,	CHANGE_QP_MATRIX_DELTA_B},
	{"qm-mode",	HAS_ARG,	0,	CHANGE_QP_MATRIX_MODE},

	{"deblocking-alpha",	HAS_ARG,	0,	DEBLOCKING_ALPHA},
	{"deblocking-beta",	HAS_ARG,	0,	DEBLOCKING_BETA},
	{"deblocking-enable",	HAS_ARG,	0,	DEBLOCKING_ENABLE},
	{"cavlc",		HAS_ARG,	0,	SPECIFY_CAVLC},
	{"cavlc-mbps",	HAS_ARG,	0,	SPECIFY_CAVLC_MAX_BITRATE},
	{"high-profile",	HAS_ARG,	0,	SPECIFY_HIGH_PROFILE},
	{"max-gop-M",	HAS_ARG,	0,	SPECIFY_MAX_GOP_M},	//max gop m for stream A
	{"read-mode",	HAS_ARG,	0,	SPECIFY_READOUT_MODE},
	{"pip-size",		HAS_ARG,	0,	SPECIFY_PIP_SIZE},
	{"audio-clk",		HAS_ARG,	0,	SPECIFY_AUDIO_CLK_FREQ},

	{"lowdelay-refresh",	HAS_ARG,	0,	LOWDELAY_REFRESH},

	{"intlc_iframe",		HAS_ARG,	0,	INTLC_IFRAME},
	{"lowdelay-cap",	HAS_ARG, 0, LOW_DELAY_CAP_ENABLE},
	{"oversampling-disable",	HAS_ARG,	0,	OVERSAMPLING_DISABLE},
	{"hd-sdi-mode",	HAS_ARG,	0,	HD_SDI_MODE},
	{"intrabias-p",	HAS_ARG, 0, SPECIFY_INTRABIAS_P},
	{"intrabias-b",	HAS_ARG, 0, SPECIFY_INTRABIAS_B},
	{"scene-detect",	HAS_ARG, 0, SPECIFY_SCENE_CHANGE_DETECT},

//bias params
	{"intra-16x16bias",	HAS_ARG, 0, CHANGE_INTRA_16X16_BIAS},
	{"intra-4x4bias",		HAS_ARG, 0, CHANGE_INTRA_4X4_BIAS},
	{"inter-16x16bias",	HAS_ARG, 0, CHANGE_INTER_16X16_BIAS},
	{"inter-8x8bias",		HAS_ARG, 0, CHANGE_INTER_8X8_BIAS},
	{"direct-16x16bias",	HAS_ARG, 0, CHANGE_DIRECT_16X16_BIAS},
	{"direct-8x8bias",	HAS_ARG, 0, CHANGE_DIRECT_8X8_BIAS},
	{"qp_offset",	HAS_ARG, 0, CHANGE_ME_LAMBDA_QP_OFFSET},

	//panic mode
	{"cpb-buf-idc",	HAS_ARG, 0, CPB_BUF_IDC},
	{"cpb-cmp-idc",	HAS_ARG, 0, CPB_CMP_IDC},
	{"en-panic-rc",	HAS_ARG, 0, ENABLE_PANIC_RC},
	{"fast-rc-idc",	HAS_ARG, 0, FAST_RC_IDC},
	{"cpb-user-size",	HAS_ARG, 0, CPB_USER_SIZE},
	{"panic-div",	HAS_ARG,	0,	PANIC_DIV},
	{"pic-size-control",	HAS_ARG,	0,PIC_SIZE_CONTROL},

	//H.264 syntax options
	{"au-type",		HAS_ARG,	0,	SPECIFY_AU_TYPE},

	//MJPEG encode configurations
	{"quality",	HAS_ARG,	0,	'q'}, //quality factor

	//common encode configurations
	{"hflip",		HAS_ARG,	0,	SPECIFY_HFLIP}, //horizontal flip
	{"vflip",		HAS_ARG,	0,	SPECIFY_VFLIP}, //vertical flip
	{"rotate",		HAS_ARG,	0,	SPECIFY_ROTATE}, //clockwise rotate
	{"monochrome",	HAS_ARG,	0,	SPECIFY_CHROME_FORMAT}, // chrome format

	//vin configurations
	VIN_LONG_OPTIONS()

	//preview
	{"psize",		HAS_ARG,	0,	'p'},		//preview A size
	{"psize2",	HAS_ARG,	0,	'P'},		//preview B size
	{"nopreview",	NO_ARG,		0,	NO_PREVIEW},

	//system state
	{"idle",		NO_ARG,		0,	SYSTEM_IDLE},			//put system to IDLE  (turn off all encoding )
	{"mctf-enable",	HAS_ARG,	0,	SPECIFY_MCTF_POSSIBLE},

	//Vout
	VOUT_LONG_OPTIONS()

	//IDSP related
	{"dump-idsp-cfg",	HAS_ARG,		0,	DUMP_IDSP_CONFIG},

	//source buffer param
	{"main-buffer",	NO_ARG,	0,	'X'},	//main source buffer
	{"second-buffer",	NO_ARG,	0,	'Y'},
	{"third-buffer",	NO_ARG,	0,	'J'},	//
	{"fourth-buffer",	NO_ARG,	0,	'K'},	//
	{"btype",			HAS_ARG,	0,	SPECIFY_BUFFER_TYPE},
	{"bsize",			HAS_ARG,	0,	SPECIFY_BUFFER_SIZE},
	{"bmaxsize",		HAS_ARG,	0,	SPECIFY_BUFFER_MAX_SIZE},
	{"binputsize",		HAS_ARG,	0,	SPECIFY_BUFFER_INPUT_SIZE},
	{"deintlc",			HAS_ARG,	0,	SPECIFY_BUFFER_DEINTLACE},
	{"intlc-scan",		HAS_ARG, 	0,	SPECIFY_BUFFER_INTERLACE},

	//OSD blending
	{"osd-mixer",		HAS_ARG,	0,	SPECIFY_OSD_MIXER},

	//show info options
	{"show-system-state",	NO_ARG,	0,	SHOW_SYSTEM_STATE},		//show system state
	{"show-encode-config",	NO_ARG,	0,	SHOW_ENCODE_CONFIG},
	{"show-stream-info",	NO_ARG,	0,	SHOW_STREAM_INFO},
	{"show-qp-hist-info",	NO_ARG,	0,	SHOW_QP_HIST_INFO},
	{"show-buffer-info",	NO_ARG,	0,	SHOW_BUFFER_INFO},
	{"show-resource-info",	NO_ARG,	0,	SHOW_RESOURCE_INFO},
	{"show-driver-info",	NO_ARG,	0,	SHOW_DRIVER_INFO},
	{"show-chip-info",		NO_ARG, 0,	SHOW_CHIP_INFO},
	{"show-all-info",		NO_ARG, 0,	SHOW_ALL_INFO},

	{0, 0, 0, 0}
};

static const char *short_options = "ABb:Cc:Def:h:JKm:nsM:N:q:i:S:p:P:v:V:XY";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tconfig for stream A"},
	{"", "\t\tconfig for stream B"},
	{"", "\t\tconfig for stream C"},
	{"", "\t\tconfig for stream D\n"},

	{"resolution", "\tenter H.264 encoding resolution"},
	{"resolution", "\tenter MJPEG encoding resolution"},
	{"", "\t\tset stream encode type to NONE"},
	{"0~3", "\tsource buffer 0~3" },
	{"0~n", "\tcut out encoding offset x from source buffer"},
	{"0~n", "\tcut out encoding offset y from source buffer"},
	{"resolution", "specify stream max size for system resouce limit\n"},

	//immediate action, configure encode stream on the fly
	{"", "\t\tstart encoding for current stream"},
	{"", "\t\tstop encoding for current stream"},
	{"", "\t\tforce IDR at once for current stream"},
	{"", "\t\trestart encoding for current stream"},
	{"1~255/1~255", "change frame rate interval for current stream, numerator/denominator"},
	{"1~255/1~255", "Simutaneously change frame rate interval for current streams during encoding, numerator/denominator"},
	{"1~255", "set preview A framerate divisor, to match with DVOUT framerate"},

	//H.264 encode configurations
	{"1~8", "\t\tH.264 GOP parameter M"},
	{"1~255", "\t\tH.264 GOP parameter N, must be multiple of M, can be changed during encoding"},
	{"1~128", "\thow many GOP's, an IDR picture should happen, can be changed during encoding"},
	{"0|1|6|7", "\tGOP model, 0 for simple GOP, 1 for advanced GOP, 6 for unchained GOP model, 7 for hierarchical GOP model"},
	{"cbr|vbr|cbr-quality|vbr-quality|cbr2", "\tbitrate control method"},
	{"value", "\tset cbr average bitrate, can be changed during encoding"},
	{"min~max", "set vbr bitrate range, can be changed during encoding"},
	{"0~51", "\tset I-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~51", "\tset P-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~51", "\tset B-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~4", "\tset strength of adaptive qp"},
	{"1~10", "\tset diff of I QP less than P QP"},
	{"1~5", "\tset diff of P QP less than B QP"},
	{"0|1|2", "0: disable, 1: skip based on CPB size, 2: skip based on target bitrate and max QP"},
	{"0~n", "set intra refresh MB rows number, default value is 0, which means disable"},
	{"-50~50", "set QP Matrix delta value for I frame in format of 'd0,d1,d2,d3'"},
	{"-50~50", "set QP Matrix delta value for P frame in format of 'd0,d1,d2,d3'"},
	{"-50~50", "set QP Matrix delta value for B frame in format of 'd0,d1,d2,d3'"},
	{"0~5", "\tset QP Matrix mode. QP Matrix is set in 0: none; 1: left; 2: right; 3: top; 4: bottom;"
		" 5: bottom and divided into 3 regions.\n"},

	{"-6~6", "deblocking-alpha"},
	{"-6~6", "deblocking-beta"},
	{"0|1|2","deblocking-enable, 2 is auto and default value"},
	{"1|0", "\t1:Baseline profile(CAVLC), 0:Main profile(CABAC), default is 0"},
	{"value", "\tset max bitrate for CAVLC, default is 1500000 bps (CANNOT be larger than 2000000 bps)"},
	{"1|0", "\t1:High profile(CABAC), 0:Main profile(CABAC), default is 0"},
	{"1|2|3", "\tmax GOP M for stream A, needs to be 3 for low delay mode, default is 1"},
	{"1|0", "\tset read encode info protocol, 0:polling mode, 1:coded bits interrupt mode. Default is 0"},
	{"1|0", "\tset pip size enable flag, 0:smallest width is 320, 1:support width smaller than 320. Default is 0.\n"
		"\t\t\t\tEnable this flag will decrease encode performance for 3MP from 20 fps to 15 fps."},
	{"value", "\tset audio clock frequency from 8000 Hz to 48000 Hz, default is 48000 Hz."},

	{"0~30", "set low delay MB number of a frame"},

	{"0|1", "\tInterlaced Encoding 0:default, 1: force two fields be I-picture\n"},

	{"0|1", "\tenable lowdelay cap, can only work for GOP config (M=1) \n"},
	{"0|1", "Disable over-sampling function, default is enabled\n"},
	{"0|1", "\tEnable hd sdi mode (low delay preview), default is disabled\n"},
	{"1~4000", "set intrabias for P frames of current stream"},
	{"1~4000", "set intrabias for B frames of current stream"},
	{"0|1", "\tEnable additional idr when scence change detect, default is disable \n"},

	{"-64~64", "A larger positive value will tune mode decision away from this mode "\
			"\t\t\t\t\twhile a smaller value will tune mode decision towards this mode \n"},
	{"-64~64", "A larger positive value will tune mode decision away from this mode" \
			"\t\t\t\t\twhile a smaller value will tune mode decision towards this mode \n"},
	{"-64~64", "A larger positive value will tune mode decision away from this mode" \
			"\t\t\t\t\twhile a smaller value will tune mode decision towards this mode \n"},
	{"-64~64", "A larger positive value will tune mode decision away from this mode" \
			"\t\t\t\t\twhile a smaller value will tune mode decision towards this mode \n"},
	{"-64~64", "A larger positive value will tune mode decision away from this mode" \
			"\t\t\t\t\twhile a smaller value will tune mode decision towards this mode \n"},
	{"-64~64", "A larger positive value will tune mode decision away from this mode" \
			"\t\t\t\t\twhile a smaller value will tune mode decision towards this mode \n"},
	{"0~51",	"\tA larger value for this parameter will help increase coding of SKIP blocks."\
			"\t\t\t\t\tThis can improve the coding efficiency in low bit rate and/or high noise condition. \n"},

	{"0~31", "\tspecify cpb buffer size. 0: using buffer size based on the encoding size," \
			"\n\t\t\t\t31: using user-defined cbp size set in cpb_user_size field" \
			"\n\t\t\t\tothers: cpb size is the in seconds multiplied by the bitrate"},
	{"0|1", "\tenable the rate control"},
	{"0|1", "\t0: disable panic mode;\t1: enable panic mode"},
	{"0|2", "\tfast rate control reaction time. \t0: no fast RC; 2: use fast RC"},
	{"value", "user defined cpb buffer size. It will be used only when cpb_buf_idc is 0x1F"\
			"\n\t\t\t\tset it as 65535(0xFFFF) at infinite GOP mode when panic mode is enabled and pic_size_control is non-zero"},
	{"0~n", " \ttunes for the number of mb rows to do before comparing with the panic threshold values,"\
			"\n\t\t\t\tthe lower the value, the faster the intial check is."},
	{"value", "a number that can control the thresholds of when to panic, a typical value is 1 Megabit"\
			"\n\t\t\t\tthe lower the value, the more likely panic will happen."\
			"\n\t\t\t\tpic_control_size and panic_div plus the average_bitrate param need to be tuned carefully to control pic size.\n"},

	//H.264 syntax options
	{"0~3", "\t0: No AUD, No SEI; 1: AUD before SPS, PPS, with SEI; 2: AUD after SPS, PPS, with SEI; 3: No AUD, with SEI.\n"},

	//MJPEG encode configurations
	{"quality", "\tset jpeg/mjpeg encode quality"},

	//common encode configurations
	{"0|1", "\tdsp horizontal flip for current stream"},
	{"0|1", "\tdsp vertical flip for current stream"},
	{"0|1", "\tdsp clockwise rotate for current stream"},
	{"0|1", "\tset chrome format, 0 for YUV420 format, 1 for monochrome\n"},

	//vin configurations
	VIN_PARAMETER_HINTS()

	//preview
	{"resolution", "\tset preview A resolution"},
	{"resolution", "set preview B resolution"},
	{"", "\t\tdo not enter preview"},

	//system state
	{"", "\t\tput system to IDLE  (turn off all encoding)"},
	{"0|1", "\tEnable MCTF possible and DZ Type I on Non-run time\n"},

	//VOUT
	VOUT_PARAMETER_HINTS()

	//IDSP related
	{"0~7", "dump iDSP config for debug purpose, default section id is 0\n"},

	//source buffer param
	{"", "\tconfig for Main source buffer"},
	{"", "\tconfig for Second source buffer"},
	{"", "\tconfig for Third source buffer"},
	{"", "\tconfig for Fourth source buffer"},
	{"enc|prev", "\tspecify source buffer type"},
	{"resolution", "\tspecify source buffer resolution, set 0x0 to disable it"},
	{"resolution", "specify source buffer max resolution, set 0x0 to cleanly disable it"},
	{"resolution", "specify source buffer input window size, so as to crop before downscale"},
	{"0|1|2", "\tInterlaced Encoding 0:field encoding 1:deinterlacing(BOB) 2: deinterlacing(WEAVE)"},
	{"0|1",	  "\tInterlace Scan 0:OFF  1: progress to interlace "},

	//OSD blending
	{"off|a|b", "OSD blending mixer, off: disable, a: select from VOUTA, b: select from VOUTB\n"},

	//show info options
	{"", "\tShow system state"},
	{"", "\tshow stream(H.264/MJPEG) encode config"},
	{"", "\tShow stream format, size, info & state"},
	{"", "\tShow stream QP histogram info"},
	{"", "\tShow source buffer info & state"},
	{"", "\tShow codec resource limit info"},
	{"", "\tShow IAV driver info"},
	{"", "\tShow chip info"},
	{"", "\tShow all info \n"},

};

#ifdef CHECK_DELAY

#include "amba_debug.h"
char *debug_mem;
struct amba_debug_mem_info debug_mem_info;

void init_timer(void)
{
	static int init = 0;
	int fd;

	if (init)
		return;

	fd = open("/dev/ambad", O_RDWR, 0);
	if (fd < 0) {
		perror("/dev/ambad");
		return;
	}

	if (ioctl(fd, AMBA_DEBUG_IOC_GET_MEM_INFO, &debug_mem_info) < 0) {
		perror("AMBA_DEBUG_IOC_GET_MEM_INFO");
		return;
	}

	debug_mem = (char *)mmap(NULL, (debug_mem_info.ahb_size +
		debug_mem_info.apb_size + debug_mem_info.ddr_size),
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (debug_mem == MAP_FAILED) {
		perror("mmap");
		return;
	}
}

u32 read_counter(void)
{
	return *(u32 *)((0x7000b010 - debug_mem_info.apb_start +
		debug_mem_info.ahb_size) + debug_mem);
}
void print_delay(bs_fifo_info_t *fifo_info)
{
	static u32 frames = 0;
	static u32 max = 0;
	static u32 maxmax = 0;

	u32 counter = read_counter();
	u32 delta = fifo_info->rt_counter - counter;

	if (delta > max)
		max = delta;

	frames += fifo_info->count;
	if ((frames % 30) == 0) {
		if (max > maxmax)
			maxmax = max;
		printf("=== delay = %d, max = %d\n", max, maxmax);
		max = 0;
	}
}

#endif


int map_buffer(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	AM_IOCTL(fd_iav, IAV_IOC_MAP_BSB, &info);
	bsb_mem = info.addr;
	bsb_size = info.length;
	//printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);

	AM_IOCTL(fd_iav, IAV_IOC_MAP_DSP, &info);
	mem_mapped = 1;
	return 0;
}

int get_arbitrary_resolution(const char *name, int *width, int *height)
{
	sscanf(name, "%dx%d", width, height);
	return 0;
}

int get_encode_resolution(const char *name, int *width, int *height)
{
	int i;

	for (i = 0; i < sizeof(__encode_res) / sizeof(__encode_res[0]); i++)
		if (strcmp(__encode_res[i].name, name) == 0) {
			*width = __encode_res[i].width;
			*height = __encode_res[i].height;
			printf("%s resolution is %dx%d\n", name, *width, *height);
			return 0;
		}
	get_arbitrary_resolution(name, width, height);
	printf("resolution %dx%d\n", *width, *height);
	return 0;
}

//first second value must in format "x~y" if delimiter is '~'
static int get_two_unsigned_int(const char *name, u32 *first, u32 *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("range should be like a%cb \n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1,  name + strlen(name) -separator);
	*second = atoi(tmp_string);

//	printf("input string %s,  first value %d, second value %d \n",name, *first, *second);
	return 0;
}

static int get_multi_s8_arg(char * optarg, s8 *argarr, int argcnt)
{
	int i;
	char *delim = ",:";
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

static const char *get_state_str(int state)
{
	switch (state) {
	case IAV_STATE_IDLE: return "idle";
	case IAV_STATE_PREVIEW:	return "preview";
	case IAV_STATE_ENCODING: return "encoding";
	case IAV_STATE_STILL_CAPTURE: return "still capture";
	case IAV_STATE_DECODING: return "decoding";
	case IAV_STATE_INIT: return "init";
	default: return "???";
	}
}

static const char *get_dsp_op_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "encode mode";
	case 1:	return "decode mode";
	case 2:	return "reset mode";
	default: return "???";
	}
}

static const char *get_dsp_encode_state_str(int state)
{
	switch (state) {
	case 0:	return "idle";
	case 1:	return "busy";
	case 2:	return "pause";
	case 3:	return "flush";
	case 4:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_encode_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "stop";
	case 1:	return "video";
	case 2:	return "sjpeg";
	case 3:	return "mjpeg";
	case 4:	return "fast3a";
	case 5:	return "rjpeg";
	case 6:	return "timer";
	case 7:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_decode_state_str(int state)
{
	switch (state) {
	case 0:	return "idle";
	case 1:	return "h264dec";
	case 2:	return "h264dec idle";
	case 3:	return "transit h264dec to idle";
	case 4:	return "transit h264dec to h264dec idle";
	case 5:	return "jpeg still";
	case 6:	return "transit jpeg still to idle";
	case 7:	return "multiscene";
	case 8:	return "transit multiscene to idle";
	case 9:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_decode_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "stopped";
	case 1:	return "idle";
	case 2:	return "jpeg";
	case 3:	return "h.264";
	case 4:	return "multi";
	default: return "???";
	}
}

int get_bitrate_control(const char *name)
{
	if (strcmp(name, "cbr") == 0)
		return IAV_CBR;
	else if (strcmp(name, "vbr") == 0)
		return IAV_VBR;
	else if (strcmp(name, "cbr-quality") == 0)
		return IAV_CBR_QUALITY_KEEPING;
	else if (strcmp(name, "vbr-quality") == 0)
		return IAV_VBR_QUALITY_KEEPING;
	else if (strcmp(name, "cbr2") == 0)
		return IAV_CBR2;
	else
		return -1;
}

int get_chrome_format(const char *format, int encode_type)
{
	int chrome = atoi(format);
	if (chrome == 0) {
		return (encode_type == IAV_ENCODE_H264) ?
			IAV_H264_CHROMA_FORMAT_YUV420 :
			IAV_JPEG_CHROMA_FORMAT_YUV420;
	} else if (chrome == 1) {
		return (encode_type == IAV_ENCODE_H264) ?
			IAV_H264_CHROMA_FORMAT_MONO :
			IAV_JPEG_CHROMA_FORMAT_MONO;
	} else {
		printf("invalid chrome format : %d.\n", chrome);
		return -1;
	}
}

int get_buffer_type(const char *name)
{
	if (strcmp(name, "enc") == 0)
		return IAV_SOURCE_BUFFER_TYPE_ENCODE;
	if (strcmp(name, "prev") == 0)
		return IAV_SOURCE_BUFFER_TYPE_PREVIEW;
	if (strcmp(name, "off") == 0)
		return IAV_SOURCE_BUFFER_TYPE_OFF;

	printf("invalid buffer type: %s\n", name);
	return -1;
}

int get_osd_mixer_selection(const char *name)
{
	if (strcmp(name, "off") == 0)
		return OSD_BLENDING_OFF;
	if (strcmp(name, "a") == 0)
		return OSD_BLENDING_FROM_MIXER_A;
	if (strcmp(name, "b") == 0)
		return OSD_BLENDING_FROM_MIXER_B;

	printf("invalid osd mixer selection: %s\n", name);
	return -1;
}

int check_encode_profile(int stream)
{
	if (encode_param[stream].h264_param.h264_entropy_codec &&
		encode_param[stream].h264_param.h264_high_profile) {
		printf("CANNOT set 'CAVLC' and 'high-profile(CABAC)' simultaneously! Please check usage.\n");
		return -1;
	}
	return 0;
}

static inline int check_intrabias(u32 intrabias)
{
	if (intrabias > INTRABIAS_MAX || intrabias < INTRABIAS_MIN) {
		printf("Invalid intrabias value [%d], it must be in [%d~%d.\n",
			intrabias, INTRABIAS_MAX, INTRABIAS_MIN);
		return -1;
	}
	return 0;
}

void usage(void)
{
	int i;

	printf("test_encode usage:\n");
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

	printf("vin mode:  ");
	for (i = 0; i < sizeof(__vin_modes)/sizeof(__vin_modes[0]); i++) {
		if (__vin_modes[i].name[0] == '\0') {
			printf("\n");
			printf("           ");
		} else
			printf("%s  ", __vin_modes[i].name);
	}
	printf("\n");

	printf("vout mode:  ");
	for (i = 0; i < sizeof(vout_res) / sizeof(vout_res[0]); i++)
		printf("%s  ", vout_res[i].name);
	printf("\n");

	printf("resolution:  ");
	for (i = 0; i < sizeof(__encode_res)/sizeof(__encode_res[0]); i++) {
		if (__encode_res[i].name[0] == '\0') {
			printf("\n");
			printf("             ");
		} else
			printf("%s  ", __encode_res[i].name);
	}
	printf("\n");
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int width, height;
	u32 min_value, max_value;
	u32 numerator, denominator;

	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {

		// handle all other options
		switch (ch) {
		case 'A':
			current_stream = 0;
			break;
		case 'B':
			current_stream = 1;
			break;
		case 'C':
			current_stream = 2;
			break;
		case 'D':
			current_stream = 3;
			break;

		case 'h':
				VERIFY_STREAMID(current_stream);
				if (get_encode_resolution(optarg, &width, &height) < 0)
					return -1;
				encode_format[current_stream].type = IAV_ENCODE_H264;
				encode_format[current_stream].type_changed_flag = 1;

				encode_format[current_stream].width = width;
				encode_format[current_stream].height = height;
				encode_format[current_stream].resolution_changed_flag = 1;

				encode_format_changed_id |= (1 << current_stream);
				break;

		case 'm':
				VERIFY_STREAMID(current_stream);
				if (get_encode_resolution(optarg, &width, &height) < 0)
					return -1;
				encode_format[current_stream].type = IAV_ENCODE_MJPEG;
				encode_format[current_stream].type_changed_flag = 1;

				encode_format[current_stream].width = width;
				encode_format[current_stream].height = height;
				encode_format[current_stream].resolution_changed_flag = 1;

				encode_format_changed_id |= (1 << current_stream);
				break;

		case 'n':
				VERIFY_STREAMID(current_stream);
				encode_format[current_stream].type = IAV_ENCODE_NONE;
				encode_format[current_stream].type_changed_flag = 1;

				encode_format_changed_id |= (1 << current_stream);
				break;

		case 'b':
				VERIFY_STREAMID(current_stream);
				encode_format[current_stream].source = atoi(optarg);
				encode_format[current_stream].source_changed_flag = 1;

				encode_format_changed_id |= (1 << current_stream);
				break;

		case ENCODING_OFFSET_X:
				VERIFY_STREAMID(current_stream);
				encode_format[current_stream].offset_x = atoi(optarg);
				encode_format[current_stream].offset_changed_flag = 1;
				encode_format_changed_id |= (1 << current_stream);
				break;

		case ENCODING_OFFSET_Y:
				VERIFY_STREAMID(current_stream);
				encode_format[current_stream].offset_y = atoi(optarg);
				encode_format[current_stream].offset_changed_flag = 1;
				encode_format_changed_id |= (1 << current_stream);
				break;

		case ENCODING_MAX_SIZE:
				VERIFY_STREAMID(current_stream);
				if (get_encode_resolution(optarg, &width, &height) < 0)
					return -1;
				stream_max_size[current_stream].width = width;
				stream_max_size[current_stream].height = height;
				stream_max_size_changed_id |= (1 << current_stream);
				system_resource_limit_changed_flag = 1;
				break;

		case 'e':
				VERIFY_STREAMID(current_stream);
				start_stream_id |= (1 << current_stream);
				break;

		case 's':
				VERIFY_STREAMID(current_stream);
				stop_stream_id |= (1 << current_stream);
				break;

		case SPECIFY_FORCE_IDR:
				VERIFY_STREAMID(current_stream);
				//force idr
				force_idr_id |= (1 << current_stream);
				break;

		case ENCODING_RESTART:
				VERIFY_STREAMID(current_stream);
				//restart encoding for stream
				printf("restart.......... \n");
				restart_stream_id |= (1 << current_stream);
				break;

		case FRAME_FACTOR:
				VERIFY_STREAMID(current_stream);
				//change frame rate on the fly
				if (get_two_unsigned_int(optarg, &numerator, &denominator, '/') < 0) {
					return -1;
				}
				framerate_factor_change_id |= (1 << current_stream);
				framerate_factor_change[current_stream][0] = numerator;
				framerate_factor_change[current_stream][1] = denominator;
				break;

		case FRAME_FACTOR_SYNC:
			VERIFY_STREAMID(current_stream);
			//change frame rate on the fly
			if (get_two_unsigned_int(optarg, &numerator, &denominator, '/')
			        < 0) {
				return -1;
			}
			framerate_factor_sync_id |= (1 << current_stream);
			framerate_factor_change[current_stream][0] = numerator;
			framerate_factor_change[current_stream][1] = denominator;
			break;

		case PREV_A_FRAMERATE_DIV:
				prev_a_framerate_div = atoi(optarg);
				if (prev_a_framerate_div == 0)	// divisor cannot be zero
					return -1;
				prev_a_framerate_div_flag = 1;
				break;

		case CHANGE_QP_LIMIT_I:
				VERIFY_STREAMID(current_stream);
				if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
					return -1;
				}
				qp_limit_changed_id |= (1 << current_stream);
				qp_limit_param[current_stream].qp_min_i = min_value;
				qp_limit_param[current_stream].qp_max_i = max_value;
				qp_limit_param[current_stream].qp_i_flag = 1;
				break;

		case CHANGE_QP_LIMIT_P:
				VERIFY_STREAMID(current_stream);
				if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
					return -1;
				}
				qp_limit_changed_id |= (1 << current_stream);
				qp_limit_param[current_stream].qp_min_p = min_value;
				qp_limit_param[current_stream].qp_max_p = max_value;
				qp_limit_param[current_stream].qp_p_flag = 1;
				break;

		case CHANGE_QP_LIMIT_B:
				VERIFY_STREAMID(current_stream);
				if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
					return -1;
				}
				qp_limit_changed_id |= (1 << current_stream);
				qp_limit_param[current_stream].qp_min_b = min_value;
				qp_limit_param[current_stream].qp_max_b = max_value;
				qp_limit_param[current_stream].qp_b_flag = 1;
				break;

		case CHANGE_ADAPT_QP:
				VERIFY_STREAMID(current_stream);
				qp_limit_changed_id |= (1 << current_stream);
				qp_limit_param[current_stream].adapt_qp = atoi(optarg);
				qp_limit_param[current_stream].adapt_qp_flag = 1;
				break;

		case CHANGE_I_QP_REDUCE:
				VERIFY_STREAMID(current_stream);
				qp_limit_changed_id |= (1 << current_stream);
				qp_limit_param[current_stream].i_qp_reduce = atoi(optarg);
				qp_limit_param[current_stream].i_qp_reduce_flag = 1;
				break;

		case CHANGE_P_QP_REDUCE:
				VERIFY_STREAMID(current_stream);
				qp_limit_changed_id |= (1 << current_stream);
				qp_limit_param[current_stream].p_qp_reduce = atoi(optarg);
				qp_limit_param[current_stream].p_qp_reduce_flag = 1;
				break;

		case CHANGE_SKIP_FRAME_MODE:
				VERIFY_STREAMID(current_stream);
				qp_limit_changed_id |= (1 << current_stream);
				qp_limit_param[current_stream].skip_frame = atoi(optarg);
				qp_limit_param[current_stream].skip_frame_flag = 1;
				break;

		case CHANGE_INTRA_MB_ROWS:
				VERIFY_STREAMID(current_stream);
				intra_mb_rows[current_stream] = atoi(optarg);
				intra_mb_rows_changed_id |= (1 << current_stream);
				break;

		case CHANGE_QP_MATRIX_DELTA_I:
				VERIFY_STREAMID(current_stream);
				if (get_multi_s8_arg(optarg, qp_matrix_param[current_stream].delta[0], 4) < 0) {
					printf("need %d args for qp matrix delta array.\n", 4);
					return -1;
				}
				qp_matrix_param[current_stream].delta_flag = 1;
				qp_matrix_changed_id |= (1 << current_stream);
				break;

		case CHANGE_QP_MATRIX_DELTA_P:
				VERIFY_STREAMID(current_stream);
				if (get_multi_s8_arg(optarg, qp_matrix_param[current_stream].delta[1], 4) < 0) {
					printf("need %d args for qp matrix delta array.\n", 4);
					return -1;
				}
				qp_matrix_param[current_stream].delta_flag = 1;
				qp_matrix_changed_id |= (1 << current_stream);
				break;

		case CHANGE_QP_MATRIX_DELTA_B:
				VERIFY_STREAMID(current_stream);
				if (get_multi_s8_arg(optarg, qp_matrix_param[current_stream].delta[2], 4) < 0) {
					printf("need %d args for qp matrix delta array.\n", 4);
					return -1;
				}
				qp_matrix_param[current_stream].delta_flag = 1;
				qp_matrix_changed_id |= (1 << current_stream);
				break;

		case CHANGE_QP_MATRIX_MODE:
				VERIFY_STREAMID(current_stream);
				min_value = atoi(optarg);
				if (min_value > 5) {
					printf("Invalid QP Matrix mode [%d], please choose from 0~5.\n", min_value);
					return -1;
				}
				qp_matrix_param[current_stream].matrix_mode = min_value;
				qp_matrix_param[current_stream].matrix_mode_flag = 1;
				qp_matrix_changed_id |= (1 << current_stream);
				printf("set qp matrix mode : %d.\n", min_value);
				break;

		case 'M':
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_M = atoi(optarg);
				encode_param[current_stream].h264_param.h264_M_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				GOP_changed_id |= (1 << current_stream);
				system_resource_limit_changed_flag = 1;
				break;

		case 'N':
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_N = atoi(optarg);
				encode_param[current_stream].h264_param.h264_N_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				GOP_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_GOP_IDR:
				//idr
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_idr_interval = atoi(optarg);
				encode_param[current_stream].h264_param.h264_idr_interval_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				GOP_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_GOP_MODEL:
				//gop
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_gop_model = atoi(optarg);
				encode_param[current_stream].h264_param.h264_gop_model_flag  = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case BITRATE_CONTROL:
				//bitrate control
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_bitrate_control = get_bitrate_control(optarg);
				encode_param[current_stream].h264_param.h264_bitrate_control_flag  = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_CBR_BITRATE:
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_cbr_avg_bitrate = atoi(optarg);
				encode_param[current_stream].h264_param.h264_cbr_bitrate_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_VBR_BITRATE:
				VERIFY_STREAMID(current_stream);
				if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
					return -1;
				}
				encode_param[current_stream].h264_param.h264_vbr_min_bitrate = min_value;
				encode_param[current_stream].h264_param.h264_vbr_max_bitrate = max_value;
				encode_param[current_stream].h264_param.h264_vbr_bitrate_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case DEBLOCKING_ALPHA:
				//deblocking alpha
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_deblocking_filter_alpha = atoi(optarg);
				encode_param[current_stream].h264_param.h264_deblocking_filter_alpha_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case DEBLOCKING_BETA:
				//deblocking beta
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_deblocking_filter_beta = atoi(optarg);
				encode_param[current_stream].h264_param.h264_deblocking_filter_beta_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case DEBLOCKING_ENABLE:
				//deblocking enable
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_deblocking_filter_enable = atoi(optarg);
				encode_param[current_stream].h264_param.h264_deblocking_filter_enable_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case CHANGE_INTRA_16X16_BIAS:
				VERIFY_STREAMID(current_stream);
				bias_param[current_stream].intra16x16_bias =  atoi(optarg);
				bias_param_changed_id |= (1 << current_stream);
				break;
		case CHANGE_INTRA_4X4_BIAS:
				VERIFY_STREAMID(current_stream);
				bias_param[current_stream].intra4x4_bias =  atoi(optarg);
				bias_param_changed_id |= (1 << current_stream);
				break;
		case CHANGE_INTER_16X16_BIAS:
				VERIFY_STREAMID(current_stream);
				bias_param[current_stream].inter16x16_bias =  atoi(optarg);
				bias_param_changed_id |= (1 << current_stream);
				break;
		case CHANGE_INTER_8X8_BIAS:
				VERIFY_STREAMID(current_stream);
				bias_param[current_stream].inter8x8_bias=  atoi(optarg);
				bias_param_changed_id |= (1 << current_stream);
				break;
		case CHANGE_DIRECT_16X16_BIAS:
				VERIFY_STREAMID(current_stream);
				bias_param[current_stream].direct16x16_bias =  atoi(optarg);
				bias_param_changed_id |= (1 << current_stream);
				break;
		case CHANGE_DIRECT_8X8_BIAS:
				VERIFY_STREAMID(current_stream);
				bias_param[current_stream].direct8x8_bias=  atoi(optarg);
				bias_param_changed_id |= (1 << current_stream);
				break;
		case CHANGE_ME_LAMBDA_QP_OFFSET:
				VERIFY_STREAMID(current_stream);
				bias_param[current_stream].me_lambda_qp_offset=  atoi(optarg);
				bias_param_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_CAVLC:
				//cabac/cavlc entroy codec
				VERIFY_STREAMID(current_stream);
				// 1 is IAV_ENTROPY_CAVLC , 0 is IAV_ENTROPY_CABAC
				encode_param[current_stream].h264_param.h264_entropy_codec = atoi(optarg);
				encode_param[current_stream].h264_param.h264_entropy_codec_flag  = 1;
				encode_param_changed_id |= (1 << current_stream);
				if (check_encode_profile(current_stream) < 0)
					return -1;
				break;

		case SPECIFY_CAVLC_MAX_BITRATE:
				cavlc_max_bitrate = atoi(optarg);
				cavlc_max_bitrate_flag = 1;
				system_resource_limit_changed_flag = 1;
				break;

		case SPECIFY_HIGH_PROFILE:
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_high_profile = atoi(optarg);
				encode_param[current_stream].h264_param.h264_high_profile_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				if (check_encode_profile(current_stream) < 0)
					return -1;
				break;

		case SPECIFY_MAX_GOP_M:
				min_value = atoi(optarg);
				if (min_value < 1 || min_value > 3) {
					printf("Invalid max gop M [%d] for stream A. Please reset it in [1~3].\n", min_value);
					return -1;
				}
				stream_max_gop_m = min_value;
				stream_max_gop_m_flag = 1;
				system_resource_limit_changed_flag = 1;
				break;

		case SPECIFY_READOUT_MODE:
				min_value = atoi(optarg);
				if (min_value > 1) {
					printf("Invalid read out protocol [%d]. Please reset it in [0|1].\n", min_value);
					return -1;
				}
				read_encode_info_mode = min_value;
				read_encode_info_mode_flag = 1;
				break;

		case SPECIFY_PIP_SIZE:
				min_value = atoi(optarg);
				if (min_value > 1) {
					printf("Invalid pip size flag [%d]. Please reset it in [0|1].\n", min_value);
					return -1;
				}
				pip_size_enable = min_value;
				pip_size_enable_flag = 1;
				break;

		case SPECIFY_AUDIO_CLK_FREQ:
				min_value = atoi(optarg);
				if (min_value < 8000 || min_value > 48000) {
					printf("Invalid audio clock frequency [%d]. Please reset it from 8K to 48K.\n", min_value);
					return -1;
				}
				audio_clk_freq = min_value;
				audio_clk_freq_flag = 1;
				break;

		case LOWDELAY_REFRESH:
				//low delay setting
				printf("ERROR: low delay not implemented \n");
				break;

		case INTLC_IFRAME:
				//interlaced iframe
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.force_intlc_tb_iframe = atoi(optarg);
				encode_param[current_stream].h264_param.force_intlc_tb_iframe_flag  = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case LOW_DELAY_CAP_ENABLE:
				//low delay cap setting
				low_delay_cap = atoi(optarg);
				low_delay_cap_flag = 1;
				break;

		case OVERSAMPLING_DISABLE:
				min_value = atoi(optarg);
				if (min_value < 0 || min_value > 1) {
					printf("Invalid value [%d] for over-sampling diabled flag [0|1]!\n",
						min_value);
				}
				oversampling_disable = min_value;
				oversampling_disable_flag = 1;
				system_resource_limit_changed_flag = 1;
				break;

		case HD_SDI_MODE:
				min_value = atoi(optarg);
				if (min_value < 0 || min_value > 1) {
					printf("Invalid value [%d] for hd sdi mode flag [0|1]!\n",
						min_value);
				}
				hd_sdi_mode = min_value;
				hd_sdi_mode_flag = 1;
				system_resource_limit_changed_flag = 1;
				break;

		case SPECIFY_INTRABIAS_P:
				VERIFY_STREAMID(current_stream);
				min_value = atoi(optarg);
				if (check_intrabias(min_value) < 0) {
					return -1;
				}
				intrabias_p_change_id |= (1 << current_stream);
				intrabias_p_change[current_stream] = min_value;
				break;

		case SPECIFY_INTRABIAS_B:
				VERIFY_STREAMID(current_stream);
				min_value = atoi(optarg);
				if (check_intrabias(min_value) < 0) {
					return -1;
				}
				intrabias_b_change_id |= (1 << current_stream);
				intrabias_b_change[current_stream] = min_value;
				break;
		case SPECIFY_SCENE_CHANGE_DETECT:
				VERIFY_STREAMID(current_stream);
				min_value = atoi(optarg);
				if (min_value < 0 || min_value > 1) {
					printf("Invalid value [%d] for scene-change-detect enabled flag [0|1]!\n",
						min_value);
					return -1;
				}
				scene_detect_id |= (1 << current_stream);
				enable_scene_detect[current_stream] = min_value;
				break;
		case CPB_BUF_IDC:
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.cpb_buf_idc = atoi(optarg);
				encode_param[current_stream].h264_param.panic_mode_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case CPB_CMP_IDC:
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.cpb_cmp_idc = atoi(optarg);
				encode_param[current_stream].h264_param.panic_mode_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case ENABLE_PANIC_RC:
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.en_panic_rc = atoi(optarg);
				encode_param[current_stream].h264_param.panic_mode_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case FAST_RC_IDC:
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.fast_rc_idc = atoi(optarg);
				encode_param[current_stream].h264_param.panic_mode_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case CPB_USER_SIZE:
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.cpb_user_size = atoi(optarg);
				encode_param[current_stream].h264_param.panic_mode_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case PANIC_DIV:
				VERIFY_STREAMID(current_stream);
				encode_param_changed_id |= (1 << current_stream);
				panic_control_param[current_stream].panic_div = atoi(optarg);
				panic_control_param_changed_id |= (1 << current_stream);
				break;

		case PIC_SIZE_CONTROL:
				VERIFY_STREAMID(current_stream);
				encode_param_changed_id |= (1 << current_stream);
				panic_control_param[current_stream].pic_size_control = atoi(optarg);
				panic_control_param_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_AU_TYPE:
				//au type in h264 syntax
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.au_type = atoi(optarg);
				encode_param[current_stream].h264_param.au_type_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case 'q':
				//mjpeg quality
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].jpeg_param.quality = atoi(optarg);
				encode_param[current_stream].jpeg_param.quality_changed_flag  = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_HFLIP:
				//horizontal flip
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_hflip = atoi(optarg);
				encode_param[current_stream].h264_param.h264_hflip_flag = 1;
				encode_param[current_stream].jpeg_param.jpeg_hflip = atoi(optarg);
				encode_param[current_stream].jpeg_param.jpeg_hflip_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_VFLIP:
				//vertical flip
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_vflip = atoi(optarg);
				encode_param[current_stream].h264_param.h264_vflip_flag = 1;
				encode_param[current_stream].jpeg_param.jpeg_vflip = atoi(optarg);
				encode_param[current_stream].jpeg_param.jpeg_vflip_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_ROTATE:
				//clockwise rotate
				VERIFY_STREAMID(current_stream);
				encode_param[current_stream].h264_param.h264_rotate = atoi(optarg);
				encode_param[current_stream].h264_param.h264_rotate_flag = 1;
				encode_param[current_stream].jpeg_param.jpeg_rotate = atoi(optarg);
				encode_param[current_stream].jpeg_param.jpeg_rotate_flag = 1;
				encode_param_changed_id |= (1 << current_stream);
				break;

		case SPECIFY_CHROME_FORMAT:
				//chrome format
				VERIFY_STREAMID(current_stream);
				if ((min_value = get_chrome_format(optarg, IAV_ENCODE_H264)) < 0)
					return -1;
				if ((max_value = get_chrome_format(optarg, IAV_ENCODE_MJPEG)) < 0)
					return -1;
				encode_param[current_stream].h264_param.h264_chrome_format = min_value;
				encode_param[current_stream].h264_param.h264_chrome_format_flag = 1;
				encode_param[current_stream].jpeg_param.jpeg_chrome_format = max_value;
				encode_param[current_stream].jpeg_param.jpeg_chrome_format_flag = 1;
				chroma_format_changed_id |= (1 << current_stream);
				break;

		//VIN
		VIN_INIT_PARAMETERS()

		case NO_PREVIEW:
				//nopreview
				nopreview_flag = 1;
				break;

		case SYSTEM_IDLE:
				//system state, go to idle
				idle_cmd = 1;
				break;

		case SPECIFY_MCTF_POSSIBLE:
				min_value = atoi(optarg);
				if (min_value > 1) {
					printf("Invalid parameter for MCTF possible option [0|1].\n");
					return -1;
				}
				MCTF_possible = min_value;
				MCTF_possible_changed_flag = 1;
				system_resource_limit_changed_flag = 1;
				break;

		//Vout
		VOUT_INIT_PARAMETERS()

		case DUMP_IDSP_CONFIG:
				width = atoi(optarg);
				if (width < 0) {
					printf("Invalid dump idsp section id : [%d].\n", width);
					return -1;
				}
				dump_idsp_section_id = width;
				dump_idsp_bin_flag = 1;
				printf("Prepare to dump IDSP section [%d] configuration.\n", width);
				break;

		//source buffer
		case 'X':
				current_buffer = 0;
				break;

		case 'Y':
				current_buffer = 1;
				break;

		case 'J':
				current_buffer = 2;
				break;

		case 'K':
				current_buffer = 3;
				break;

		case 'P':
				if (get_encode_resolution(optarg, &width, &height) < 0)
					return -1;
				preview_buffer_format[0].width = width;
				preview_buffer_format[0].height = height;
				preview_buffer_format[0].resolution_changed_flag = 1;
				preview_buffer_format_changed_flag = 1;
				break;

		case 'p':
				if (get_encode_resolution(optarg, &width, &height) < 0)
					return -1;
				preview_buffer_format[1].width = width;
				preview_buffer_format[1].height = height;
				preview_buffer_format[1].resolution_changed_flag = 1;
				preview_buffer_format_changed_flag = 1;
				break;

		case SPECIFY_BUFFER_TYPE:
				VERIFY_TYPE_CHANGEABLE_BUFFERID(current_buffer);
				int buffer_type = get_buffer_type(optarg);
				if (buffer_type < 0)
					return -1;
				source_buffer_type[current_buffer].buffer_type = buffer_type;
				source_buffer_type[current_buffer].buffer_type_changed_flag = 1;
				source_buffer_type_changed_flag = 1;
				break;

		case SPECIFY_BUFFER_SIZE:
				VERIFY_BUFFERID(current_buffer);
				if (get_encode_resolution(optarg, &width, &height) < 0)
					return -1;
				source_buffer_format[current_buffer].width = width;
				source_buffer_format[current_buffer].height = height;
				source_buffer_format[current_buffer].resolution_changed_flag = 1;
				source_buffer_format_changed_id |= (1 << current_buffer);
				break;

		case SPECIFY_BUFFER_MAX_SIZE:
				VERIFY_BUFFERID(current_buffer);
				if (get_encode_resolution(optarg, &width, &height) < 0)
					return -1;
				source_buffer_max_size[current_buffer].width = width;
				source_buffer_max_size[current_buffer].height = height;
				source_buffer_max_size_changed_id |= (1 << current_buffer);
				system_resource_limit_changed_flag = 1;
				break;

		case SPECIFY_BUFFER_INPUT_SIZE:
				VERIFY_BUFFERID(current_buffer);
				if (get_encode_resolution(optarg, &width, &height) < 0)
					return -1;
				source_buffer_format[current_buffer].input_width = width;
				source_buffer_format[current_buffer].input_height = height;
				source_buffer_format[current_buffer].input_window_changed_flag = 1;
				source_buffer_format_changed_id |= (1 << current_buffer);
				break;

		case SPECIFY_BUFFER_DEINTLACE:
				VERIFY_BUFFERID(current_buffer);
				int deintlc_option = atoi(optarg);
				source_buffer_format[current_buffer].deintlc_option = deintlc_option;
				source_buffer_format[current_buffer].deintlc_option_changed_flag = 1;
				source_buffer_format_changed_id |= (1 << current_buffer);
				break;

		case SPECIFY_BUFFER_INTERLACE:
				VERIFY_BUFFERID(current_buffer);
				int intlc_option = atoi(optarg);
				source_buffer_format[current_buffer].intlc_scan = intlc_option;
				source_buffer_format[current_buffer].intlc_scan_changed_flag = 1;
				source_buffer_format_changed_id |= (1 << current_buffer);
				break;

		case SPECIFY_OSD_MIXER:
				osd_mixer = get_osd_mixer_selection(optarg);
				if (osd_mixer < 0)
					return -1;
				osd_mixer_changed_flag = 1;
				break;

		//Show status
		case SHOW_SYSTEM_STATE:
				show_iav_state_flag = 1;
				break;

		case SHOW_ENCODE_CONFIG:
				//show h264 or mjpeg encode config
				show_encode_config_flag = 1;
				break;

		case SHOW_STREAM_INFO:
				//show encode stream info
				show_encode_stream_info_flag = 1;
				break;

		case SHOW_QP_HIST_INFO:
				//show stream QP histogram info
				show_qp_hist_info_flag = 1;
				break;

		case SHOW_BUFFER_INFO:
				//show source buffer info
				show_source_buffer_info_flag = 1;
				break;

		case SHOW_RESOURCE_INFO:
				//show resource limit info
				show_resource_limit_info_flag = 1;
				break;

		case SHOW_DRIVER_INFO:
				show_iav_driver_info_flag = 1;
				break;

		case SHOW_CHIP_INFO:
				show_chip_info_flag = 1;
				break;

		case SHOW_ALL_INFO:
				show_iav_state_flag = 1;
				show_encode_config_flag = 1;
				show_encode_stream_info_flag = 1;
				show_source_buffer_info_flag = 1;
				show_resource_limit_info_flag = 1;
				show_iav_driver_info_flag = 1;
				show_chip_info_flag = 1;
				break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

int show_encode_stream_info(void)
{
	iav_encode_stream_info_ex_t stream_info;
	iav_encode_format_ex_t  	encode_format;
	int format_configured;
	char state_str[128];
	char type_str[128];
	char source_str[128];
	int i;

	printf("\n[Encode stream info]:\n");
	for (i = 0; i < MAX_ENCODE_STREAM_NUM ; i++) {
		memset(&stream_info, 0, sizeof(stream_info));
		stream_info.id = (1 << i);

		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
		switch (stream_info.state) {
		case IAV_STREAM_STATE_UNKNOWN:
			strcpy(state_str, "unknown");
			break;
		case IAV_STREAM_STATE_ERROR:
			strcpy(state_str, "error");
			break;
		case IAV_STREAM_STATE_READY_FOR_ENCODING:
			strcpy(state_str, "ready for encoding");
			break;
		case IAV_STREAM_STATE_ENCODING:
			strcpy(state_str, "encoding");
			break;
		default:
			return -1;
		}

		memset(&encode_format, 0, sizeof(encode_format));
		encode_format.id = (1 << i);

		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &encode_format);

		switch (encode_format.encode_type) {
		case IAV_ENCODE_H264:
			strcpy(type_str, "H.264");
			format_configured = 1;
			break;
		case IAV_ENCODE_MJPEG:
			strcpy(type_str, "MJPEG");
			format_configured = 1;
			break;
		case IAV_ENCODE_NONE:
			strcpy(type_str, "None");
		default:
			format_configured = 0;
			break;
		}

		switch (encode_format.source) {
		case IAV_ENCODE_SOURCE_MAIN_BUFFER:
			strcpy(source_str, "MAIN source buffer");
			break;
		case IAV_ENCODE_SOURCE_SECOND_BUFFER:
			strcpy(source_str, "SECOND source buffer");
			break;
		case IAV_ENCODE_SOURCE_THIRD_BUFFER:
			strcpy(source_str, "THIRD source buffer");
			break;
		case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
			strcpy(source_str, "FOURTH source buffer");
			break;
		}

		COLOR_PRINT("Stream %c \n", i + 'A');
		BOLD_PRINT("\t%s\n", type_str);
		if (format_configured) {
			printf("\tstate : %s\n", state_str);
			printf("\tencode source : %s\n", source_str);
			printf("\tresolution : (%dx%d) \n", encode_format.encode_width,
				encode_format.encode_height);
			printf("\tencode offset : (%d,%d)\n", encode_format.encode_x,
				encode_format.encode_y);
		}
	}

	return 0;
}

int show_source_buffer_info(void)
{
	iav_source_buffer_type_all_ex_t 	buffer_type_all;
	iav_source_buffer_type_ex_t		buffer_type;
	iav_source_buffer_format_ex_t	buffer_format;
	iav_source_buffer_info_ex_t		buffer_info;
	iav_preview_buffer_format_all_ex_t preview_buffer_format_all;
	int preview_width = 0;
	int preview_height = 0;
	char state_str[64];
	char buffer_type_str[64];
	char deintlc_str[64];
	int i;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX, &buffer_type_all);
	AM_IOCTL(fd_iav, IAV_IOC_GET_PREVIEW_BUFFER_FORMAT_ALL_EX,
		&preview_buffer_format_all);

	printf("\n[Source buffer info]:\n");
	for (i = 0; i < MAX_SOURCE_BUFFER_NUM; i++) {
		buffer_info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_INFO_EX, &buffer_info);
		switch (buffer_info.state) {
		case IAV_SOURCE_BUFFER_STATE_UNKNOWN:
			strcpy(state_str, "unknown");
			break;
		case IAV_SOURCE_BUFFER_STATE_ERROR:
			strcpy(state_str, "error");
			break;
		case IAV_SOURCE_BUFFER_STATE_IDLE:
			strcpy(state_str, "idle");
			break;
		case IAV_SOURCE_BUFFER_STATE_BUSY:
			strcpy(state_str, "busy");
			break;
		default:
			return -1;
		}

		buffer_format.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX, &buffer_format);

		switch (i) {
		case 0:
			COLOR_PRINT0("  Main source buffer: \n");
			buffer_type = buffer_type_all.main_buffer_type;
			break;
		case 1:
			COLOR_PRINT0("  Second source buffer: \n");
			buffer_type = buffer_type_all.second_buffer_type;
			break;
		case 2:
			COLOR_PRINT0("  Third source buffer: \n");
			buffer_type = buffer_type_all.third_buffer_type;
			preview_width = preview_buffer_format_all.main_preview_width;
			preview_height = preview_buffer_format_all.main_preview_height;
			break;
		case 3:
			COLOR_PRINT0("  Fourth source buffer: \n");
			buffer_type = buffer_type_all.fourth_buffer_type;
			preview_width = preview_buffer_format_all.second_preview_width;
			preview_height = preview_buffer_format_all.second_preview_height;
			break;
		default:
			assert(0);
			break;
		}

		switch (buffer_type) {
		case IAV_SOURCE_BUFFER_TYPE_ENCODE:
			strcpy(buffer_type_str, "encode");
			break;
		case IAV_SOURCE_BUFFER_TYPE_PREVIEW:
			strcpy(buffer_type_str,"preview");
			break;
		case IAV_SOURCE_BUFFER_TYPE_OFF:
			strcpy(buffer_type_str,"off");
			break;
		default:
			sprintf(buffer_type_str,"%d", buffer_type);
			break;
		}

		switch (buffer_format.deintlc_for_intlc_vin) {
		case 0:
			strcpy(deintlc_str, "off");
			break;
		case 1:
			strcpy(deintlc_str, "BOB");
			break;
		case 2:
			strcpy(deintlc_str,"WEAVE");
			break;
		default:
			sprintf(deintlc_str,"%d", buffer_format.deintlc_for_intlc_vin);
			break;
		}

		printf("\ttype : %s \n", buffer_type_str);
		switch (buffer_type) {
		case IAV_SOURCE_BUFFER_TYPE_ENCODE:
			printf("\tencode format : %dx%d\n", buffer_format.width,
				buffer_format.height);
			printf("\tinput format : %dx%d\n",
				buffer_format.input_width, buffer_format.input_height);
			printf("\tinput offset : %dx%d\n",
				buffer_format.input_offset_x, buffer_format.input_offset_y);
			printf("\tdeintlc option : %s \n", deintlc_str);
			printf("\tstate : %s \n\n", state_str);
			break;
		case IAV_SOURCE_BUFFER_TYPE_PREVIEW:
			printf("\tpreview format : %dx%d\n\n", preview_width, preview_height);
			break;
		default:
			printf("\n");
			break;
		}
	}

	return 0;
}

int show_resource_limit_info(void)
{
	iav_system_resource_setup_ex_t  resource_limit_setup;
	iav_system_setup_info_ex_t sys_setup;
	int i;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_limit_setup);
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX, &sys_setup);

	printf("\n[System information]:\n");
	printf("  Low delay capture mode : %s\n", sys_setup.low_delay_cap_enable ?
		"Enabled" : "Disabled");
	printf("  Frame read out protocol : %s\n", sys_setup.coded_bits_interrupt_enable ?
		"Interrupt" : "Polling");
	printf("  Oversampling mode : %s\n", resource_limit_setup.oversampling_disable ?
		"disabled" : "enabled");
	printf("  HD SDI mode : %s\n", resource_limit_setup.hd_sdi_mode?
		"enabled" : "disabled");
	printf("\n[Codec resource limit info]:\n");
	printf("  max size for main source buffer : %dx%d\n",
		resource_limit_setup.main_source_buffer_max_width,
		resource_limit_setup.main_source_buffer_max_height);
	printf("  max size for second source buffer : %dx%d\n",
		resource_limit_setup.second_source_buffer_max_width,
		resource_limit_setup.second_source_buffer_max_height);
	printf("  max size for third source buffer : %dx%d\n",
		resource_limit_setup.third_source_buffer_max_width,
		resource_limit_setup.third_source_buffer_max_height);
	printf("  max size for fourth source buffer : %dx%d\n",
		resource_limit_setup.fourth_source_buffer_max_width,
		resource_limit_setup.fourth_source_buffer_max_height);

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		printf("  max size for stream %c : %dx%d\n", 'A' + i,
			resource_limit_setup.stream_max_encode_size[i].width,
			resource_limit_setup.stream_max_encode_size[i].height);
	}

	printf("  max GOP M for streams : %d, %d, %d, %d\n",
		resource_limit_setup.stream_max_GOP_M[0],
		resource_limit_setup.stream_max_GOP_M[1],
		resource_limit_setup.stream_max_GOP_M[2],
		resource_limit_setup.stream_max_GOP_M[3]);

	printf("  max GOP N for streams : %d, %d, %d, %d\n",
		resource_limit_setup.stream_max_GOP_N[0],
		resource_limit_setup.stream_max_GOP_N[1],
		resource_limit_setup.stream_max_GOP_N[2],
		resource_limit_setup.stream_max_GOP_N[3]);

	printf("  max number of encode streams : %d\n",
		resource_limit_setup.max_num_encode_streams);
	printf("  max number of capture sources : %d\n",
		resource_limit_setup.max_num_cap_sources);
	printf("  max bitrate of CAVLC encoding : %d\n",
		resource_limit_setup.cavlc_max_bitrate);
	printf("  total dram memory size (G bit) : %d\n",
		resource_limit_setup.total_memory_size);
	printf("  MCTF possible : %d\n", resource_limit_setup.MCTF_possible);

	return 0;
}

int show_h264_encode_config(int stream_id , iav_encode_format_ex_t * format )
{
	iav_bitrate_info_ex_t bitrate_info;
	iav_h264_config_ex_t config;
	char tmp[32];

	memset(&bitrate_info, 0, sizeof(bitrate_info));
	memset(&config, 0, sizeof(config));
	memset(tmp, 0, sizeof(tmp));

	config.id = (1 << stream_id);
	bitrate_info.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &config);
	AM_IOCTL(fd_iav, IAV_IOC_GET_BITRATE_EX, &bitrate_info);

	if (config.high_profile) {
		strcpy(tmp, "High");
	} else if (config.entropy_codec == IAV_ENTROPY_CABAC) {
		strcpy(tmp, "Main");
	} else {
		strcpy(tmp, "Baseline");
	}
	BOLD_PRINT0("\tH.264\n");
	printf("\t       profile = %s\n", tmp);
	printf("\t             M = %d\n", config.M);
	printf("\t             N = %d\n", config.N);
	printf("\t  idr interval = %d\n", config.idr_interval);
	printf("\t     gop model = %s\n", (config.gop_model == 0)? "simple":"advanced");
	printf("\t       bitrate = %d bps\n", config.average_bitrate);
	printf("\t         hflip = %d\n", config.hflip);
	printf("\t         vflip = %d\n", config.vflip);
	printf("\t        rotate = %d\n", config.rotate_clockwise);
	printf("\t chrome format = %s\n", (config.chroma_format == IAV_H264_CHROMA_FORMAT_YUV420)?"YUV420":"MONO");

	// four kinds of bitrate control method
	switch (bitrate_info.rate_control_mode) {
	case IAV_CBR:
		strcpy(tmp, "cbr");
		break;
	case IAV_VBR:
		strcpy(tmp, "vbr");
		break;
	case IAV_CBR_QUALITY_KEEPING:
		strcpy(tmp, "cbr-quality");
		break;
	case IAV_VBR_QUALITY_KEEPING:
		strcpy(tmp, "vbr-quality");
		break;
	case IAV_CBR2:
		strcpy(tmp, "cbr2");
		break;
	case IAV_VBR2:
		strcpy(tmp, "vbr2");
		break;
	}
	printf("\t  bitrate ctrl = %s\n", tmp);
	printf("\t          ar_x = %d\n", config.pic_info.ar_x);
	printf("\t          ar_y = %d\n", config.pic_info.ar_y);
	printf("\t    frame mode = %d\n", config.pic_info.frame_mode);
	printf("\t          rate = %d\n", config.pic_info.rate);
	printf("\t         scale = %d\n", config.pic_info.scale);

	return 0;
}

int show_mjpeg_encode_config(int stream_id , iav_encode_format_ex_t * format )
{
	iav_jpeg_config_ex_t config;
	config.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_JPEG_CONFIG_EX, &config);
	BOLD_PRINT0("\tMJPEG\n");
	printf("\tQuality factor = %d\n", config.quality);
	printf("\t         hflip = %d\n", config.hflip);
	printf("\t         vflip = %d\n", config.vflip);
	printf("\t        rotate = %d\n", config.rotate_clockwise);
	printf("\t Chroma format = %s\n",
		(config.chroma_format == IAV_JPEG_CHROMA_FORMAT_YUV420) ? "YUV420" : "MONO");

	return 0;
}

int show_encode_config(void)
{
	int i;
	iav_encode_format_ex_t  format;

	printf("\n[Encode stream config]:\n");
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		format.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);

		COLOR_PRINT(" Stream %c\n ", i + 'A');
		if (format.encode_type == IAV_ENCODE_H264) {
			show_h264_encode_config(i, &format);
		} else if (format.encode_type == IAV_ENCODE_MJPEG) {
			show_mjpeg_encode_config(i, &format);
		} else {
			printf("\tencoding not configured\n");
		}
	}
	return 0;
}

int show_state(void)
{
	iav_state_info_t info;

	AM_IOCTL(fd_iav, IAV_IOC_GET_STATE_INFO, &info);

	printf("\n[System state]:\n");
	printf("    vout_irq_count = %d\n", info.vout_irq_count);
	printf("     vin_irq_count = %d\n", info.vin_irq_count);
	printf("    vdsp_irq_count = %d\n", info.vdsp_irq_count);
	printf("             state = %d [%s]\n", info.state,
		get_state_str(info.state));
	printf("       dsp_op_mode = %d [%s]\n", info.dsp_op_mode,
		get_dsp_op_mode_str(info.dsp_op_mode));
	printf("  dsp_encode_state = %d [%s]\n", info.dsp_encode_state,
		get_dsp_encode_state_str(info.dsp_encode_state));
	printf("   dsp_encode_mode = %d [%s]\n", info.dsp_encode_mode,
		get_dsp_encode_mode_str(info.dsp_encode_mode));
	printf("  dsp_decode_state = %d [%s]\n", info.dsp_decode_state,
		get_dsp_decode_state_str(info.dsp_decode_state));
	printf("      decode_state = %d [%s]\n", info.decode_state,
		get_dsp_decode_mode_str(info.decode_state));
	printf("   encode timecode = %d\n", info.encode_timecode);
	printf("        encode pts = %d\n", info.encode_pts);

	return 0;
}

int show_driver_info(void)
{
	iav_driver_info_t iav_driver_info;

	AM_IOCTL(fd_iav, IAV_IOC_GET_DRIVER_INFO, &iav_driver_info);

	printf("\n[IAV driver info]:\n");
	printf("   IAV Driver Version : %s-%d.%d.%d (Last updated: %x)\n",
		iav_driver_info.description, iav_driver_info.major,
		iav_driver_info.minor, iav_driver_info.patch,
		iav_driver_info.mod_time);

	return 0;
}

int show_chip_info(void)
{
	iav_chip_id chip_id;
	char chip_str[128];

	AM_IOCTL(fd_iav, IAV_IOC_GET_CHIP_ID_EX, &chip_id);

	switch (chip_id) {
		case IAV_CHIP_ID_A5S_33:
		case IAV_CHIP_ID_A5S_33E:
			strcpy(chip_str, "A5s33");
			break;
		case IAV_CHIP_ID_A5S_55:
		case IAV_CHIP_ID_A5S_55E:
			strcpy(chip_str, "A5s55");
			break;
		case IAV_CHIP_ID_A5S_66:
		case IAV_CHIP_ID_A5S_66E:
			strcpy(chip_str, "A5s66");
			break;
		case IAV_CHIP_ID_A5S_88:
		case IAV_CHIP_ID_A5S_88E:
			strcpy(chip_str, "A5s88");
			break;
		case IAV_CHIP_ID_A5S_99:
			strcpy(chip_str, "A5s99");
			break;
		case IAV_CHIP_ID_A5S_70:
			strcpy(chip_str, "A5s70");
			break;
		default:
			strcpy(chip_str, "Unknown");
			break;
	}

	printf("\n[Chip version]:\n");
	printf("   CHIP Version : %s\n", chip_str);

	return 0;
}

int set_realtime_h264_enc_param(void)
{
	iav_h264_enc_param_ex_t enc_param;
	int i;
	int set_flag = 0;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {

		if ((intrabias_p_change_id & (1 << i)) || (intrabias_b_change_id & (1 << i))
			|| (scene_detect_id & (1 << i))) {

			set_flag = 0;
			enc_param.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_H264_ENC_PARAM_EX, &enc_param);
			printf("intrabias_P:%d,enable_scene_detect:%d\n", enc_param.intrabias_P,
					enc_param.enable_scene_detect);

			if (intrabias_p_change_id & (1 << i)) {
				if (enc_param.intrabias_P == intrabias_p_change[i]) {
					printf("Stream %d change intrabias-P is %d already\n",
						i, intrabias_p_change[i]);
				} else {
					enc_param.intrabias_P = intrabias_p_change[i];
					printf("Stream [%d] change intrabias-P [%d].\n",
						i, intrabias_p_change[i]);
					set_flag = 1;
				}
			}
			if (intrabias_b_change_id & (1 << i)) {
				if (enc_param.intrabias_B == intrabias_b_change[i]) {
					printf("Stream %d change intrabias-B is %d already\n",
						i, intrabias_b_change[i]);
				} else {
					enc_param.intrabias_B = intrabias_b_change[i];
					printf("Stream [%d] change intrabias-B [%d].\n",
						i, intrabias_b_change[i]);
					set_flag = 1;
				}
			}

			if (scene_detect_id & (1 << i)) {
				if (enc_param.enable_scene_detect == enable_scene_detect[i]) {
					printf("Stream %d scene change detect is %s already\n",
						i, enc_param.enable_scene_detect ? "enabled":"disabled");
				} else {
					enc_param.enable_scene_detect = enable_scene_detect[i];
					printf("Stream [%d] %s scene change detect.\n", i,
						enc_param.enable_scene_detect ?  "enable":"disable");
					set_flag = 1;
				}
			}

			if (set_flag == 1)
				AM_IOCTL(fd_iav, IAV_IOC_SET_H264_ENC_PARAM_EX, &enc_param);
		}

	}
	return 0;
}

int set_h264_encode_param(int stream)
{
	iav_bitrate_info_ex_t bitrate_info;
	iav_change_gop_ex_t change_gop;
	iav_h264_config_ex_t config;
	h264_param_t * param = & (encode_param[stream].h264_param);

	config.id =  (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &config);
	bitrate_info.id = (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_BITRATE_EX, &bitrate_info);

	if (param->h264_M_flag)
		config.M = param->h264_M;

	if (param->h264_N_flag)
		config.N = param->h264_N;

	if (param->h264_idr_interval_flag)
		config.idr_interval = param->h264_idr_interval;

	if (param->h264_gop_model_flag)
		config.gop_model = param->h264_gop_model;

	// deblocking filter params
	if (param->h264_deblocking_filter_alpha_flag)
		config.slice_alpha_c0_offset_div2 = param->h264_deblocking_filter_alpha;

	if (param->h264_deblocking_filter_beta_flag)
		config.slice_beta_offset_div2 = param->h264_deblocking_filter_beta;

	if (param->h264_deblocking_filter_enable_flag)
		config.deblocking_filter_enable = param->h264_deblocking_filter_enable;

	if (param->h264_entropy_codec_flag)
		config.entropy_codec = param ->h264_entropy_codec;

	if (param->h264_high_profile_flag)
		config.high_profile = param->h264_high_profile;

	// force first field or two fields to be I-frame setting
	if (param->force_intlc_tb_iframe_flag)
		config.force_intlc_tb_iframe = param->force_intlc_tb_iframe;

	if (param->h264_hflip_flag)
		config.hflip = param->h264_hflip;
	if (param->h264_vflip_flag)
		config.vflip = param->h264_vflip;
	if (param->h264_rotate_flag)
		config.rotate_clockwise = param->h264_rotate;

	if (param->h264_chrome_format_flag)
		config.chroma_format = param->h264_chrome_format;

	// bitrate control settings
	if (param->h264_bitrate_control_flag)
		bitrate_info.rate_control_mode = param->h264_bitrate_control;

	if (param->h264_cbr_bitrate_flag)
		bitrate_info.cbr_avg_bitrate = param->h264_cbr_avg_bitrate;

	if (param->h264_vbr_bitrate_flag) {
		bitrate_info.vbr_min_bitrate = param->h264_vbr_min_bitrate;
		bitrate_info.vbr_max_bitrate = param->h264_vbr_max_bitrate;
	}

	// panic mode settings
	if (param->panic_mode_flag) {
		config.cpb_buf_idc = param->cpb_buf_idc;
		config.cpb_cmp_idc = param->cpb_cmp_idc;
		config.en_panic_rc = param->en_panic_rc;
		config.fast_rc_idc = param->fast_rc_idc;
		config.cpb_user_size = param->cpb_user_size;
	}

	// h264 syntax settings
	if (param->au_type_flag)
		config.au_type = param->au_type;

	AM_IOCTL(fd_iav, IAV_IOC_SET_H264_CONFIG_EX, &config);

	// Following configurations can be changed during encoding
	if (param->h264_bitrate_control_flag || param->h264_cbr_bitrate_flag ||
		param->h264_vbr_bitrate_flag) {
		AM_IOCTL(fd_iav, IAV_IOC_SET_BITRATE_EX, &bitrate_info);
	}

	if (param->h264_N_flag || param->h264_idr_interval_flag) {
		change_gop.id = (1 << stream);
		change_gop.N = config.N;
		change_gop.idr_interval = config.idr_interval;
		AM_IOCTL(fd_iav, IAV_IOC_CHANGE_GOP_EX, &change_gop);
	}

	return 0;
}

int set_mjpeg_encode_param(int stream)
{
	jpeg_param_t * param = &(encode_param[stream].jpeg_param);
	iav_jpeg_config_ex_t config;
	config.id = (1 << stream);

	AM_IOCTL(fd_iav, IAV_IOC_GET_JPEG_CONFIG_EX, &config);

	if (param->quality_changed_flag)
		config.quality = param->quality;

	if (param->jpeg_hflip_flag)
		config.hflip = param->jpeg_hflip;
	if (param->jpeg_vflip_flag)
		config.vflip = param->jpeg_vflip;
	if (param->jpeg_rotate_flag)
		config.rotate_clockwise = param->jpeg_rotate;

	if (param->jpeg_chrome_format_flag)
		config.chroma_format = param->jpeg_chrome_format;

	AM_IOCTL(fd_iav, IAV_IOC_SET_JPEG_CONFIG_EX, &config);

	return 0;
}

int set_encode_param(void)
{
	int i;
	iav_encode_format_ex_t  format;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (encode_param_changed_id & (1 << i)) {
			format.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);

			if (format.encode_type == IAV_ENCODE_H264)
				set_h264_encode_param(i);
			else if (format.encode_type == IAV_ENCODE_MJPEG)
				set_mjpeg_encode_param(i);
			else
				return -1;
		}
	}
	return 0;
}

int set_encode_format(void)
{
	iav_encode_format_ex_t format;
	int i;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (encode_format_changed_id & (1 << i)) {
			format.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);
			if (encode_format[i].type_changed_flag) {
				format.encode_type = encode_format[i].type;
			}
			if (encode_format[i].resolution_changed_flag) {
				format.encode_width = encode_format[i].width;
				format.encode_height = encode_format[i].height;
			}
			if (encode_format[i].offset_changed_flag) {
				format.encode_x = encode_format[i].offset_x;
				format.encode_y = encode_format[i].offset_y;
			}
			if (encode_format[i].source_changed_flag) {
				format.source = encode_format[i].source;
			}
			format.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_SET_ENCODE_FORMAT_EX, &format);
		}
	}
	return 0;
}

int goto_idle(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_ENTER_IDLE, 0);
	printf("goto_idle done\n");
	return 0;
}

int enable_preview(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_ENABLE_PREVIEW, 0);
	printf("enable_preview done\n");
	return 0;
}

int start_encode(iav_stream_id_t  streamid)
{
	int i;
	iav_encode_stream_info_ex_t info;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &info);
		if (info.state == IAV_STREAM_STATE_ENCODING) {
			streamid &= ~(1 << i);
		}
	}
	if (streamid == 0) {
		printf("already in encoding, nothing to do \n");
		return 0;
	}
	// start encode
	AM_IOCTL(fd_iav, IAV_IOC_START_ENCODE_EX, streamid);

	printf("Start encoding for stream 0x%x successfully\n", streamid);
	return 0;
}

//this function will get encode state, if it's encoding, then stop it, otherwise, return 0 and do nothing
int stop_encode(iav_stream_id_t  streamid)
{
	int i;
	iav_state_info_t info;
	iav_encode_stream_info_ex_t stream_info;

	printf("Stop encoding for stream 0x%x \n", streamid);
	AM_IOCTL(fd_iav, IAV_IOC_GET_STATE_INFO, &info);
	if (info.state != IAV_STATE_ENCODING) {
		return 0;
	}
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
		if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
			streamid &= ~(1 << i);
		}
	}
	if (streamid == 0)
		return 0;
	AM_IOCTL(fd_iav, IAV_IOC_STOP_ENCODE_EX, streamid);

	return 0;
}

int restart_encode(iav_stream_id_t  streamid)
{
	printf("restart for stream 0x%x \n", streamid);
	//stop streams
	stop_encode(streamid);
	start_encode(streamid);
	return 0;
}

/* some actions like dump bin may need file access, but that should be put into different unit tests */
int dump_idsp_bin(void)
{
	char filename[32];
	int fd_idsp_dump = -1;
	u8 * dump_buffer = NULL;

	sprintf(filename, "idsp_dump_sec_%d.bin", dump_idsp_section_id);
	fd_idsp_dump = open(filename, O_WRONLY | O_CREAT, 0666);
	if (fd_idsp_dump < 0) {
		printf("Failed to open dump file [%s].\n", filename);
		goto error_exit;
	}

#if 1
	iav_idsp_config_info_t dump_idsp;

	dump_buffer = (u8 *)malloc(MAX_DUMP_BUFFER_SIZE);
	if (dump_buffer == NULL) {
		printf("Not enough memory to dump IDSP config!\n");
		goto error_exit;
	}
	dump_idsp.id_section = dump_idsp_section_id;
	dump_idsp.addr = dump_buffer;
	if (ioctl(fd_iav, IAV_IOC_DSP_DUMP_CFG, &dump_idsp) < 0) {
		perror("IAV_IOC_DSP_DUMP_CFG");
		goto error_exit;
	}

#else	// Same implementation as test_tuning
	int buffer_size = 0;
	dump_buffer = malloc(MAX_DUMP_BUFFER_SIZE);
	if (dump_buffer == NULL) {
		printf("Not enough memory to dump IDSP config!\n");
		goto error_exit;
	}
	buffer_size = img_dsp_dump(fd_iav, dump_idsp_section_id,
		dump_buffer, MAX_DUMP_BUFFER_SIZE);
	if (buffer_size == -1) {
		printf("Malloc too few buffer [50KB] to dump the IDSP config!\n");
		goto error_exit;
	}
#endif

	if (write(fd_idsp_dump, dump_buffer, MAX_DUMP_BUFFER_SIZE) < 0) {
		printf("Failed to write the idsp section configuration!\n");
		goto error_exit;
	}
	close(fd_idsp_dump);
	free(dump_buffer);
	printf("===[DONE] Dump IDSP section [%d] configuration!\n",
		dump_idsp_section_id);
	return 0;

error_exit:
	if (fd_idsp_dump > 0) {
		close(fd_idsp_dump);
		fd_idsp_dump = -1;
	}
	if (dump_buffer != NULL) {
		free(dump_buffer);
		dump_buffer = NULL;
	}
	return -1;
}

int change_frame_rate(void)
{
	iav_change_framerate_factor_ex_t change_framerate;
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (framerate_factor_change_id & (1 << i)) {
			change_framerate.id = (1 << i);
			if (framerate_factor_change[i][0] > framerate_factor_change[i][1]) {
				printf("Invalid frame interval value : %d/%d should be less than 1/1\n",
					framerate_factor_change[i][0],
					framerate_factor_change[i][1]);
				return -1;
			}
			change_framerate.ratio_numerator = framerate_factor_change[i][0];
			change_framerate.ratio_denominator = framerate_factor_change[i][1];
			printf("Stream [%d] change frame interval %d/%d \n", i,
				framerate_factor_change[i][0],
				framerate_factor_change[i][1]);
			AM_IOCTL(fd_iav, IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX, &change_framerate);
		}
	}
	return 0;
}

int sync_frame_rate(void)
{
	iav_sync_framerate_factor_ex_t sync_framerate;
	int i;
	memset(&sync_framerate, 0, sizeof(sync_framerate));
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (framerate_factor_sync_id & (1 << i)) {
			if (framerate_factor_change[i][0] > framerate_factor_change[i][1]) {
				printf("Invalid frame interval value : %d/%d should be less than 1/1\n",
					framerate_factor_change[i][0],
					framerate_factor_change[i][1]);
				return -1;
			}
			sync_framerate.enable[i] = 1;
			sync_framerate.framefactor[i].id = (1 << i);
			sync_framerate.framefactor[i].ratio_numerator = framerate_factor_change[i][0];
			sync_framerate.framefactor[i].ratio_denominator = framerate_factor_change[i][1];
			printf("Stream [%d] sync frame interval %d/%d \n", i,
				framerate_factor_change[i][0],
				framerate_factor_change[i][1]);
		}
	}
	AM_IOCTL(fd_iav, IAV_IOC_SYNC_FRAMERATE_FACTOR_EX, &sync_framerate);
	return 0;
}

int set_system_setup_info(void)
{
	iav_system_setup_info_ex_t system_setup_info;
	memset(&system_setup_info, 0, sizeof(system_setup_info));

	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX, &system_setup_info);
	if (osd_mixer_changed_flag) {
		if (osd_mixer == OSD_BLENDING_FROM_MIXER_A) {
			system_setup_info.voutA_osd_blend_enable = 1;
			system_setup_info.voutB_osd_blend_enable = 0;
		} else if (osd_mixer == OSD_BLENDING_FROM_MIXER_B) {
			system_setup_info.voutA_osd_blend_enable = 0;
			system_setup_info.voutB_osd_blend_enable = 1;
		} else {
			//OSD Blending disabled
			system_setup_info.voutA_osd_blend_enable = 0;
			system_setup_info.voutB_osd_blend_enable = 0;
		}
	}
	if (read_encode_info_mode_flag) {
		system_setup_info.coded_bits_interrupt_enable = read_encode_info_mode;
	}
	if (pip_size_enable_flag) {
		system_setup_info.pip_size_enable = pip_size_enable;
	}

	if (low_delay_cap_flag) {
		system_setup_info.low_delay_cap_enable = low_delay_cap;
	}

	AM_IOCTL(fd_iav, IAV_IOC_SET_SYSTEM_SETUP_INFO_EX, &system_setup_info);

	return 0;
}

int change_system_audio_clock(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_SET_AUDIO_CLK_FREQ_EX, audio_clk_freq);

	return 0;
}

int change_panic_control_param()
{
	int i;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (panic_control_param_changed_id & (1 << i)) {
			iav_panic_control_param_ex_t panic_control;
			panic_control.id = (1 << i);
			panic_control.panic_div = panic_control_param[i].panic_div;
			panic_control.pic_size_control = panic_control_param[i].pic_size_control;
			AM_IOCTL(fd_iav,IAV_IOC_SET_PANIC_CONTROL_PARAM_EX,&panic_control);
		}
	}
	return 0;
}

int set_chroma_format()
{
	int i;
	iav_encode_format_ex_t  format;
	iav_chroma_format_info_ex_t chroma_format_info;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (chroma_format_changed_id & (1 << i)) {
			format.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);
			chroma_format_info.id = (1 << i);
			if (format.encode_type == IAV_ENCODE_H264) {
				chroma_format_info.chroma_format =
					encode_param[i].h264_param.h264_chrome_format;
			} else if (format.encode_type == IAV_ENCODE_MJPEG) {
				chroma_format_info.chroma_format =
					encode_param[i].jpeg_param.jpeg_chrome_format;
			}
			AM_IOCTL(fd_iav,IAV_IOC_SET_CHROMA_FORMAT_EX,&chroma_format_info);
		}
	}
	return 0;
}

int setup_resource_limit(iav_stream_id_t  source_buffer_id, iav_stream_id_t stream_id)
{
	int i;
	iav_system_resource_setup_ex_t  resource_limit_setup;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_limit_setup);
	// set source buffer max size
	if (source_buffer_id & (1 << 0)) {
		resource_limit_setup.main_source_buffer_max_width = source_buffer_max_size[0].width;
		resource_limit_setup.main_source_buffer_max_height = source_buffer_max_size[0].height;
		resource_limit_setup.stream_max_encode_size[0].width = source_buffer_max_size[0].width;
		resource_limit_setup.stream_max_encode_size[0].height = source_buffer_max_size[0].height;
	}
	if (source_buffer_id & (1 << 1)) {
		resource_limit_setup.second_source_buffer_max_width = source_buffer_max_size[1].width;
		resource_limit_setup.second_source_buffer_max_height = source_buffer_max_size[1].height;
	}
	if (source_buffer_id & (1 << 2)) {
		resource_limit_setup.third_source_buffer_max_width = source_buffer_max_size[2].width;
		resource_limit_setup.third_source_buffer_max_height = source_buffer_max_size[2].height;
	}
	if (source_buffer_id & (1 << 3)) {
		resource_limit_setup.fourth_source_buffer_max_width = source_buffer_max_size[3].width;
		resource_limit_setup.fourth_source_buffer_max_height = source_buffer_max_size[3].height;
	}

	// set stream max size
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (stream_id & (1 << i)) {
			resource_limit_setup.stream_max_encode_size[i].width = stream_max_size[i].width;
			resource_limit_setup.stream_max_encode_size[i].height = stream_max_size[i].height;
		}
	}

	if (stream_max_gop_m_flag)
		resource_limit_setup.stream_max_GOP_M[0] = stream_max_gop_m;

	if (MCTF_possible_changed_flag)
		resource_limit_setup.MCTF_possible = MCTF_possible;

	if (cavlc_max_bitrate_flag)
		resource_limit_setup.cavlc_max_bitrate = cavlc_max_bitrate;

	if (oversampling_disable_flag)
		resource_limit_setup.oversampling_disable = oversampling_disable;

	if (hd_sdi_mode_flag)
		resource_limit_setup.hd_sdi_mode = hd_sdi_mode;

	AM_IOCTL(fd_iav, IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX, &resource_limit_setup);

	return 0;
}

int setup_resource_limit_if_necessary(void)
{
	iav_system_resource_setup_ex_t  resource_limit_setup;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_limit_setup);

	if (preview_buffer_format[0].resolution_changed_flag && preview_buffer_format[0].height > 720) {
		printf("Solution to restrict to encode only 1 stream if preview is HD.\n");
		resource_limit_setup.third_source_buffer_max_width = preview_buffer_format[0].width;
		resource_limit_setup.third_source_buffer_max_height = preview_buffer_format[0].height;
		resource_limit_setup.max_num_cap_sources = 2;
		resource_limit_setup.max_num_encode_streams = 1;
		resource_limit_setup.stream_max_GOP_M[0] = 1;
		resource_limit_setup.stream_max_GOP_N[0] = 30;

		printf("Update system resource limit by current selected preview size %d x %d, max Number of streams %d \n",
			resource_limit_setup.third_source_buffer_max_width, resource_limit_setup.third_source_buffer_max_height,
			resource_limit_setup.max_num_encode_streams);
	}
#if 0
	else {
		resource_limit_setup.third_source_buffer_max_width = 1280;
		resource_limit_setup.third_source_buffer_max_height = 720;
		resource_limit_setup.max_num_cap_sources = 3;
		resource_limit_setup.max_num_encode_streams = 4;
		resource_limit_setup.stream_max_GOP_M[0] = 3;
		resource_limit_setup.stream_max_GOP_N[0] = 255;
	}
#endif

	//update  system resource limit
	AM_IOCTL(fd_iav, IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX, &resource_limit_setup);
	return 0;
}

static inline int set_all_source_buffer_format(iav_source_buffer_format_all_ex_t * buf_format)
{
	if (ioctl(fd_iav, IAV_IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX, buf_format) < 0) {
		perror("IAV_IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX");
		printf("\nsource buffer config: main %dx%d, second %dx%d, third %dx%d, fourth %dx%d\n",
			buf_format->main_width, buf_format->main_height,
			buf_format->second_width, buf_format->second_height,
			buf_format->third_width, buf_format->third_height,
			buf_format->fourth_width, buf_format->fourth_height);
		return -1;
	}
	return 0;
}

int source_buffer_type_config()
{
	iav_source_buffer_type_all_ex_t buffer_type_all;
	//int third_buffer_type_change = 0;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX, &buffer_type_all);
	if (source_buffer_type[1].buffer_type_changed_flag) {
		buffer_type_all.second_buffer_type = source_buffer_type[1].buffer_type;
	}
	if (source_buffer_type[2].buffer_type_changed_flag) {
		//third_buffer_type_change = 1;
		buffer_type_all.third_buffer_type = source_buffer_type[2].buffer_type;
	}
	if (source_buffer_type[3].buffer_type_changed_flag) {
		buffer_type_all.fourth_buffer_type = source_buffer_type[3].buffer_type;
	}
	AM_IOCTL(fd_iav, IAV_IOC_SET_SOURCE_BUFFER_TYPE_ALL_EX, &buffer_type_all);

	return 0;
}

int preview_buffer_format_config()
{
	iav_preview_buffer_format_all_ex_t preview_buffer_format_all;

	AM_IOCTL(fd_iav, IAV_IOC_GET_PREVIEW_BUFFER_FORMAT_ALL_EX,
		&preview_buffer_format_all);
	if (preview_buffer_format[0].resolution_changed_flag) {
		preview_buffer_format_all.main_preview_width = preview_buffer_format[0].width;
		preview_buffer_format_all.main_preview_height = preview_buffer_format[0].height;
	}
	if (preview_buffer_format[1].resolution_changed_flag) {
		preview_buffer_format_all.second_preview_width = preview_buffer_format[1].width;
		preview_buffer_format_all.second_preview_height = preview_buffer_format[1].height;
	}
	AM_IOCTL(fd_iav, IAV_IOC_SET_PREVIEW_BUFFER_FORMAT_ALL_EX,
		&preview_buffer_format_all);

	return 0;
}

int source_buffer_format_config()
{
	iav_source_buffer_format_all_ex_t buf_format;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format);
	if (source_buffer_format[0].resolution_changed_flag) {
		buf_format.main_width = source_buffer_format[0].width;
		buf_format.main_height = source_buffer_format[0].height;
	}
	if (source_buffer_format[1].resolution_changed_flag) {
		buf_format.second_width = source_buffer_format[1].width;
		buf_format.second_height = source_buffer_format[1].height;
	}
	if (source_buffer_format[2].resolution_changed_flag) {
		buf_format.third_width = source_buffer_format[2].width;
		buf_format.third_height = source_buffer_format[2].height;
	}
	if (source_buffer_format[3].resolution_changed_flag) {
		buf_format.fourth_width = source_buffer_format[3].width;
		buf_format.fourth_height = source_buffer_format[3].height;
	}

	/*
	 * Generally, second, third and fourth source buffer are all downscaled
	 * from main source buffer. So their input windows are equal to the size
	 * of main source buffer. If the input window is explicitly assigned to be
	 * smaller than main source buffer, cropping is performed before downscale.
	*/
	if (source_buffer_format[1].input_window_changed_flag) {
		buf_format.second_input_width = source_buffer_format[1].input_width;
		buf_format.second_input_height = source_buffer_format[1].input_height;
	} else {
		buf_format.second_input_width = buf_format.main_width;
		buf_format.second_input_height = buf_format.main_height;
	}
	if (source_buffer_format[2].input_window_changed_flag) {
		buf_format.third_input_width = source_buffer_format[2].input_width;
		buf_format.third_input_height = source_buffer_format[2].input_height;
	} else {
		buf_format.third_input_width = buf_format.main_width;
		buf_format.third_input_height = buf_format.main_height;
	}
	if (source_buffer_format[3].input_window_changed_flag) {
		buf_format.fourth_input_width = source_buffer_format[3].input_width;
		buf_format.fourth_input_height = source_buffer_format[3].input_height;
	} else {
		buf_format.fourth_input_width = buf_format.main_width;
		buf_format.fourth_input_height = buf_format.main_height;
	}

	if (source_buffer_format[0].deintlc_option_changed_flag) {
		buf_format.main_deintlc_for_intlc_vin = source_buffer_format[0].deintlc_option;
	}
	if (source_buffer_format[1].deintlc_option_changed_flag) {
		buf_format.second_deintlc_for_intlc_vin = source_buffer_format[1].deintlc_option;
	}
	if (source_buffer_format[2].deintlc_option_changed_flag) {
		buf_format.third_deintlc_for_intlc_vin = source_buffer_format[2].deintlc_option;
	}
	if (source_buffer_format[3].deintlc_option_changed_flag) {
		buf_format.fourth_deintlc_for_intlc_vin = source_buffer_format[3].deintlc_option;
	}

	//only use main buffer intlc_scan
	if (source_buffer_format[0].intlc_scan_changed_flag) {
		buf_format.intlc_scan = source_buffer_format[0].intlc_scan;
	}

	if (set_all_source_buffer_format(&buf_format) < 0)
		return -1;

	return 0;
}

int init_default_value(void)
{
	return 0;
}

static int force_idr_insertion(iav_stream_id_t  stream_id)
{
	if (ioctl(fd_iav, IAV_IOC_FORCE_IDR_EX, stream_id)  < 0) {
		perror("IAV_IOC_FORCE_IDR_EX");
		printf("force idr for stream id 0x%x Failed \n", stream_id);
		return -1;
	} else {
		printf("force idr for stream id 0x%x OK \n", stream_id);
		return 0;
	}
}

static int change_qp_limit(int stream)
{
	qp_limit_params_t * param = &qp_limit_param[stream];
	iav_change_qp_limit_ex_t qp_limit;
	printf("change_qp_limit, for test only \n");

	qp_limit.id = (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_QP_LIMIT_EX, &qp_limit);
	if (param->qp_i_flag) {
		qp_limit.qp_min_on_I = param->qp_min_i;
		qp_limit.qp_max_on_I = param->qp_max_i;
	}
	if (param->qp_p_flag) {
		qp_limit.qp_min_on_P = param->qp_min_p;
		qp_limit.qp_max_on_P = param->qp_max_p;
	}
	if (param->qp_b_flag) {
		qp_limit.qp_min_on_B = param->qp_min_b;
		qp_limit.qp_max_on_B = param->qp_max_b;
	}
	if (param->adapt_qp_flag)
		qp_limit.adapt_qp = param->adapt_qp;
	if (param->i_qp_reduce_flag)
		qp_limit.i_qp_reduce = param->i_qp_reduce;
	if (param->p_qp_reduce_flag)
		qp_limit.p_qp_reduce = param->p_qp_reduce;
	if (param->skip_frame_flag)
		qp_limit.skip_flag = param->skip_frame;
	AM_IOCTL(fd_iav, IAV_IOC_CHANGE_QP_LIMIT_EX, &qp_limit);

	return 0;
}

static int set_qp_limit(void)
{
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (qp_limit_changed_id & (1 << i)) {
			change_qp_limit(i);
		}
	}

	return 0;
}

static int change_intra_refresh_mb_rows(void)
{
	int i;
	iav_change_intra_mb_rows_ex_t intra_rows;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (intra_mb_rows_changed_id & (1 << i)) {
			intra_rows.id = (1 << i);
			intra_rows.intra_refresh_mb_rows = intra_mb_rows[i];
			AM_IOCTL(fd_iav, IAV_IOC_CHANGE_INTRA_MB_ROWS_EX, &intra_rows);
		}
	}
	return 0;
}

static int change_prev_a_framerate(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_SET_PREV_A_FRAMERATE_DIV_EX, prev_a_framerate_div);
	return 0;
}

static int print_qp_histogram(iav_qp_hist_ex_t *hist)
{
	const int entry_num = 4;
	int i, j, index;
	int mb_sum, qp_sum, mb_entry;

	printf("====== stream [%d], PTS [%d].",
		hist->id, hist->PTS);
	printf("\n====== QP:MB \n");
	mb_sum = qp_sum = 0;
	for (i = 0; i < IAV_QP_HIST_BIN_MAX_NUM / entry_num; ++i) {
		mb_entry = 0;
		printf(" [Set %d] ", i);
		for (j = 0; j < entry_num; ++j) {
			index = i * entry_num + j;
			printf("%2d:%-4d ", hist->qp[index], hist->mb[index]);
			mb_entry += hist->mb[index];
			mb_sum += hist->mb[index];
			qp_sum += hist->qp[index] * hist->mb[index];
		}
		printf("[MBs: %d].\n", mb_entry);
	}
	printf("\n====== Total MB : %d.", mb_sum);
	printf("\n====== Average QP : %d.\n\n", qp_sum / mb_sum);
	return 0;
}

static int show_qp_histogram(void)
{
	iav_qp_hist_info_ex_t qp_hist;
	int i;

	AM_IOCTL(fd_iav, IAV_IOC_GET_QP_HIST_INFO_EX, &qp_hist);
	printf("\nQP histogram for [%d] streams.\n\n", qp_hist.total_streams);
	for (i = 0; i < qp_hist.total_streams; ++i) {
		print_qp_histogram(&qp_hist.stream_qp_hist[i]);
	}
	return 0;
}

static int set_qp_matrix_mode(int stream)
{
	u32 *addr = NULL;
	u32 width, buf_pitch, height, total;
	int i, j;
	iav_encode_format_ex_t format;

	format.id = (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);
	addr = (u32 *)(G_qp_matrix_addr + (G_qp_matrix_size / MAX_ENCODE_STREAM_NUM * stream));
	width = ROUND_UP(format.encode_width, 16) / 16;
	buf_pitch = ROUND_UP(width, 8);
	height = ROUND_UP(format.encode_height, 16) / 16;
	total =  (buf_pitch * 4) * height;	// (((width * 4) * height) + 31) & (~31);
	memset(addr, 0, total);
	printf("set_qp_matrix_mode : %d.\n", qp_matrix_param[stream].matrix_mode);
	printf("width (pitch) : %d (%d), height : %d, total : %d.\n",
		width, buf_pitch, height, total);
	switch (qp_matrix_param[stream].matrix_mode) {
	case 0:
		break;
	case 1:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width / 3; j++)
				addr[i * buf_pitch + j] = 3;
		}
		break;
	case 2:
		for (i = 0; i < height; i++) {
			for (j = width * 2 / 3; j < width; j++)
				addr[i * buf_pitch + j] = 3;
		}
		break;
	case 3:
		for (i = 0; i < height / 3; i++) {
			for (j = 0; j < width; j++)
				addr[i * buf_pitch + j] = 3;
		}
		break;
	case 4:
		for (i = height * 2 / 3; i < height; i++) {
			for (j = 0; j < width; j++)
				addr[i * buf_pitch + j] = 3;
		}
		break;
	case 5:
		for (i = height / 2; i < height; i++) {
			for (j = 0; j < width / 3; j++)
				addr[i * buf_pitch + j] = 1;
			for (j = width / 3; j < width * 2 / 3; j++)
				addr[i * buf_pitch + j] = 2;
			for (j = width * 2 / 3; j < width; j++)
				addr[i * buf_pitch + j] = 3;
		}
		break;
	default:
		break;
	}

	for (i = 0; i < height; i++) {
		printf("\n");
		for (j = 0; j < width; j++) {
			printf("%d ", addr[i * buf_pitch + j]);
		}
	}
	printf("\n");

	return 0;
}

static int change_qp_matrix(void)
{
	int i, j;
	iav_qp_roi_matrix_ex_t matrix;
	iav_mmap_info_t qp_info;

	AM_IOCTL(fd_iav, IAV_IOC_MAP_QP_ROI_MATRIX_EX, &qp_info);
	G_qp_matrix_addr = qp_info.addr;
	G_qp_matrix_size = qp_info.length;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (qp_matrix_changed_id & (1 << i)) {
			matrix.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_QP_ROI_MATRIX_EX, &matrix);

			if (qp_matrix_param[i].delta_flag) {
				for (j = 0; j < NUM_FRAME_TYPES; ++j) {
					memcpy(matrix.delta[j],
						qp_matrix_param[i].delta[j], 4 * sizeof(s8));
				}
			}
			if (qp_matrix_param[i].matrix_mode_flag) {
				matrix.enable = (qp_matrix_param[i].matrix_mode != 0);
				if (set_qp_matrix_mode(i) < 0) {
					printf("set_qp_matrix_mode for stream [%d] failed!\n", i);
					return -1;
				}
			}

			AM_IOCTL(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &matrix);
		}
	}
	return 0;
}

static int do_show_status(void)
{
	//show encode stream info
	if (show_encode_stream_info_flag) {
		if (show_encode_stream_info() < 0)
			return -1;
	}

	//show encode stream QP histogram info
	if (show_qp_hist_info_flag) {
		if (show_qp_histogram() < 0) {
			return -1;
		}
	}

	//show source buffer info
	if (show_source_buffer_info_flag) {
		if (show_source_buffer_info() < 0)
			return -1;
	}

	//show codec resource limit info
	if (show_resource_limit_info_flag) {
		if (show_resource_limit_info() < 0)
			return -1;
	}

	if (show_encode_config_flag) {
		if (show_encode_config() < 0)
			return -1;
	}

	if (show_iav_state_flag) {
		show_state();
	}

	if (show_iav_driver_info_flag) {
		show_driver_info();
	}

	if (show_chip_info_flag) {
		show_chip_info();
	}

	return 0;
}

static int do_debug_dump(void)
{
	//dump Image DSP setting
	if (dump_idsp_bin_flag) {
		if (dump_idsp_bin() < 0) {
			perror("dump_idsp_bin failed");
			return -1;
		}
	}

	return 0;
}

static int do_stop_encoding(void)
{
	if (stop_encode(stop_stream_id) < 0)
		return -1;
	return 0;
}

static int do_goto_idle(void)
{
	if (goto_idle() < 0)
		return -1;
	return 0;
}

static int do_vout_setup(void)
{
	if (check_vout() < 0) {
		return -1;
	}

	if (dynamically_change_vout())
		return 0;

	if (vout_flag[VOUT_0] && init_vout(VOUT_0, 0) < 0)
		return -1;
	if (vout_flag[VOUT_1] && init_vout(VOUT_1, 0) < 0)
		return -1;
	return 0;
}

static int do_vin_setup(void)
{
	// select channel: for multi channel VIN (initialize)
	if (channel >= 0) {
		if (select_channel() < 0)
			return -1;
	}

	if (init_vin(vin_mode) < 0)
		return -1;

	return 0;
}

static int do_change_parameter_during_idle(void)
{
	if (osd_mixer_changed_flag ||
		read_encode_info_mode_flag ||
		pip_size_enable_flag ||
		low_delay_cap_flag ) {
		if (set_system_setup_info() < 0)
			return -1;
	}

	if (system_resource_limit_changed_flag) {
		if (setup_resource_limit(source_buffer_max_size_changed_id,
			stream_max_size_changed_id) < 0)
			return -1;
	}

	if (source_buffer_type_changed_flag) {
		source_buffer_type_config();
	}

	if (source_buffer_format_changed_id) {
		if (source_buffer_format_config() < 0)
			return -1;
		source_buffer_format_changed_id = 0;
	}

	if (preview_buffer_format_changed_flag) {
		preview_buffer_format_config();
	}

	setup_resource_limit_if_necessary();

	return 0;
}

static int do_enable_preview(void)
{
	if (enable_preview() < 0)
		return -1;

	return 0;
}


static int do_change_encode_format(void)
{
	if (source_buffer_format_changed_id)  {
		if (source_buffer_format_config() < 0)
			return -1;
	}

	if (set_encode_format() < 0)
		return -1;
	return 0;
}


static int do_change_encode_config(void)
{
	if (set_encode_param() < 0)
		return -1;

	return 0;
}

static int do_start_encoding(void)
{
	if (start_encode(start_stream_id) < 0)
		return -1;
	return 0;
}

static int set_h264_bias_param(void)
{
	int i;
	iav_h264_bias_param_ex_t enc_param;
	int set_flag = 0;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (bias_param_changed_id & (1 << i)) {
			enc_param.id = 1 << i;
			set_flag = 0;
			AM_IOCTL(fd_iav, IAV_IOC_GET_H264_BIAS_PARAM_EX, &enc_param);
			if (enc_param.intra16x16_bias == bias_param[i].intra16x16_bias) {
				printf("Stream %d change intra16x16_bias is %d already\n",
					i, bias_param[i].intra16x16_bias);
			} else {
				enc_param.intra16x16_bias = bias_param[i].intra16x16_bias;
				printf("Stream [%d] change intra16x16_bias [%d].\n",
					i,  bias_param[i].intra16x16_bias);
				set_flag = 1;
			}
			if (enc_param.intra4x4_bias == bias_param[i].intra4x4_bias) {
				printf("Stream %d change intra4x4_bias is %d already\n",
					i, bias_param[i].intra4x4_bias);
			} else {
				enc_param.intra4x4_bias = bias_param[i].intra4x4_bias;
				printf("Stream [%d] change intra4x4_bias [%d].\n",
					i,  bias_param[i].intra4x4_bias);
				set_flag = 1;
			}

			if (enc_param.inter16x16_bias == bias_param[i].inter16x16_bias) {
				printf("Stream %d change inter16x16_bias is %d already\n",
					i, bias_param[i].inter16x16_bias);
			} else {
				enc_param.inter16x16_bias = bias_param[i].inter16x16_bias;
				printf("Stream [%d] change inter16x16_bias [%d].\n",
					i,  bias_param[i].inter16x16_bias);
				set_flag = 1;
			}
			if (enc_param.inter8x8_bias == bias_param[i].inter8x8_bias) {
				printf("Stream %d change inter8x8_bias is %d already\n",
					i, bias_param[i].inter8x8_bias);
			} else {
				enc_param.inter8x8_bias = bias_param[i].inter8x8_bias;
				printf("Stream [%d] change inter8x8_bias [%d].\n",
					i,  bias_param[i].inter8x8_bias);
				set_flag = 1;
			}
			if (enc_param.direct16x16_bias == bias_param[i].direct16x16_bias) {
				printf("Stream %d change direct16x16_bias is %d already\n",
					i, bias_param[i].direct16x16_bias);
			} else {
				enc_param.direct16x16_bias = bias_param[i].direct16x16_bias;
				printf("Stream [%d] change direct16x16_bias [%d].\n",
					i,  bias_param[i].direct16x16_bias);
				set_flag = 1;
			}
			if (enc_param.direct8x8_bias == bias_param[i].direct8x8_bias) {
				printf("Stream %d change direct8x8_bias is %d already\n",
					i, bias_param[i].direct8x8_bias);
			} else {
				enc_param.direct8x8_bias = bias_param[i].direct8x8_bias;
				printf("Stream [%d] change direct8x8_bias [%d].\n",
					i,  bias_param[i].direct8x8_bias);
				set_flag = 1;
			}

			if (enc_param.me_lambda_qp_offset == bias_param[i].me_lambda_qp_offset) {
				printf("Stream %d change me_lambda_qp_offset is %d already\n",
					i, bias_param[i].me_lambda_qp_offset);
			} else {
				enc_param.me_lambda_qp_offset = bias_param[i].me_lambda_qp_offset;
				printf("Stream [%d] change me_lambda_qp_offset [%d].\n",
					i,  bias_param[i].me_lambda_qp_offset);
				set_flag = 1;
			}
			if (set_flag) {
				AM_IOCTL(fd_iav, IAV_IOC_SET_H264_BIAS_PARAM_EX, &enc_param);
			}
		}
	}
	return 0;
}

static int do_real_time_change(void)
{
	if (mirror_pattern_flag) {
		if (set_vin_mirror_pattern() < 0)
			return -1;
	}

	if (framerate_flag) {
		if (set_vin_frame_rate() < 0)
			return -1;
	}

	if (framerate_factor_change_id)  {
		if (change_frame_rate() < 0)
			return -1;
	}

	if (framerate_factor_sync_id) {
		if (sync_frame_rate() < 0)
			return -1;
	}

	if (force_idr_id) {
		if (force_idr_insertion(force_idr_id) <0)
			return -1;
	}

	if (qp_limit_changed_id) {
		if (set_qp_limit() < 0)
			return -1;
	}

	if (intra_mb_rows_changed_id) {
		if (change_intra_refresh_mb_rows() < 0)
			return -1;
	}

	if (prev_a_framerate_div_flag) {
		if (change_prev_a_framerate() < 0) {
			return -1;
		}
	}

	if (qp_matrix_changed_id) {
		if (change_qp_matrix() < 0)
			return -1;
	}

	if (audio_clk_freq_flag) {
		if (change_system_audio_clock() < 0)
			return -1;
	}

	if (panic_control_param_changed_id) {
		if (change_panic_control_param() < 0)
			return -1;
	}

	if (chroma_format_changed_id) {
		if (set_chroma_format() < 0) {
			return -1;
		}
	}

	if (intrabias_p_change_id || intrabias_b_change_id || scene_detect_id) {
		if (set_realtime_h264_enc_param() < 0)
			return -1;
	}

	if (bias_param_changed_id) {
		if (set_h264_bias_param() < 0) {
			return -1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int do_show_status_flag = 0;
	int do_debug_dump_flag = 0;
	int do_stop_encoding_flag = 0;
	int do_goto_idle_flag = 0;
	int do_vout_setup_flag = 0;
	int do_vin_setup_flag = 0;
	int do_change_parameter_during_idle_flag  = 0;
	int do_enable_preview_flag = 0;
	int do_change_encode_format_flag = 0;
	int do_change_encode_config_flag = 0;
	int do_start_encoding_flag = 0;
	int do_real_time_change_flag = 0;

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_default_value() < 0)
		return -1;

	if (init_param(argc, argv) < 0)
		return -1;

	if (map_buffer() < 0)
		return -1;

/********************************************************
 *  process parameters to get flag
 *******************************************************/
	//show message flag
	if (show_encode_stream_info_flag ||
		show_source_buffer_info_flag ||
		show_encode_config_flag ||
		show_qp_hist_info_flag ||
		show_resource_limit_info_flag ||
		show_iav_state_flag ||
		show_iav_driver_info_flag ||
		show_chip_info_flag) {
		do_show_status_flag  = 1;
	}

	//debug dump flag
	if (dump_idsp_bin_flag)  {
		do_debug_dump_flag = 1;
	}

	//stop encoding flag
	stop_stream_id |= restart_stream_id;
	if (vin_flag || source_buffer_type_changed_flag ||
		(source_buffer_format_changed_id & IAV_MAIN_BUFFER)) {
		stop_stream_id = ALL_ENCODE_STREAMS;
	}
	if (stop_stream_id)  {
		do_stop_encoding_flag = 1;
	}

	//go to idle (disable preview) flag
	if (channel < 0) {
		//channel is -1 means VIN is single channel
		if (vout_flag[VOUT_0] || vout_flag[VOUT_1] || vin_flag ||
			(source_buffer_format_changed_id & IAV_MAIN_BUFFER) ||
			preview_buffer_format_changed_flag ||
			source_buffer_type_changed_flag ||
			system_resource_limit_changed_flag ||
			osd_mixer_changed_flag ||
			read_encode_info_mode_flag ||
			pip_size_enable_flag ||
			idle_cmd ||
			low_delay_cap_flag) {
			do_goto_idle_flag = 1;
		}
	}

	//vout setup flag
	if (vout_flag[VOUT_0] || vout_flag[VOUT_1]) {
		do_vout_setup_flag = 1;
	}

	//vin setup flag
	if (vin_flag) {
		do_vin_setup_flag = 1;
	}

	//source buffer flag config during idle flag
	if (do_goto_idle_flag == 1) {
		do_change_parameter_during_idle_flag = 1;
	}

	//enable preview flag
	if ((do_goto_idle_flag == 1 && !nopreview_flag) || preview_cmd) {
		do_enable_preview_flag = 1;
	}

	//set encode format flag
	if (encode_format_changed_id || source_buffer_format_changed_id) {
		do_change_encode_format_flag = 1;
	}

	//set encode param flag
	if (encode_param_changed_id) {
		do_change_encode_config_flag = 1;
	}

	//encode start flag
	start_stream_id |= restart_stream_id;
	if (start_stream_id) {
		do_start_encoding_flag = 1;
	}

	//real time change flag
	if (mirror_pattern_flag ||
		framerate_flag ||
		framerate_factor_change_id ||
		framerate_factor_sync_id ||
		force_idr_id ||
		qp_limit_changed_id ||
		intra_mb_rows_changed_id ||
		prev_a_framerate_div_flag ||
		qp_matrix_changed_id ||
		audio_clk_freq_flag ||
		panic_control_param_changed_id ||
		chroma_format_changed_id ||
		intrabias_p_change_id ||
		intrabias_b_change_id ||
		scene_detect_id ||
		bias_param_changed_id) {
		do_real_time_change_flag = 1;
	}

/********************************************************
 *  execution base on flag
 *******************************************************/
	//show message
	if (do_show_status_flag) {
		if (do_show_status() < 0)
			return -1;
	}

	//debug dump
	if (do_debug_dump_flag) {
		if (do_debug_dump() < 0)
			return -1;
	}

	//stop encoding
	if (do_stop_encoding_flag) {
		if (do_stop_encoding() < 0)
			return -1;
	}

	//disable preview (goto idle)
	if (do_goto_idle_flag) {
		if (do_goto_idle() < 0)
			return -1;
	}

	//vout setup
	if (do_vout_setup_flag) {
		if (do_vout_setup() < 0)
			return -1;
	}

	//vin setup
	if (do_vin_setup_flag) {
		if (do_vin_setup() < 0)
			return -1;
	}

	//change source buffer paramter that needs idle state
	if (do_change_parameter_during_idle_flag) {
		if (do_change_parameter_during_idle() < 0)
			return -1;
	}

	//enable preview
	if (do_enable_preview_flag) {
		if (do_enable_preview() < 0)
			return -1;
	}

	//change encoding format
	if (do_change_encode_format_flag) {
		if (do_change_encode_format() < 0)
			return -1;
	}

	//change encoding param
	if (do_change_encode_config_flag) {
		if (do_change_encode_config() < 0)
			return -1;
	}

	//real time change encoding parameter
	if (do_real_time_change_flag) {
		if (do_real_time_change() < 0)
			return -1;
	}

	//start encoding
	if (do_start_encoding_flag) {
		if (do_start_encoding() < 0)
			return -1;
	}

	return 0;
}

