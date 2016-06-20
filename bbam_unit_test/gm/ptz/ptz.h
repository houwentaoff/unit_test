/*
 * ptz.h*
 * History:
 *	2011/7/20 - [George Chen] created file
 *
 * Copyright (C) 2007-2011, GM-Innovation, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of GM-Innovation, Inc.
 *
 */

#ifndef __PTZ_H__
#define __PTZ_H__

#define PACKED __attribute__((packed))

typedef struct {
	unsigned char sync;
	unsigned char addr;
	unsigned char command1;
	unsigned char command2;
	unsigned char data1;
	unsigned char data2;
	unsigned char checksum;	
}PACKED pelcoData;


typedef struct{
	unsigned char cmd1;
	unsigned char cmd2;
	unsigned char pan_speed;
	unsigned char tilt_speed;
	unsigned char addr;
}pelcoControl;


typedef struct{
	unsigned char xpos;
	unsigned char ascii;
}pelcod_scrchar;

typedef struct{
	unsigned char data1;
	unsigned char data2;
}cmdtype1;

typedef struct{
	unsigned char cmd1;
	unsigned char data1;
	unsigned char data2;

}cmdtype2;

typedef enum{
		PELCO_CMD_STOP = 0,

		PELCO_CMD_UP,
		PELCO_CMD_DOWN,
		PELCO_CMD_LEFT,
		PELCO_CMD_RIGHT,
		
		PELCO_CMD_INVALID,

} PELCO_CMD;

typedef struct{
	int port;
	int wlevel;
}pelcoGpioSetting;


int PelcoD_get_speed(int *pan_speed,int *tilt_speed);
int PelcoD_set_speed(int pan_speed,int tilt_speed);
int PelcoD_cmd(int cmd, void *param);
int PelcoD_init(int fd,int addr,int baudrate,int gpio_num,int gpio_wlevel);

#endif //  __PTZ_H__

