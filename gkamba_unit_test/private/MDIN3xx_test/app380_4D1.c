//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
//
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:	APP380.C
// Description 		:
// Ref. Docment		:
// Revision History 	:

//--------------------------------------------------------------------------------------------------------------------------
// Note for Host Clock Interface
//--------------------------------------------------------------------------------------------------------------------------
// TEST_MODE1() is GPIO(OUT) pin of CPU. This is connected to TEST_MODE1 of MDIN3xx.
// TEST_MODE2() is GPIO(OUT) pin of CPU. This is connected to TEST_MODE2 of MDIN3xx.

//--------------------------------------------------------------------------------------------------------------------------
// You must make drive functions for I2C read & I2C write.
//--------------------------------------------------------------------------------------------------------------------------
// static BYTE MDINI2C_Write(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes)
// static BYTE MDINI2C_Read(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes)

//--------------------------------------------------------------------------------------------------------------------------
// You must make drive functions for BUS read & BUS write.
//--------------------------------------------------------------------------------------------------------------------------
// static void MDINBUS_Write(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes);
// static void MDINBUS_Read(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes);

//--------------------------------------------------------------------------------------------------------------------------
// If you want to use dma-function, you must make drive functions for DMA read & DMA write.
//--------------------------------------------------------------------------------------------------------------------------
// MDIN_ERROR_t MDINBUS_DMAWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
// MDIN_ERROR_t MDINBUS_DMARead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);

//--------------------------------------------------------------------------------------------------------------------------
// You must make drive functions for delay (usec and msec).
//--------------------------------------------------------------------------------------------------------------------------
// MDIN_ERROR_t MDINDLY_10uSec(WORD delay) -- 10usec unit
// MDIN_ERROR_t MDINDLY_mSec(WORD delay) -- 1msec unit

//--------------------------------------------------------------------------------------------------------------------------
// You must make drive functions for debug (ex. printf).
//--------------------------------------------------------------------------------------------------------------------------
// void UARTprintf(const char *pcString, ...)

// ----------------------------------------------------------------------
// Include files
// ----------------------------------------------------------------------
#ifndef		__MDIN3xx_H__
#include	 "mdin3xx.h"
#endif

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
static MDIN_VIDEO_INFO		stVideo;
static MDIN_INTER_WINDOW	stInterWND;

// ----------------------------------------------------------------------
// External Variable
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Exported functions
// ----------------------------------------------------------------------

//-----------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <getopt.h>
#include "RN6264_i2c.h"

#include <ctype.h>
#include <stdlib.h>

#define GPIO_MODE1 28
#define GPIO_MODE2 30


#define	NO_ARG		0
#define	HAS_ARG		1


