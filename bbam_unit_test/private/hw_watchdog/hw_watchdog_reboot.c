/******************************************************************************
modules		:    BoardWrapper
name		:     gmi_led_u.c
function	:    LED Device application interface  for application
author		:    minchao.wang
version		:    1.0.0.0.0
---------------------------------------------------------------
modification	:
	date		version		 author 	modification
	1/21/2013	1.0.0.0.0     minchao.wang         establish
******************************************************************************/
#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "gmi_led_u.h"

#define GMI_LOG_FILE ("/root/hw_wd_reboot.log")
#define GMI_LOG_MAX_SIZE  (1<<20) //1M

static unsigned int get_file_size(const char *filename)
{
	struct stat buf;
	if(stat(filename, &buf)<0)
	{
		return 0;
	}
	return (unsigned int)buf.st_size;
}

static void write_syslog(char *fmt,...)
{
	va_list argptr;
	//int cnt = 0;
	char buffer[1024] = {0};
	FILE *fp = NULL;
	time_t			t = time(NULL);
	struct tm		timet;

	if (get_file_size(GMI_LOG_FILE) < GMI_LOG_MAX_SIZE)
	{
		//append log to the end of file
		fp = fopen(GMI_LOG_FILE, "a+" );
	}
	else
	{
		//create a new file to write log
		fp = fopen(GMI_LOG_FILE, "wb" );
	}

	if(fp == NULL ) 
	{
		printf("fopen error\n");         
		return ;
	} 

	//write time info
	memset(buffer, 0, sizeof(buffer));
	localtime_r(&t, &timet);
	snprintf(buffer, sizeof(buffer) - 1, "%04d-%02d-%02d %02d:%02d:%02d\n", timet.tm_year + 1900, timet.tm_mon + 1, timet.tm_mday, timet.tm_hour, timet.tm_min, timet.tm_sec);
	fwrite(buffer, strlen(buffer), 1, fp);

	//write log msg
	memset(buffer, 0, sizeof(buffer));
	va_start(argptr, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, argptr);
	va_end(argptr);
	fwrite(buffer, strlen(buffer), 1, fp);
	fflush(fp);
	fclose(fp);
	fp = NULL;
}


int main(void)
{
	int ret = 0;
	write_syslog((char*)"GMI_LedWdReboot\n");
	sync();
	ret = GMI_LedWdReboot();
	if(ret){
		write_syslog((char*)"GMI_LedWdReboot failed!!!\n");
		return -1;
	}
	sleep(1);
	return 0;
}
