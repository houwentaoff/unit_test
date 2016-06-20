/*
 * ptz_control.c
 *
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
 
#include "ptz.h"
#include "basetypes.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <linux/ioctl.h>	 

#include <getopt.h>
#include <sched.h>
#include <time.h>
#include <termios.h> 

#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>

//#define DEBUG_PELCO

#ifdef DEBUG_PELCO
#define dprintf printf
#else
#define dprintf(format, args...)
#endif

static int fd_ttys;
static pelcoGpioSetting gpioSetting;
static pelcoControl ctrlData;

#define FILL_PELCO_CMD(pelco,cmd1,cmd2,_data1,_data2) \
	do { \
		(pelco).sync = 0xFF;\
		(pelco).addr =	ctrlData.addr;\
		(pelco).command1 = cmd1; \
		(pelco).command2 = cmd2; \
		(pelco).data1 = _data1; \
		(pelco).data2 = _data2; \
		(pelco).checksum = calc_check(&pelco);\
	} while (0)

/*
static int range(u8 i, u8 min, u8 max)
{
	if(i < min || i > max)
		return 0;
	else
		return 1;
}
*/
static u8 calc_check(pelcoData *data)
{
	u8 sum = data->addr;
		sum += data->command1;
		sum += data->command2;
		sum += data->data1;
		sum += data->data2;

	dprintf("\r\n check sum is 0x%x,",sum);	
	return sum;
}

static int set_uart_writable()
{
	char cmd[64];
	sprintf(cmd,"/usr/local/bin/a5s_debug -g %d -d 0x%x",gpioSetting.port,gpioSetting.wlevel);
	dprintf("cmd=%s\n",cmd);
	system(cmd);
	usleep(10000);

return 0;
}
/*
static int set_uart_readable()
{
	char cmd[64];
	int level = gpioSetting.wlevel ? 0:1;
	sprintf(cmd,"/usr/local/bin/a5s_debug -g %d -d 0x%x",gpioSetting.port,level);
	dprintf("cmd=%s\n",cmd);
	system(cmd);
	usleep(1000000);

return 0;
}
*/
static int uart_write(pelcoData *data)
{
	int n = 0;
	if(data != NULL)
	{
		dprintf("r\n sizeof data is %d",sizeof(pelcoData));
		// 1:gpio is high to enable writable
		set_uart_writable();
		// 2:write uart
		n = write(fd_ttys, data, sizeof(pelcoData));
		// 3.reset uart to read state
	//	set_uart_readable();
	}else	{
		printf("\r\n pelcoD write error!");
	}

	return n;
}

static int pelco_cmd_stop()
{
		pelcoData pel_command;

		pel_command.sync = 0xFF;
		pel_command.addr =  ctrlData.addr;
		
		pel_command.command1 = 0x00;
		pel_command.command2 = 0x00;
		
		pel_command.data1 = 0x00;
		pel_command.data2 = 0x00;
		
		pel_command.checksum = calc_check(&pel_command);
		uart_write(&pel_command);
			
		return 0;
}

static int pelco_cmd_up()
{
	pelcoData pel_command;
	pel_command.sync = 0xFF;

	pel_command.addr =  ctrlData.addr;
	
	pel_command.command1 = ctrlData.cmd1;
	pel_command.command2 = ctrlData.cmd2;
	pel_command.command2 &= 0x01; //we need to clean other direction commands!
	pel_command.command2 |= 0x10;
	ctrlData.cmd2 = pel_command.command2;
	
	pel_command.data1 = ctrlData.pan_speed;
	pel_command.data2 = ctrlData.tilt_speed;
	pel_command.checksum = calc_check(&pel_command);
	uart_write(&pel_command);
	
	return 0;

}

static int pelco_cmd_down()
{
	pelcoData pel_command;
	pel_command.sync = 0xFF;

	pel_command.addr =  ctrlData.addr;
	
	pel_command.command1 = ctrlData.cmd1;
	pel_command.command2 = ctrlData.cmd2;
	pel_command.command2 &= 0x01; //we need to clean other direction commands!
	pel_command.command2 |= 0x08;
	ctrlData.cmd2 = pel_command.command2;
	
	pel_command.data1 = ctrlData.pan_speed;
	pel_command.data2 = ctrlData.tilt_speed;

	pel_command.checksum = calc_check(&pel_command);
	uart_write(&pel_command);

return 0;

}

static int pelco_cmd_left()
{
	pelcoData pel_command;
	pel_command.sync = 0xFF;

	pel_command.addr =  ctrlData.addr;
	
	pel_command.command1 = ctrlData.cmd1;
	pel_command.command2 = ctrlData.cmd2;
	pel_command.command2 &= 0x01; //we need to clean other direction commands!
	pel_command.command2 |= 0x04;
	ctrlData.cmd2 = pel_command.command2;
	
	pel_command.data1 = ctrlData.pan_speed;
	pel_command.data2 = ctrlData.tilt_speed;

	pel_command.checksum = calc_check(&pel_command);
	uart_write(&pel_command);

	return 0;
}

static int pelco_cmd_right()
{
	pelcoData pel_command;
	pel_command.sync = 0xFF;

	pel_command.addr =  ctrlData.addr;
	
	pel_command.command1 = ctrlData.cmd1;
	pel_command.command2 = ctrlData.cmd2;
	pel_command.command2 &= 0x01; //we need to clean other direction commands!
	pel_command.command2 |= 0x02;
	ctrlData.cmd2 = pel_command.command2;
	
	pel_command.data1 = ctrlData.pan_speed;
	pel_command.data2 = ctrlData.tilt_speed;

	pel_command.checksum = calc_check(&pel_command);
	uart_write(&pel_command);

return 0;

}