static const char *short_options = "hp:r:w:d:sM:";
static struct option long_options[] = {
	{"help", NO_ARG, 0, 'h'},
	{"nID",	HAS_ARG, 0, 'p'},
	{"read", HAS_ARG, 0, 'r'},
	{"write", HAS_ARG, 0, 'w'},
	{"Data", HAS_ARG, 0, 'd'},
	{"hw_reset", NO_ARG, 0, 's'},
	{"video_mode", HAS_ARG, 0, 'M'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tmdin_test usage"},
	{"byte", "MDINxxx page ID, can't using for RNxxxx"},
	{"", "\t\tMDINxxx:u32, RNxxxx:u8"},
	{"", "\t\tMDINxxx:u32, RNxxxx:u8"},
	{"", "\t\tMDINxxx:u16, RNxxxx:u8"},
	{"", "\t\t HW reset, GPIO93"},
	{"0/1/2", "\tPAL =0 , NTSC =1,960H_NTSC = 2,Notice:using 960H must after init.sh --mdin380_960H"},

};

void usage(void)
{
	int i;
	printf("mdin_test usage:\n");
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
	printf("Examples: \n");
	printf("\t test_mdin -s \t\t HW reset\n");
	printf("\t test_mdin -M 0 \t\t init mdin380 with PAL mode\n");
	printf("\t test_mdin -M 1 \t\t init mdin380 with NTSC mode\n");
	printf("\t test_mdin -s -M 0 \t\t HW reset and then init mdin380 with PAL mode\n");
	printf("\t test_mdin -p xx -r xx \t\t read mdin380 register value\n");
	printf("\t test_mdin -p xx -w xx -d xx \t\t write mdin380 register value\n");
	printf("\t test_mdin -r xx \t\t read RN6264 register value\n");
	printf("\t test_mdin -w xx -d xx \t\t write RN6264 register value\n");
	printf("\n");
}

static int hw_reset_flag=0;
static int Page_Addr_flag=0;
static int Read_flag=0;
static int Write_flag=0;
static int help_flag = 0;
static int init_flag = 0;

static int video_mode=0;

static u8 test_nID;

static u32 Mdinxxx_rAddr=0;
static u16 *Mdinxxx_Data;

static u8 RNxxxx_rAddr = 0;
static u8 *RNxxxx_Data;




static int	amba_gpio_config(u8 gpio_id,u8 inout){
	int _export, direction;
	char buf[128];

	_export = open("/sys/class/gpio/export", O_WRONLY);
	if (_export < 0) {
		printf("%s: Can't open export sys file!\n", __func__);
		goto gpio_config_exit;
	}
	sprintf(buf, "%d", gpio_id);
	write(_export, buf, sizeof(buf));
	close(_export);

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_config_exit;
	}
	if (inout)
		sprintf(buf, "out");
	else
		sprintf(buf, "in");
	write(direction, buf, sizeof(buf));
	close(direction);

gpio_config_exit:
	return 0;
}

static void amba_gpio_set(u8 gpio_id)
{
	int direction;
	char buf[128];

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_set_exit;
	}
	sprintf(buf, "high");
	write(direction, buf, sizeof(buf));
	close(direction);

gpio_set_exit:
	return;
}

static void amba_gpio_clr(u8 gpio_id)
{
	int direction;
	char buf[128];

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_clr_exit;
	}
	sprintf(buf, "low");
	write(direction, buf, sizeof(buf));
	close(direction);

gpio_clr_exit:
	return;
}

#define HW_RESET(GPIO)	\
	do {	\
		amba_gpio_config(GPIO,1);	\
		amba_gpio_clr(GPIO);	\
		MDINDLY_mSec(150);			\
		amba_gpio_set(GPIO);	\
		MDINDLY_mSec(50);	\
	}while(0)


#define TEST_MODE1(val)	\
	do {	\
		amba_gpio_config(GPIO_MODE1,1);	\
		if(val)					\
			amba_gpio_set(GPIO_MODE1);	\
		else					\
			amba_gpio_clr(GPIO_MODE1);	\
	}while(0)

#define TEST_MODE2(val)	\
	do {	\
		amba_gpio_config(GPIO_MODE2,1);	\
		if(val)					\
			amba_gpio_set(GPIO_MODE2);	\
		else					\
			amba_gpio_clr(GPIO_MODE2);	\
	}while(0)

//--------------------------------------------------------------------------------------------------------------------------

#if	!defined(SYSTEM_USE_PCI_HIF)&&defined(SYSTEM_USE_MCLK202)
static void MDIN3xx_SetHCLKMode(MDIN_HOST_CLK_MODE_t mode)
{
	switch (mode) {
		case MDIN_HCLK_CRYSTAL:	TEST_MODE2( LOW); TEST_MODE1( LOW); break;
		case MDIN_HCLK_MEM_DIV: TEST_MODE2(HIGH); TEST_MODE1(HIGH); break;

	#if	defined(SYSTEM_USE_MDIN380)
		case MDIN_HCLK_HCLK_IN: TEST_MODE2( LOW); TEST_MODE1(HIGH); break;
	#endif
	}
}
#endif

