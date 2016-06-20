/***************************
*
*
*customer_awb_algo.c
*
*
***************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "customer_awb_algo.h"
#include         "img_struct.h"

#define     awbTileNum       384
#define     UNITGAIN          1024
#define     DIFF(A,B)    (((A) > (B))? ((A)-(B)):((B)-(A)))

static    wb_gain_t       DfutGain;
static    wb_gain_t       CurrGain;

static  u8  awb_gray_world_mode(awb_data_t * pTileInfo, wb_gain_t* pNextGain);

void customer_awb_control_init()
{
        DfutGain.r_gain = 1500;
	DfutGain.g_gain = 1024;
	DfutGain.b_gain = 1900;

	CurrGain = DfutGain;
	
}

void customer_awb_control( awb_data_t *pTileInfo,  awb_gain_t *pawbGain )
{
       u8               awbNoWhites;
	wb_gain_t    NextGain;

	NextGain = CurrGain;                                 //initilization

	awbNoWhites= awb_gray_world_mode(pTileInfo, &NextGain);                // update nextgain

	if(awbNoWhites == 1){
		memcpy(&pawbGain->video_wb_gain, &CurrGain, sizeof(wb_gain_t)); // nextgain is not changed
	}
	else if(awbNoWhites == 0){
		static u16 rGainDiff, bGainDiff;
		rGainDiff = DIFF(NextGain.r_gain,  CurrGain.r_gain);
		bGainDiff = DIFF(NextGain.b_gain,  CurrGain.b_gain);

		if(rGainDiff > 500 || bGainDiff > 500){
			pawbGain->video_wb_gain.r_gain =( (NextGain.r_gain * 12) + (CurrGain.r_gain * 116)) /128;
			pawbGain->video_wb_gain.b_gain=( (NextGain.b_gain* 12) + (CurrGain.b_gain* 116)) /128;
			pawbGain->video_wb_gain.g_gain = UNITGAIN;
		}
		else{
			pawbGain->video_wb_gain.r_gain =( (NextGain.r_gain * 8) + (CurrGain.r_gain * 120)) /128;
			pawbGain->video_wb_gain.b_gain=( (NextGain.b_gain* 8) + (CurrGain.b_gain* 120)) /128;
			pawbGain->video_wb_gain.g_gain = UNITGAIN;
		}
	}

	if( pawbGain->video_wb_gain.r_gain != CurrGain.r_gain ||	pawbGain->video_wb_gain.b_gain!= CurrGain.b_gain )
		pawbGain->video_gain_update = 1;
	else
		pawbGain->video_gain_update = 0;
	
       memcpy(&NextGain, &pawbGain->video_wb_gain, sizeof(wb_gain_t));
	CurrGain = NextGain;
}

static  u8  awb_gray_world_mode(awb_data_t * pTileInfo, wb_gain_t* pNextGain)
{
       u32      i;
	u8        awb_no_whites=1;

	u32      r_value,     r_sum,
		    g_value,     g_sum,
		    b_vlaue,     b_sum,
		    rAvg,  gAvg, bAvg, liny,
		    rgbCnt=0;
	
	r_sum = 0;
	g_sum = 0;
	b_sum = 0;

	for (i = 0; i<awbTileNum ; i++){
		r_value = pTileInfo[i].r_avg>>6;
		g_value = pTileInfo[i].g_avg>>6;
		b_vlaue = pTileInfo[i].b_avg>>6;
		liny = pTileInfo[i].lin_y;

		if(r_value>0 && b_vlaue>0 && g_value>0 && liny>10 && liny<50){
			r_sum += r_value;
			g_sum += g_value;
			b_sum += b_vlaue;
			rgbCnt++;
		}
	}

	if (rgbCnt > 0 ) {
		rAvg= r_sum / rgbCnt;
		gAvg= g_sum / rgbCnt;
		bAvg= b_sum / rgbCnt;

		if(rAvg > 0 && bAvg > 0){
			pNextGain->r_gain = gAvg * UNITGAIN/ rAvg;
			pNextGain->g_gain= UNITGAIN;
			pNextGain->b_gain= gAvg * UNITGAIN/ bAvg;
		}
		
		awb_no_whites= 0;
	} 
	return (awb_no_whites);
	
}
