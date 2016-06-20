#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>

#include <getopt.h>
#include <sched.h>
#include <errno.h>
#include <basetypes.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/signal.h>

#include "iav_drv.h"
#include "ambas_common.h"
#include "ambas_vin.h"

#include "mw_hw_check.h"

char *sensor_name[] = {
	NULL,
	"mn34041pl",
	"imx122",
	"ov2715",
	"ov9715",
	"tw9910",
	NULL,		
};

char *cpu_name[] = {
	NULL,
	"A5s88",
	"A5s66",
	"A5s55",
	"A5s33",
	NULL,
};

char *shield_name[] = {
	NULL,
	"TianJin_HuiXun",
	"ChangZhou_HongBen",
	"ShenZhen_JiuSong",
	"HangZhou_JinYang",
	NULL,
};

char *alarm_type[] = {
	NULL,
	"Native_GPIO",
	"I2C_GPIO",
	"Serial GPIO",
	NULL,
};

char *encboard[] = {
	NULL,
	"Normal_Board",
	"Lark_Board",
	NULL,
};

char *zoom_lens[] = {
	NULL,
	"Lens_DF003",
	NULL,
};

int main(int argc, char *argv[])
{
	unsigned int type = 0;
	if (0 == mw_hw_get_sensor_type(&type)) 
		printf("sensor type:%s\n", sensor_name[type]);
	else
		printf("check sensor failed\n");

	type = 0;
	if (0 == mw_hw_get_cpu_type(&type))
		printf("cpu type:%s\n", cpu_name[type]);
	else
		printf("check cpu failed\n");

	type = 0;
	if (0 == mw_hw_get_shield_type(&type))
		printf("shield type:%s\n", shield_name[type]);
	else
		printf("check shield failed\n");

	type = 0;
	if (0 == mw_hw_get_alarm_port_type(&type))
		printf("alarm port type:%s\n", alarm_type[type]);
	else
		printf("check alarm port failed\n");

	type = 0;
	if (0 == mw_hw_get_encoder_board_type(&type))
		printf("encode board type:%s\n", encboard[type]);
	else
		printf("check encode board failed\n");

	type = 0;
	if (0 == mw_hw_get_zoomlens_type(&type))
		printf("zoom lens type:%s\n", zoom_lens[type]);
	else
		printf("check zoom lens failed\n");

	return 0;
}