//--------------------------------------------------------------------------------------------------------------------------
static void CreateMDIN380VideoInstance(void)
{
	WORD nID = 0;

//	MDIN_RST(ON);
//	MDINDLY_mSec(10);	// delay 10ms

#if	!defined(SYSTEM_USE_PCI_HIF)&&defined(SYSTEM_USE_MCLK202)
	MDIN3xx_SetHCLKMode(MDIN_HCLK_CRYSTAL);	// set HCLK to XTAL
	MDINDLY_mSec(10);	// delay 10ms
#endif

#if defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_BUS_HIF)
#if		CPU_ACCESS_BUS_NBYTE == 1
	MDIN3xx_SetHostDataMapMode(MDIN_HOST_DATA_MAP2);	// mux-16bit map
#elif	CPU_ACCESS_BUS_NOMUX == 1
	MDIN3xx_SetHostDataMapMode(MDIN_HOST_DATA_MAP0);	// sep-8bit map
#else
	MDIN3xx_SetHostDataMapMode(MDIN_HOST_DATA_MAP1);	// mux-8bit map
#endif
#endif

	while (nID!=0x85) MDIN3xx_GetChipID(&nID);	// get chip-id
	MDIN3xx_EnableMainDisplay(OFF);		// set main display off
	MDIN3xx_SetMemoryConfig();			// initialize DDR memory

#if 	!defined(SYSTEM_USE_PCI_HIF)&&defined(SYSTEM_USE_MCLK202)
	MDIN3xx_SetHCLKMode(MDIN_HCLK_MEM_DIV);	// set HCLK to MCLK/2
	MDINDLY_mSec(10);	// delay 10ms
#endif

	MDIN3xx_SetVCLKPLLSource(MDIN_PLL_SOURCE_XTAL);		// set PLL source
	MDIN3xx_EnableClockDrive(MDIN_CLK_DRV_ALL, ON);

	MDIN3xx_SetInDataMapMode(MDIN_IN_DATA36_MAP0);		// set in-data map
//	MDIN3xx_SetDIGOutMapMode(MDIN_DIG_OUT_M_MAP0);		// disable digital out
//	MDIN3xx_SetDIGOutMapMode(MDIN_DIG_OUT_X_MAP12);		// enable digital out
	MDIN3xx_SetDIGOutMapMode(MDIN_DIG_OUT_X_MAP15);
	// setup enhancement
	MDIN3xx_SetFrontNRFilterCoef(NULL);		// set default frontNR filter coef
	MDINAUX_SetFrontNRFilterCoef(NULL);		// set default frontNR filter coef
	MDIN3xx_SetPeakingFilterCoef(NULL);		// set default peaking filter coef
	MDIN3xx_SetColorEnFilterCoef(NULL);		// set default color enhancer coef
	MDIN3xx_SetBlockNRFilterCoef(NULL);		// set default blockNR filter coef
	MDIN3xx_SetMosquitFilterCoef(NULL);		// set default mosquit filter coef
	MDIN3xx_SetSkinTonFilterCoef(NULL);		// set default skinton filter coef

	MDIN3xx_EnableLTI(OFF);					// set LTI off
	MDIN3xx_EnableCTI(OFF);					// set CTI off
	MDIN3xx_SetPeakingFilterLevel(0);		// set peaking gain
	MDIN3xx_EnablePeakingFilter(ON);		// set peaking on

	MDIN3xx_EnableFrontNRFilter(OFF);		// set frontNR off
	MDIN3xx_EnableBWExtension(OFF);			// set B/W extension off


	MDIN3xx_SetIPCBlock();		// initialize IPC block (3DNR gain is 34)

	memset((PBYTE)&stVideo, 0, sizeof(MDIN_VIDEO_INFO));

	MDIN3xx_SetMFCHYFilterCoef(&stVideo, NULL);	// set default MFC filters
	MDIN3xx_SetMFCHCFilterCoef(&stVideo, NULL);
	MDIN3xx_SetMFCVYFilterCoef(&stVideo, NULL);
	MDIN3xx_SetMFCVCFilterCoef(&stVideo, NULL);

	// set aux display ON
	stVideo.dspFLAG = MDIN_AUX_DISPLAY_ON | MDIN_AUX_FREEZE_OFF;

	// set video path (main/aux/dac/enc)
	stVideo.srcPATH = PATH_MAIN_A_AUX_M;	// set main is A, aux is A , 01Aug2011
	stVideo.dacPATH = DAC_PATH_AUX_4CH;	// set 4ch mode , 01Aug2011
	stVideo.encPATH = VENC_PATH_PORT_X;		// set venc is aux

	// if you need to front format conversion then set size.
//	stVideo.ffcH_m  = 1440;

	// define video format of PORTA-INPUT
#if	defined(SYSTEM_USE_4D1_IN)
//#if 	defined(NTSC_4D1_IN)
	if ((video_mode == VDCNV_4CH_ON_NTSC) || (video_mode == VDCNV_4CH_ON_NTSC_960))
		stVideo.stSRC_a.frmt = VIDSRC_720x480i60;	// 08Aug2011
//#endif
//#if	defined(PAL_4D1_IN)
	else
		stVideo.stSRC_a.frmt = VIDSRC_720x576i50;	// 08Aug2011
//#endif
#endif
	stVideo.stSRC_a.mode = MDIN_SRC_MUX656_8;	// 01Aug2011
	stVideo.stSRC_a.fine = MDIN_FIELDID_BYPASS | MDIN_LOW_IS_TOPFLD;

	// define video format of MAIN-OUTPUT
#if	defined(SYSTEM_USE_4D1_IN)
//#if 	defined(NTSC_4D1_IN)
if ((video_mode == VDCNV_4CH_ON_NTSC) || (video_mode == VDCNV_4CH_ON_NTSC_960))
		stVideo.stOUT_m.frmt = VIDOUT_720x480p60;	// 08Aug2011
//#endif
//#if	defined(PAL_4D1_IN)
	else
		stVideo.stOUT_m.frmt = VIDOUT_720x576p50;	// 08Aug2011
//#endif
#endif
	stVideo.stOUT_m.mode = MDIN_OUT_YUV444_8;	// 01Aug2011
	stVideo.stOUT_m.fine = MDIN_SYNC_FREERUN;	// set main outsync free-run

	stVideo.stOUT_m.brightness = 128;			// set main picture factor
	stVideo.stOUT_m.contrast = 128;
	stVideo.stOUT_m.saturation = 128;
	stVideo.stOUT_m.hue = 128;

#if RGB_GAIN_OFFSET_TUNE == 1
	stVideo.stOUT_m.r_gain = 128;				// set main gain/offset
	stVideo.stOUT_m.g_gain = 128;
	stVideo.stOUT_m.b_gain = 128;
	stVideo.stOUT_m.r_offset = 128;
	stVideo.stOUT_m.g_offset = 128;
	stVideo.stOUT_m.b_offset = 128;
#endif

	// define video format of IPC-block
	stVideo.stIPC_m.mode = MDIN_DEINT_ADAPTIVE;
	stVideo.stIPC_m.film = MDIN_DEINT_FILM_ALL;
	stVideo.stIPC_m.gain = 34;

	if (video_mode == VDCNV_4CH_ON_NTSC_960)
	{
		stVideo.stIPC_m.fine = MDIN_DEINT_3DNR_OFF;
		// define map of frame buffer
		stVideo.stMAP_m.frmt = MDIN_MAP_AUX_ON_NR_OFF;	// when MDIN_DEINT_3DNR_OFF
	}
	else
	{
		stVideo.stIPC_m.fine = MDIN_DEINT_3DNR_ON | MDIN_DEINT_CCS_ON;
		stVideo.stMAP_m.frmt = MDIN_MAP_AUX_ON_NR_ON;
	}

	// define video format of PORTB-INPUT
	stVideo.stSRC_b.frmt = VIDSRC_720x480i60;
	stVideo.stSRC_b.mode = MDIN_SRC_MUX656_8;
	stVideo.stSRC_b.fine = MDIN_FIELDID_INPUT | MDIN_LOW_IS_TOPFLD;

	// define video format of AUX-OUTPUT
#if	defined(SYSTEM_USE_4D1_IN) && defined(SYSTEM_USE_4D1_IN_QUADOUT) // for 4D1 input Quad output mode
//	#if defined(NTSC_4D1_IN)
//	stVideo.stOUT_x.frmt = VIDOUT_1920x1080p60;	// for full screen output, 01Aug2011
	if (video_mode == VDCNV_4CH_ON_NTSC)
	{
		stVideo.stOUT_x.frmt = VIDOUT_1600x1200p60;	// for no scale(1:1) output (1440x960), 01Aug2011
	}
	else if (video_mode == VDCNV_4CH_ON_NTSC_960 )
		stVideo.stOUT_x.frmt = VIDOUT_1920x1200pRB;	// for no scale(1:1) output (1980x960), 26Oct2011
//	#endif
//	#if defined(PAL_4D1_IN)
//	stVideo.stOUT_x.frmt = VIDOUT_1920x1080p50;	// for full screen output, 01Aug2011
	else {
		stVideo.stOUT_x.frmt = VIDOUT_1600x1200p60;	// for no scale(1:1) output (1440x1152), 01Aug2011
	}
//	#endif
//	stVideo.stOUT_x.mode = MDIN_OUT_RGB444_8; //MDIN_EMB422_8 YUV422	// 01Aug2011
	stVideo.stOUT_x.mode = MDIN_OUT_EMB422_8; //MDIN_OUT_EMB422_8 YUV422
#endif

#if	defined(SYSTEM_USE_4D1_IN) && defined(SYSTEM_USE_4D1_IN_656OUT) // for 4D1 input BT656 output mode
	stVideo.stOUT_x.frmt = VIDOUT_720x480i60;
	stVideo.stOUT_x.mode = MDIN_OUT_MUX656_8;
#endif
	stVideo.stOUT_x.fine = MDIN_SYNC_FREERUN;	// set aux outsync free-run

	stVideo.stOUT_x.brightness = 128;			// set aux picture factor
	stVideo.stOUT_x.contrast = 128;
	stVideo.stOUT_x.saturation = 128;
	stVideo.stOUT_x.hue = 128;

#if RGB_GAIN_OFFSET_TUNE == 1
	stVideo.stOUT_x.r_gain = 128;				// set aux gain/offset
	stVideo.stOUT_x.g_gain = 128;
	stVideo.stOUT_x.b_gain = 128;
	stVideo.stOUT_x.r_offset = 128;
	stVideo.stOUT_x.g_offset = 128;
	stVideo.stOUT_x.b_offset = 128;
#endif

#if	defined(SYSTEM_USE_4D1_IN)
	// define video format of 4CH-display, 	01Aug2011
	stVideo.st4CH_x.chID  = MDIN_4CHID_IN_SYNC;	// set CH-ID extract
	stVideo.st4CH_x.order = MDIN_4CHID_A1A2B1B2; 	// set CH-ID mapping
	stVideo.st4CH_x.view  = MDIN_4CHVIEW_ALL;	// set 4CH view mode
#endif

	// define video format of video encoder
	stVideo.encFRMT = VID_VENC_NTSC_M;

	// define video format of HDMI-OUTPUT
	stVideo.stVID_h.mode  = HDMI_OUT_RGB444_8;
	stVideo.stVID_h.fine  = HDMI_CLK_EDGE_RISE;

	stVideo.stAUD_h.frmt  = AUDIO_INPUT_I2S_0;						// audio input format
	stVideo.stAUD_h.freq  = AUDIO_MCLK_256Fs | AUDIO_FREQ_48kHz;	// sampling frequency
	stVideo.stAUD_h.fine  = AUDIO_MAX24B_MINUS0 | AUDIO_SD_JUST_LEFT | AUDIO_WS_POLAR_HIGH |
							AUDIO_SCK_EDGE_RISE | AUDIO_SD_MSB_FIRST | AUDIO_SD_1ST_SHIFT;
	MDINHTX_SetHDMIBlock(&stVideo);		// initialize HDMI block

	// define window for inter-area
	stInterWND.lx = 315;	stInterWND.rx = 405;
	stInterWND.ly = 90;		stInterWND.ry = 150;
	MDIN3xx_SetDeintInterWND(&stInterWND, MDIN_INTER_BLOCK0);
	MDIN3xx_EnableDeintInterWND(MDIN_INTER_BLOCK0, OFF);

//	stVideo.exeFLAG = MDIN_UPDATE_MAINFMT;	// execution of video process
	stVideo.exeFLAG = MDIN_UPDATE_MAINFMT | MDIN_UPDATE_AUXFMT;
}

