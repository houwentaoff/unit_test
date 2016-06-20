#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "config.h"
#include "capability.h"

#define PROC_ADC_FILE       "/proc/ambarella/adc"
#define FILE_SIZE_MAX		256
#define IRIS_DRV_FLAG_ADC3       "adc3 = 0x"
#define IRIS_DRV_FLAG_ADC0       "adc0 = 0x"
#define ADC_HEX_LEN			3

static SysEncoderBoardId g_EncoderBoardId = e_ENCODER_BOARD_NORMAL;

static int init_capability_param(void)
{
	int ret = -1;
	SysReturnValue sysret = e_FAIL;
	
	CapabilityHandle handle = Capability_CreateHandle();
	if (INVALID_HANDLE == handle){
		printf("Capability_CreateHandle fail.\n");
		goto errExit; 
	}

	sysret = Capability_GetEncoderBoardId(handle, &g_EncoderBoardId);
	if (e_SUCCESS != sysret) {
		printf("Capability_GetEncoderBoardId fail.\n");
		goto errExit; 
	}

	//set flag
	ret = 0;

errExit:

	//clean resource
	Capability_DestroyHandle(handle);

	return ret;
}

int GetLightMode(void) {
	FILE *fp = NULL;
	char buf[FILE_SIZE_MAX];
	char *p1, *p2;
	int i, val;
	const char *iris_drv_flag = IRIS_DRV_FLAG_ADC0;
		
	if (e_ENCODER_BOARD_LARK == g_EncoderBoardId){
		iris_drv_flag = IRIS_DRV_FLAG_ADC3;
	}else{
		iris_drv_flag = IRIS_DRV_FLAG_ADC0;
	}//endof if (e_ENCODER_BOARD_LARK == g_EncoderBoardId)

	fp = fopen(PROC_ADC_FILE, "r");
	if(fp == NULL) {
		printf("ERR : %s : %d\n", __FILE__, __LINE__);
		return -1;
	}
	
//		printf("adc1 val : %d\n", val);

	fread(buf, FILE_SIZE_MAX, 1, fp);
//	printf("buf : %s\n", buf);
	p1 = p2 = NULL;
	p1 = strstr(buf, iris_drv_flag);
	p2 = p1 + strlen(iris_drv_flag);
//	printf("strstr : %s\n", p2);
	for(i = 0, val = 0; i < ADC_HEX_LEN; i ++) {
		if((*(p2 + i) >= '0') && (*(p2 + i) <= '9'))
			val += (*(p2 + i) - '0') << ((ADC_HEX_LEN - 1 - i) * 4);
		else 
			val += (*(p2 + i) - 'a' + 10) << ((ADC_HEX_LEN - 1 - i) * 4);
	}

//	printf("adc0 val : %d\n", val);
	fclose(fp);

	return val;
}


int main(int argc, char *argv[])
{
	FILE *fp = NULL;
	int light = 0;
	int enable_ircut = 0;
#define FILE_NAME "/etc/ambaipcam/calibration/ircut_adc.cfg"
	char filename[100] =  {0};
#define READ_TIMES 200
	int cnt = READ_TIMES;
	int sleep_time_us = 10000;
	int i;
	int min = 0;
	int max = 0;
	double sum = 0;
	int  aver = 0;
	int tmplight = 0;
	int adc_avg[2][2] ={ {0}};

	system("echo stop image_server > /tmp/__server_manager_fifo_file__");

	system("mkdir /etc/ambaipcam/calibration");

	if (0 != init_capability_param()){
		printf("init_capability_param fail.\n");
		return -1;
	}
	printf("ircut calibrate start lark bq version\n");
	//sprintf(filename, FILE_NAME, light, enable_ircut);
	sprintf(filename, FILE_NAME);
	fp = fopen(filename, "w+");
	if (!fp) {
		printf("open %s failed\n", filename);
		return -1;
	}

	fprintf(fp, "# lark bq ircut defalut value\n\n");
	fprintf(fp, "ircut_type = 5                                        #1:HL 2:YY 3:QQ 4:HT 5:BQ(lark) 6:IR GUN 7:GQ 8:BQ(old version) 9:NO IR(using ae)\n\n");
	fprintf(fp, "ircut_mode = 1                                     # 0: ae 1:adc 2:ae_ex_ir_intensity\n");
	fprintf(fp, "ircut_have_irlight_intensity = 0            # 0: have irlight intensity 1: have no irlight intensity\n");
	fprintf(fp, "ircut_get_adc_mode = 1                     # 0: adc0 1:adc3 2:file 3:alarm in\n");
	fprintf(fp, "ircut_irlight_control_mode = 1            # 0: 0x83 0=open 1=close 1: 0x83 0=close 1=open\n");


	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			printf("input day to night cut lux value:\n");
		}
		else if( i == 1 )
		{
			printf("input night to day cut lux value:\n");
			enable_ircut = 0;
		}
		scanf( "%d", &light );

		if( light < 0 || light > 100 ) {
			printf("light %d is invalid , should 0~50\n", light);
			return -1;
		}
		sleep(2);
