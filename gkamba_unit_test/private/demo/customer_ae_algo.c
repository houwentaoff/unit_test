/***************************
*
*
*customer_ae_algo.c
*
*
***************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "customer_ae_algo.h"
#include "img_struct.h"

#define  TileNum    96

static   joint_t    CurrPoint;
static   u16       aeTarget;

static  u32        MinShutterGain,   MaxShutterGain;
static  u32        MinagcGain,         MaxagcGain;

static  u32        ae_average_mode(ae_data_t *pTileInfo);

void ae_decide_exposure(s32 ChangeSign, ae_info_t*  pAeInfo)
{
       joint_t       NextPoint;
       s32            steps;
	s32           ShutterTemp = 0,     agcTemp = 0;

	steps            =  ChangeSign;
	memcpy(&NextPoint, &CurrPoint,sizeof(joint_t));                                                                  //initilization of next exposure point

	if(steps < 0){                                    //************************************Image is to bright**********************************************
		agcTemp = CurrPoint.factor[1] +steps;
		if( agcTemp>= MinagcGain && agcTemp > 0)
			NextPoint.factor[1] = agcTemp;
		else {
			ShutterTemp =  CurrPoint.factor[0] + steps;
			if(ShutterTemp<= MinShutterGain && ShutterTemp > 0)
				ShutterTemp= MinShutterGain;
			NextPoint.factor[0] = ShutterTemp;
		}
	}
	else if(steps > 0) {                      //*************************************Image is too dark**********************************************
	       agcTemp = CurrPoint.factor[1] + steps;
	       if(  agcTemp<= MaxagcGain)
		   	NextPoint.factor[1] = agcTemp; 
		else {
			ShutterTemp=  CurrPoint.factor[0] + steps;
			if( ShutterTemp>= MaxShutterGain )
				ShutterTemp = MaxShutterGain;
			NextPoint.factor[0] = ShutterTemp;
		}
	}
	
	//****************************************************************update*************************************************

	if(NextPoint.factor[1] != CurrPoint.factor[1]){                                                                              // agc
		pAeInfo->agc_index = NextPoint.factor[1];
		pAeInfo->agc_update=1;
	}

	if(NextPoint.factor[0] != CurrPoint.factor[0]){                                                                            // shutter
	       pAeInfo->shutter_index = 2047 - NextPoint.factor[0];
		pAeInfo->shutter_update = 1;
	}

	if(NextPoint.factor[2] != CurrPoint.factor[2]){                                                                            // iris
	       pAeInfo->iris_index = NextPoint.factor[2];
		pAeInfo->iris_update = 1;
	}
	CurrPoint =  NextPoint;
}

void customer_ae_control_init( )
{
	aeTarget   = 48;

	MinShutterGain = 2047 - 1524;     MaxShutterGain = 2047 - 1012;
	MinagcGain       =2;                       MaxagcGain         = 640;
	
	CurrPoint.factor[0] = 2047 -1106; 
	CurrPoint.factor[1] = 128;
	CurrPoint.factor[2] = 0;
}

void customer_ae_control(ae_data_t* pTileInfo,  ae_info_t*  pVideoAeInfo)
{
      u16            aeTarget_high,  	          aeTarget_low,
	  	         aeTarget_higher,              aeTarget_lower,
	  	         aeTarget_highest,             aeTarget_lowest,
	  	          aeTarget_tmp;
      u32            aeLumaAvge;

      s32              chngeSign;
	                  chngeSign= 0;

      pVideoAeInfo->shutter_index = 2047 - CurrPoint.factor[0];
      pVideoAeInfo->agc_index      = CurrPoint.factor[1];
      pVideoAeInfo->iris_index       = CurrPoint.factor[2];

      aeTarget_tmp       =      aeTarget;
      aeTarget_high       =     aeTarget_tmp * 104/100;
      aeTarget_higher    =     aeTarget_tmp * 109 / 100;
      aeTarget_highest   =    aeTarget_tmp * 115 /100;
      aeTarget_low        =      aeTarget_tmp * 97/100;
      aeTarget_lower     =     aeTarget_tmp  * 92 / 100;
      aeTarget_lowest    =     aeTarget_tmp  * 86 / 100;

      aeLumaAvge = ae_average_mode(pTileInfo);

	if(aeLumaAvge > aeTarget_highest)
		chngeSign= -4;                   //cut down  ae exposure to make it daker;
	else if(aeLumaAvge > aeTarget_higher)
		chngeSign = -2;
	else if(aeLumaAvge > aeTarget_high)
		chngeSign = -1;
	else if(aeLumaAvge < aeTarget_lowest)
	  	chngeSign =  4;                      //pull up ae expousure to make it brighter;
	else if(aeLumaAvge < aeTarget_lower )
		chngeSign = 2;
	else if(aeLumaAvge < aeTarget_low)
		chngeSign = 1;

      ae_decide_exposure(chngeSign, pVideoAeInfo);

}

static  u32 ae_average_mode(ae_data_t *pTileInfo)
{
        u32      luma,
		     LumaSum=0;
        u32      TileCount=0;
	 u32      AvgeLuma=0;
	 u16      i;

	 for(i=0;i<TileNum;i++){
	 	luma = pTileInfo[i].lin_y>>6;
		LumaSum += luma;
		TileCount++;		
	 }
	 if(TileCount>0)
	 	AvgeLuma = LumaSum / TileCount;
	 else
	 	AvgeLuma = 0;

	 return AvgeLuma;
}