//--------------------------------------------------------------------------------------------------------------------------
void Set_AUXViewSize(WORD w, WORD h, WORD x, WORD y)
{
	MDIN_VIDEO_WINDOW  stView;

	stView.w = w;	// width
	stView.h = h;	// height
	stView.x = x;	// x position
	stView.y = y;	// y position

	MDINAUX_SetVideoWindowVIEW(&stVideo, stView);	// set view size
}


int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int rval = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
			switch (ch) {
				case 'h':
					help_flag = 1;
					usage();
					break;
				case 'p':
				/*	rval = sscanf(optarg, "0x%02x", &test_nID);
					if (rval != 1 )
					{
						printf("input error \n");
						return -1;
					}	*/
					test_nID= atoi(optarg);
					Page_Addr_flag=1;
					break;
				case 'r':
					Read_flag = 1;
					if (Page_Addr_flag)
					{
					/*	rval = sscanf(optarg, "0x%08x", &Mdinxxx_rAddr);
						if (rval != 1 )
						{
							printf("input error \n");
							return -1;
						}
						*/
						Mdinxxx_rAddr= atoi(optarg);
					}
					else
					{
					/*	rval = sscanf(optarg, "0x%02x", &RNxxxx_rAddr);
						if (rval != 1 )
						{
							printf("input error \n");
							return -1;
						}*/
						RNxxxx_rAddr= atoi(optarg);
					}
					break;
				case 'w':
					Write_flag = 1;
					if (Page_Addr_flag)
					{
					/*	rval = sscanf(optarg, "0x%04x", &Mdinxxx_rAddr);
						if (rval != 1 )
						{
							printf("input error \n");
							return -1;
						}
						*/
					Mdinxxx_rAddr= atoi(optarg);
					}

					else
					{
					/*	rval = sscanf(optarg, "0x%02x", &RNxxxx_rAddr);
						if (rval != 1 )
						{
							printf("input error \n");
							return -1;
						}
						*/
					RNxxxx_rAddr= atoi(optarg);
					}
					break;
				case 'd':
					if (Write_flag != 1)
					{
						printf("No data \n");
						return -1;
					}
					if (Page_Addr_flag)
					{
						Mdinxxx_Data = (u16 *)malloc(sizeof(u16));
					/*	rval = sscanf(optarg, "0x%04x", Mdinxxx_Data);
						if (rval != 1 )
						{
							printf("input error \n");
							return -1;
						}
						*/
						*Mdinxxx_Data= atoi(optarg);
					}
					else
					{
						RNxxxx_Data = (u8 *)malloc(sizeof(u8));
					/*	rval = sscanf(optarg, "0x%02x", RNxxxx_Data);
						if (rval != 1 )
						{
							printf("input error \n");
							return -1;
						}
						*/
						*RNxxxx_Data= atoi(optarg);
					}
					break;
				case 's':
					hw_reset_flag = 1;
					break;
				case 'M':
					rval = sscanf(optarg, "%d", &video_mode);
						if (rval != 1 )
						{
							printf("input error \n");
							return -1;
						}
					init_flag = 1;
					break;
				default:
			//		printf("unknown command %s \n", optarg);
			//			return -1;
					break;
			}
		}
	return 0;

}