DO_AGAIN:

		enable_ircut = !!enable_ircut;
		printf("light %d, enable_ircut %d\n", light, enable_ircut);

		if (enable_ircut)
		{
			if (e_ENCODER_BOARD_LARK == g_EncoderBoardId){
				system("a5s_debug -g 83 -d 0x1");
			}else{
				system("a5s_debug -g 84 -d 0x1");
			}//endif if (e_ENCODER_BOARD_LARK == g_EncoderBoardId)
		}
		else
		{
			if (e_ENCODER_BOARD_LARK == g_EncoderBoardId){
				system("a5s_debug -g 83 -d 0x0");
			}else{
				system("a5s_debug -g 84 -d 0x0");
			}//endof if (e_ENCODER_BOARD_LARK == g_EncoderBoardId)
		}
		sleep(2);

		min = 0;
		max = 0;
		aver = 0;
		sum = 0;
		for (cnt = READ_TIMES; cnt > 0; ) {
			tmplight = GetLightMode();
			if (tmplight < 0) {
				printf("read ircut ADC failed, cnt %d\n", cnt);
				continue;
			}
			
			if(cnt == READ_TIMES)
				min = tmplight;

			if (tmplight > max)
				max = tmplight;
			else if (tmplight < min)
				min = tmplight;

			sum += tmplight;
			//fprintf(fp, "%d\n", tmplight);
			if (cnt%10 == 0)
				printf("cnt %d, tmplight %d, sum %d\n", cnt, tmplight, (int)sum);
			cnt--;
			usleep(sleep_time_us);
		}

		aver = sum /READ_TIMES;

		//fprintf(fp, "\ntimes	min		aver	max\n");
		//fprintf(fp, "%d		%d	 %d		%d\n", READ_TIMES, min, aver, max);
		fprintf(fp, "ircut_%d_lux_%d_min = %d\n", enable_ircut, light, min);
		fprintf(fp, "ircut_%d_lux_%d_max = %d\n", enable_ircut, light, max);
		fprintf(fp, "ircut_%d_lux_%d_aver = %d\n\n", enable_ircut, light, aver);

		printf("\nmin    max    aver    max-min\n");
		printf("%3d    %3d    %4d    %7d\n", min, max, aver, max - min);

		if (!enable_ircut) {
			adc_avg[i][0] = aver;
			enable_ircut = !enable_ircut;
			goto DO_AGAIN;
		}
		else{
			adc_avg[i][1] = aver;
		}
	}
	fclose(fp);

	if( adc_avg[1][0] - adc_avg[0][0]  < 20 )
	{
		printf("failed\n");
		return -1;
	}else if( adc_avg[0][1] - adc_avg[0][0] > 90 )
	{
		printf("failed\n");
		return -1;
	}else if( abs( adc_avg[1][1] - adc_avg[1][0] - ( adc_avg[0][1] - adc_avg[0][0] ) )  > 10  )
	{
		printf("failed\n");
		return -1;
	}
	else
	{
		printf("OK\n");
	}

	return 0;
}