int getbaud(int fd) {
	struct termios termAttr;
	int inputSpeed = -1;
	speed_t baudRate;
	tcgetattr(fd, &termAttr);


	/* Get the input speed.                              */
	baudRate = cfgetispeed(&termAttr);
	switch (baudRate) {
		case B0:      inputSpeed = 0; break;
		case B50:     inputSpeed = 50; break;
		case B110:    inputSpeed = 110; break;
		case B134:    inputSpeed = 134; break;
		case B150:    inputSpeed = 150; break;
		case B200:    inputSpeed = 200; break;
		case B300:    inputSpeed = 300; break;
		case B600:    inputSpeed = 600; break;
		case B1200:   inputSpeed = 1200; break;
		case B1800:   inputSpeed = 1800; break;
		case B2400:   inputSpeed = 2400; break;
		case B4800:   inputSpeed = 4800; break;
		case B9600:   inputSpeed = 9600; break;
		case B19200:  inputSpeed = 19200; break;
		case B38400:  inputSpeed = 38400; break;
		case B115200: inputSpeed = 115200; break;
	}
	return inputSpeed;
}

speed_t baud_trans(int baudrate)
{
	switch (baudrate) {
		case 0: return B0;
		case 2400: return B2400;
		case 4800: return B4800;
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 115200: return B115200;
		default: return B0;
	}
}

int PelcoD_get_speed(int *pan_speed,int *tilt_speed)
{
	*pan_speed = ctrlData.pan_speed;
	*tilt_speed = ctrlData.tilt_speed;

return 0;
}

int PelcoD_set_speed(int pan_speed,int tilt_speed)
{
	int ret = 0;
	u8 pan =pan_speed;
	u8 tilt = tilt_speed;
	
	if(pan > 0 && pan < 0x3f)
		ctrlData.pan_speed = pan;
	else if(pan == 0xFF) //turbo
		ctrlData.pan_speed = pan;
	else{
		printf("r\n PelcoD_set_param err!");
		return -1;
	}

	if(tilt > 0 && tilt < 0x3f)
		ctrlData.tilt_speed = tilt;
	else if(pan == 0xff) //turbo
		ctrlData.tilt_speed = tilt;
	else{
		printf("r\n PelcoD_set_param err!");
		return -1;
	}

	return ret;
	
}

int PelcoD_init(int fd,int addr,int baudrate,int gpio_num,int gpio_wlevel)
{
	struct termios Opt;
	int i;
	
	fd_ttys = fd;

	speed_t obaud = baud_trans(baudrate);
	speed_t ibaud = baud_trans(baudrate);// B115200;
	
	tcgetattr(fd, &Opt);// Get the current options for the port
	dprintf("cur uart setting:c_iflag=0x%x,c_oflag=0x%x,c_cflag=0x%x,c_lflag=0x%x,c_line=%c\n",
		Opt.c_iflag,Opt.c_oflag,Opt.c_cflag,Opt.c_lflag,Opt.c_line);
	for (i=0;i<8;i++){
		dprintf("cur uart setting:c_cc=%c\n",Opt.c_cc[i]);
	}
	int fd_uart0 = open("/dev/ttyS0", O_RDWR, 0);
	if (fd_uart0 == -1) {
		perror("open uart port: Unable to open /dev/ttyS0 - ");
		return -1;
	} else {
		fcntl(fd_uart0, F_SETFL, 0);
	}
	tcgetattr(fd_uart0, &Opt);// Get the current options for the port
	close(fd_uart0);
	dprintf("cur uart0 setting:c_iflag=0x%x,c_oflag=0x%x,c_cflag=0x%x,c_lflag=0x%x,c_line=%c\n",
		Opt.c_iflag,Opt.c_oflag,Opt.c_cflag,Opt.c_lflag,Opt.c_line);
	for (i=0;i<8;i++){
		dprintf("cur uart0 setting:c_cc=%c\n",Opt.c_cc[i]);
	}
	cfsetispeed(&Opt,ibaud); 
	cfsetospeed(&Opt,obaud);
	if (tcsetattr(fd,TCSAFLUSH,&Opt) != 0)
		perror("set  uart error\n");
	
	dprintf("baud=%d\n", getbaud(fd));
	dprintf("cur uart setting:c_iflag=0x%x,c_oflag=0x%x,c_cflag=0x%x,c_lflag=0x%x,c_line=%c\n",
		Opt.c_iflag,Opt.c_oflag,Opt.c_cflag,Opt.c_lflag,Opt.c_line);
	for (i=0;i<8;i++){
		dprintf("new uart setting:c_cc=%c\n",Opt.c_cc[i]);
	}
	gpioSetting.port = gpio_num;
	gpioSetting.wlevel = gpio_wlevel;

	memset(&ctrlData,0,sizeof(pelcoControl));
	ctrlData.pan_speed = 0x30;
	ctrlData.tilt_speed = 0x30;
	ctrlData.addr = addr;

	return 0;
}

int PelcoD_cmd(int cmd, void *param)
{
	int ret = 0;
	switch(cmd){
		case PELCO_CMD_STOP:
			pelco_cmd_stop();
			break;	
		case PELCO_CMD_UP:
			pelco_cmd_up();
			break;
		case PELCO_CMD_DOWN:
			pelco_cmd_down();
			break;
		case PELCO_CMD_LEFT:
			pelco_cmd_left();
			break;
		case PELCO_CMD_RIGHT:
			pelco_cmd_right();
			break;
		default:
			return -1;
		}

		return ret;	
}