//--------------------------------------------------------------------------------------------------------------------------
int main(int argc, char **argv)
{

	if (argc < 2)
	{
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0)
	{
		usage();
		return -1;
	}

	if ( help_flag == 1)
		return 0;

	if (hw_reset_flag)
	{
		HW_RESET(93);
	}

	if (Page_Addr_flag)
	{
		if (Write_flag)
		{
			MDINI2C_RegWrite(test_nID,Mdinxxx_rAddr,*Mdinxxx_Data);
			printf("MDINI2C_RegWrite nID=0x%x rAddr=0x%04x wData=0x%04x  \n",test_nID,Mdinxxx_rAddr,*Mdinxxx_Data);
		}
		if (Read_flag)
		{
			Mdinxxx_Data = (u16 *)malloc(sizeof(u16));
			MDINI2C_RegRead(test_nID,Mdinxxx_rAddr,Mdinxxx_Data);
			printf("MDINI2C_RegRead nID=0x%x rAddr=0x%04x wData=0x%04x  \n",test_nID,Mdinxxx_rAddr,*Mdinxxx_Data);
		}
		return 0;

	}
	else
	{
		if (Write_flag)
		{
			RN6264_I2c_write(RNxxxx_rAddr,*RNxxxx_Data);
			printf("RN6264_I2c_write rAddr=0x%02x wData=0x%02x  \n",RNxxxx_rAddr,*RNxxxx_Data);
			return 0;
		}
		if (Read_flag)
		{
			RNxxxx_Data = (u8 *)malloc(sizeof(u8));
			RN6264_I2c_read(RNxxxx_rAddr,RNxxxx_Data);
			printf("RN6264_I2c_read rAddr=0x%02x wData=0x%02x  \n",RNxxxx_rAddr,*RNxxxx_Data);
			return 0;
		}
	}

	if (init_flag == 1)
	{
		if (RN6264_SetVideoSource(video_mode) < 0)
			return -1;
		printf(" RNXXXX i2c install successfully! \n");

		CreateMDIN380VideoInstance();			// initialize MDIN-380
		if (stVideo.exeFLAG) {					// check change video formats
			MDIN3xx_EnableMainDisplay(OFF);
			MDIN3xx_EnableAuxDisplay(&stVideo, OFF);

			MDIN3xx_VideoProcess(&stVideo);		// mdin3xx main video process

		#if	defined(SYSTEM_USE_4D1_IN) && defined(SYSTEM_USE_4D1_IN_QUADOUT) // for 4D1 input Quad output mode
			// for no scale(1:1) output (1600x1200)
//			#if	defined(NTSC_4D1_IN)
			if (video_mode == VDCNV_4CH_ON_NTSC)
				Set_AUXViewSize(1440,  960, 0, 0);	// for NTSC input, set view size, added on 01Aug2011
//			#endif
//			#if	defined(PAL_4D1_IN)
			else if (video_mode == VDCNV_4CH_ON_NTSC_960)
				Set_AUXViewSize(1920, 960, 0, 0);	// for PAL  input, set view size, added on 01Aug2011
			else
				Set_AUXViewSize(1440, 1152, 0, 0);	// for PAL  input, set view size, added on 01Aug2011
//			#endif

			MDINAUX_SetOut4CH_OutQuad(video_mode);
			if (video_mode == VDCNV_4CH_ON_NTSC_960)
				MDIN3xx_SetOut4CH_OutVsync_Half(ON);	// for half frequency (vsync: 60 to 30Hz, 50 to 25Hz)
			else
				MDIN3xx_SetOut4CH_OutVsync_Half(OFF);	// for half frequency (vsync: 60 to 30Hz, 50 to 25Hz)

		#endif
		#if	defined(SYSTEM_USE_4D1_IN) && defined(SYSTEM_USE_4D1_IN_656OUT) // for 4D1 input BT656 output mode
			MDINAUX_SetOut4CH_OutBT656(video_mode);
			MDIN3xx_SetOut4CH_OutBT656(video_mode);
		#endif
//			MDIN3xx_SetOutTestPattern(MDIN_OUT_TEST_COLOR);
//			MDIN3xx_EnableAuxDisplay(&stVideo, ON);
			MDIN3xx_EnableMainDisplay(ON);
			MDIN3xx_EnableAuxDisplay(&stVideo, ON);

			printf(" MDIN380 i2c install successfully! \n");
		}
	}
	return 0;
}

/*  FILE_END_HERE */
