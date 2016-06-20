/***********************************************************
 * test_log.c
 *
 * History:
 *	2012/12/26 - [Yuan Lulu] created file
 *
 * Copyright (C) 2012-2014, Gmi, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ***********************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#include "ambas_imgproc.h"

#include "img_struct.h"
#include "img_api.h"

#include "mw_struct.h"
#include "mw_api.h"
#include "sqlite3.h"

static int log_test_get_cond(log_cond_t *cond)
{
	unsigned int type_comb = 0;
	char tmp[100] = {0};

	if (!cond) {
		printf("%s ERROR:NULL argc\n", __func__);
		return -1;
	}
	memset((void *)cond, 0, sizeof(log_cond_t));

	printf("\ndo you want to input type combine?(y/n)\n");
	gets(tmp);
	if (strcmp(tmp, "y") == 0) {
		printf("do you need type VISIT?(y/n)\n");
		memset(tmp, 0, sizeof(tmp));
		gets(tmp);
		if (strcmp(tmp, "y") == 0)
			type_comb |= MW_LOG_TYPE_VISIT;

		
		printf("do you need type ALARM?(y/n)\n");
		memset(tmp, 0, sizeof(tmp));
		gets(tmp);
		if (strcmp(tmp, "y") == 0)
			type_comb |= MW_LOG_TYPE_ALARM;

		printf("do you need type SYSTEM?(y/n)\n");
		memset(tmp, 0, sizeof(tmp));
		gets(tmp);
		if (strcmp(tmp, "y") == 0)
			type_comb |= MW_LOG_TYPE_SYSTEM;

		cond->type_comb = type_comb;
	}

	printf("\ndo you need time interval?(y/n)\n"); 
	memset(tmp, 0, sizeof(tmp));
	gets(tmp);
	if (strcmp(tmp, "y") == 0) {
		printf("input the interval(like 20121203 100011 20121226 151032 meanes 2012.12.03 10:00:11 ~ 2012.12.26 15:10:32)\n");
		gets(tmp);
		sscanf(tmp, "%d%d%d%d", &(cond->start_date), &(cond->start_time), &(cond->end_date), &(cond->end_time));
	}

	printf("\nwhen get rownum, you should not input limit\n");
	printf("do you need row limit?(y/n)\n"); 
	memset(tmp, 0, sizeof(tmp));
	gets(tmp);
	if (strcmp(tmp, "y") == 0) {
		printf("input the start_row and row_limit(like:10 15 will select the 10th ~ 25th log)\n");
		gets(tmp);
		sscanf(tmp, "%d%d", &(cond->start_row), &(cond->row_limit));
	}

	printf("log cond you input is:\ntype_comb 0x%x\nstart_date - start_time:%d-%d\nend_date - end_time:%d-%d\nstart_row %d, row_limit %d\n",
			cond->type_comb, cond->start_date, cond->start_time, cond->end_date, cond->end_time, cond->start_row, cond->row_limit);

	return 0;
}

static int log_test_get_number_all(void)
{
	int retv = 0;
	int rownum = 0;

	retv = mw_log_query_rownum(NULL, &rownum);
	if (retv != 0) {
		printf("mw_log_query_rownum failed\n");
		return -1;
	}

	printf("There are [%d] logs in file\n", rownum);
	return 0;
}

static int log_test_get_number_cond(void)
{
	int retv = 0;
	int rownum = 0;
	log_cond_t cond;

	log_test_get_cond(&cond);
	retv = mw_log_query_rownum(&cond, &rownum);
	if (retv != 0) {
		printf("mw_log_query_rownum failed\n");
		return -1;
	}

	printf("There are [%d] logs in file\n", rownum);

	return 0;
}

static int exec_callback_func(void *param, int column_count, char **column_values, char **column_names);
#if 1
static int log_test_show_cond(void)
{
	int retv = 0;
	log_cond_t cond;
	char file[100] = {0};
	FILE *fp = NULL;

	log_test_get_cond(&cond);

	printf("\ndo you want to store the result to a file?(y/n)\n");
	gets(file);
	if (strstr(file, "y")) {
		memset(file, 0, sizeof(file));
		printf("input the file you want to store result:\n");
		gets(file);
		fp = fopen(file, "a+");
		if (!fp) {
			printf("open %s with \"a+\" failed\n", file);
			return -1;
		}
	}

	retv = 	mw_log_query_callback(&cond, exec_callback_func, fp);
	if (retv != 0) {
		printf("mw_log_query_callback failed\n");
		return -1;
	}
	
	if (fp) {
		fclose(fp);
		printf("the result is in \"%s\"\n", file);	
	}
	printf("\nquery sucees!\n");
	return 0;
}
#else
static int log_test_show_cond(void)
{
	int retv = 0;
	log_cond_t cond;

	char **result = NULL;
	int row = 0;
	int column = 0;
	int i = 0; 
	int j = 0;


	log_test_get_cond(&cond);
	retv = 	mw_log_query_do(&cond, &result, &row, &column);
	if (retv != 0) {
		printf("mw_log_query_do failed\n");
		return -1;
	}

	printf("row %d, column %d\n", row, column);
	for (i = 0; i <= row; i ++) {
		for (j = 0; j < column; j++) {
			printf("%-15.64s	", result[i*column +j]);
		}
		printf("\n");
	}
	printf("row %d, column %d\n", row, column);
	
	mw_log_query_undo(&cond, &result);
	return 0;
}
#endif
static int exec_callback_func(void *para, int column_count, char **column_values, char **column_names);
static int log_test_show_all(void)
{
	int retv = 0;
#if 0
	char **result = NULL;
	int row = 0;
	int column = 0;
	int i = 0; 
	int j = 0;
	retv = 	mw_log_query_do(NULL, &result, &row, &column);
	if (retv != 0) {
		printf("mw_log_query_do failed\n");
		return -1;
	}
	
	printf("row %d, column %d\n", row, column);
	for (i = 0; i <= row; i ++) {
		for (j = 0; j < column; j++) {
			printf("%-15.64s	", result[i*column +j]);
		}
		printf("\n");
	}
	
	mw_log_query_undo(NULL, &result);
#endif

	printf("rowid	date	time	type	user	brief		detail\n");
	retv = mw_log_exec_callback("select rowid,* from GmiLog", exec_callback_func, NULL);
	if (retv != 0) {
		printf("%s:show all logs failed\n", __func__);
		return -1;
	}

	printf("%s succeed\n", __func__);
	return 0;
}

static int log_test_delete_cond(void)
{
	int retv = 0;
	log_cond_t cond;

	log_test_get_cond(&cond);
	retv = 	mw_log_delete(&cond);
	if (retv != 0) {
		printf("mw_log_delete_cond failed\n");
		return -1;
	}

	printf("%s succeed\n", __func__);
	return 0;
}

static int log_test_delete_id(void)
{
	long long id = 0;
	int retv = 0;
	char tmp[100] = {0};

	printf("input the id you want to delete\n");
	gets(tmp);
	sscanf(tmp, "%lld", &id);

	retv = mw_log_delete_id(id);
	if (retv != 0) {
		printf("mw_log_delete_id failed\n");
		return -1;
	}

	printf("delete log where rowid==%lld succeed\n", id);
	return 0;
}
static int log_test_delete_ids(void) 
{
	int retv = 0;
	char ids[150] = {0};

	printf("input the ids you want to delete(use \",\" to seprate them)\n");
	gets(ids);

	retv = mw_log_delete_ids(ids);
	if (retv != 0) {
		printf("mw_log_delete_ids failed\n");
		return -1;
	}

	printf("delete log where rowid in (%s) succeed\n", ids);
	return 0;
}

static int log_test_delete_all(void)
{
	int retv = 0;

	retv = mw_log_delete_all();
	if (retv != 0) {
		printf("mw_log_delete_all failed\n");
		return -1;
	}
	
	printf("delete all succeed!\n");
	return 0;
}

static int log_test_insert(void)
{
	char user[100] = {0};
	char brief[100] = {0};
	char detail[256] = {0};
	char buf[100] = {0};
	
	log_format_t log;
	unsigned int type = 0;
	int retv = 0;

	memset((void *)&log, 0, sizeof(log));

	printf("input a num to select the log type:\n");
	printf("visit:%d\nalarm:%d\nsystem:%d\n", MW_LOG_TYPE_VISIT, MW_LOG_TYPE_ALARM, MW_LOG_TYPE_SYSTEM);
	gets(buf);
	type = atoi(buf);
	log.type = type; 

	printf("do you ned to input date & time (y?)\n");
	gets(buf);
	if (strstr(buf, "y")) {
		printf("inoput date(exp 20120123):\n");
		gets(buf);
		log.date = atoi(buf);
		printf("inoput time(exp 150130 or 91312):\n");
		gets(buf);
		log.time = atoi(buf);
	}

	printf("input user name(just a single word)\n");
	gets(user);
	user[sizeof(user)-1] = 0;
	
	printf("input brief description(<64 chars)\n");
	gets(brief);
	brief[sizeof(brief)-1] = 0;
	
	printf("input detail description(<256 chars)\n");
	gets(detail);
	detail[sizeof(detail)-1] = 0;

	log.user = user;
	log.brief = brief;
	log.detail = detail;

	retv = mw_log_insert(&log);
	if (retv != 0) {
		printf("insert to log file failed\n");
		return -1;
	}
	
	printf("insert succeed! new log's rowid is %lld\n", log.rowid);
	return 0;
}

static int log_test_insert_batch(void)
{
	char tmp[20] = {0};
	char user[36] = {0};
	char brief[64] = {0};
	char detail[256] = {0};
	
	log_format_t log;
	int retv = 0;
	int batch_num = 0;
	int i = 0;
	int type = 0;

	memset((void *)&log, 0, sizeof(log));

	printf("how many logs do you want to insert? input the number:\n");
	gets(tmp);
	batch_num = atoi(tmp);

	printf("input the type(1/2/4):\n");
	gets(tmp);
	type = atoi(tmp);

	printf("input user name(just a single word)\n");
	gets(user);
	user[sizeof(user)-1] = 0;
	
	printf("input brief description(<64 chars)\n");
	gets(brief);
	brief[sizeof(brief)-1] = 0;
	
	printf("input detail description(<256 chars)\n");
	gets(detail);
	detail[sizeof(detail)-1] = 0;

	log.user = user;
	log.brief = brief;
	log.detail = detail;

	for (i = 0; i < batch_num; i++) {
		log.type = type;

		retv = mw_log_insert(&log);
		if (retv != 0) {
			printf("insert %dth log file failed\n", i);
			break;
		}
		printf(".");
		if (i && i%50==0)
			printf("\n");
	}

	printf("\n%d logs has been inserted\n", i);
	return 0;
}



static int log_test_insert_visit(void)
{
	char user[36] = {0};
	char brief[64] = {0};
	char detail[256] = {0};
	
	log_format_t log;
	int retv = 0;

	memset((char *)&log, 0, sizeof(log));
	log.type = MW_LOG_TYPE_VISIT; 

	printf("input user name(just a single word)\n");
	gets(user);
	user[sizeof(user)-1] = 0;

	printf("input brief description(<64 chars)\n");
	gets(brief);
	brief[sizeof(brief)-1] = 0;
	
	printf("input detail description(<256 chars)\n");
	gets(detail);
	detail[sizeof(detail)-1] = 0;

	log.user = user;
	log.brief = brief;
	log.detail = detail;

	retv = mw_log_insert(&log);
	if (retv != 0) {
		printf("insert to log file failed\n");
		return -1;
	}
	
	printf("insert succeed! new log's rowid is %lld\n", log.rowid);
	return 0;
}

static int log_test_insert_alarm(void)
{
	char user[36] = {0};
	char brief[64] = {0};
	char detail[256] = {0};
	
	log_format_t log;
	int retv = 0;

	memset((char *)&log, 0, sizeof(log));
	log.type = MW_LOG_TYPE_ALARM; 

	printf("input user name(just a single word)\n");
	gets(user);
	user[sizeof(user)-1] = 0;
	
	printf("input brief description(<64 chars)\n");
	gets(brief);
	brief[sizeof(brief)-1] = 0;
	
	printf("input detail description(<256 chars)\n");
	gets(detail);
	detail[sizeof(detail)-1] = 0;

	log.user = user;
	log.brief = brief;
	log.detail = detail;

	retv = mw_log_insert(&log);
	if (retv != 0) {
		printf("insert to log file failed\n");
		return -1;
	}
	
	printf("insert succeed! new log's rowid is %lld\n", log.rowid);
	return 0;
}

static int log_test_insert_system(void)
{
	char user[36] = {0};
	char brief[64] = {0};
	char detail[256] = {0};
	
	log_format_t log;
	int retv = 0;

	memset((char *)&log, 0, sizeof(log));
	log.type = MW_LOG_TYPE_SYSTEM; 

	printf("input user name(just a single word)\n");
	gets(user);
	user[sizeof(user)-1] = 0;
	
	printf("input brief description(<64 chars)\n");
	gets(brief);
	brief[sizeof(brief)-1] = 0;
	
	printf("input detail description(<256 chars)\n");
	gets(detail);
	detail[sizeof(detail)-1] = 0;

	log.user = user;
	log.brief = brief;
	log.detail = detail;

	retv = mw_log_insert(&log);
	if (retv != 0) {
		printf("insert to log file failed\n");
		return -1;
	}
	
	printf("insert succeed! new log's rowid is %lld\n", log.rowid);
	return 0;
}

static int log_test_exec_sql(void)
{
	char cmd[200] = {0};
	int retv = 0;

	printf("input the SQL cmd you want to exec:\n");
	fflush(stdin);
	gets(cmd);

	retv = mw_log_exec_sql(cmd);
	if (retv != 0) {
		printf("exec %s failed\n", cmd);
		return -1;
	}
	
	printf("exec succeed:%s\n", cmd);
	return 0;
}

static int exec_callback_func(void *para, int column_count, char **column_values, char **column_names)
{
	int i = 0;
	FILE *fp = (FILE*)para;

	if (!fp)
		fp = stdout;

	for (i = 0; i < column_count; i++) {
		fprintf(fp, "%-12.256s	", column_values[i]);	
	}
	fprintf(fp, "\n");

	return 0;
}

int log_test_exec_callback(void)
{
	int retv = 0;
	char cmd[200] = {0};

	printf("input your SQL command\n");
	gets(cmd);

	retv = mw_log_exec_callback(cmd, exec_callback_func, NULL);
	if (retv != 0) {
		printf("exec \"%s\" failed\n", cmd);
		return -1;
	}
	
	printf("exec succeed:%s\n", cmd);
	return 0;
}

static int log_test_create(void)
{
	int retv = 0;

	retv = mw_log_create();
	if (retv != 0) {
		printf("ERROR: create empty log file faiuled\n");
		return -1;
	}

	printf("create a empty log file\n");

	return 0;
}

static int log_test_adjust_size(void)
{
	mw_log_adjust_size();
	mw_log_defrag();
	return 0;
}

static int log_test_set_limit(void)
{
	
	log_limit_t limit;
	int retv = 0;
	char buf[100] = {0};

	retv = mw_log_get_limit(&limit);
	if (retv != 0) {
		printf("get limit failed\n");
		return -1;
	}
	printf("limit type:%d(0:number-limit, 1:days-limit), days-limit:%d, number-lim:%d\n", limit.type, limit.days, limit.number);

	printf("\ndo you wan't to set log limit(y to continue)?\n");
	gets(buf);
	if (!strstr(buf, "y")) {
		printf("exit!\n");
		return 0;
	}

	printf("input \"type days number\"\n");
	gets(buf);
	retv = sscanf(buf, "%d%d%d", &(limit.type), &(limit.days), &(limit.number));
	printf("Your input >>>>\nlimit type:%d, days-limit:%d, number-lim:%d\n", limit.type, limit.days, limit.number);

	retv = mw_log_set_limit(&limit);
	if (retv != 0) {
		printf("\nERROR:set limit failed!\n");
		return -1;
	}
		
	retv = mw_log_get_limit(&limit);
	if (retv != 0) {
		printf("get limit failed\n");
		return -1;
	}
	printf("read again>>>>\nlimit type:%d, days-limit:%d, number-lim:%d\n", limit.type, limit.days, limit.number);

	return 0;
}

static int print_info(void)
{
	int i = 0;
	char *info[] = {
		"0	Exit >>**<<\n\n"
		
		"1	Get the total number in log file",
		"2	Get the number of logs macth specific condition",
		"3	Show all the logs macth specific condition",
		"4	Show all the logs\n",
		
		"10	Delete logs macth speific condition",
		"11	Delete logs with specific id",
		"12	Delete all the log",
		"13	Delete logs with a string of ids (like \"2, 434, 5, ...\")\n",

		"20	Insert a log",
		"21	Insert a visit log",
		"22	Insert a alarm log",
		"23	Insert a system log",
		"24	Batch Insert logs\n",
		
		"30	exec a SQL command",
		"31	exec a SQL command and show the result\n",
		"35	Create a empty log file(the old will be rename)",
		"36	Adjust log file's size to a suitable size",
		"37	Get And Set limit config\n",
		NULL,
	};
	
	printf("==============================\n");
	printf("input a num to select what to do?\n");
	for (i = 0; info[i] != NULL; i++) {
		printf("%s\n", info[i]);
	}
	printf("------------------------------\n");
	
	return 0;
}

static int log_exec(unsigned int cmd) {
	switch (cmd) {
	case 1:
		log_test_get_number_all();
		break;
	case 2:
		log_test_get_number_cond();
		break;
	case 3:
		log_test_show_cond();
		break;
	case 4:
		log_test_show_all();
		break;
	case 10:
		log_test_delete_cond();
		break;
	case 11:
		log_test_delete_id();
		break;
	case 12:
		log_test_delete_all();
		break;
	case 13:
		log_test_delete_ids();
		break;
	case 20:
		log_test_insert();
		break;
	case 21:
		log_test_insert_visit();
		break;
	case 22:
		log_test_insert_alarm();
		break;
	case 23:
		log_test_insert_system();
		break;
	case 24:
		log_test_insert_batch();
		break;
	case 30:
		log_test_exec_sql();
		break;
	case 31:
		log_test_exec_callback();
		break;
	case 35:
		log_test_create();
		break;
	case 36:
		log_test_adjust_size();
		break;
	case 37:
		log_test_set_limit();
		break;
	default:
		printf("your cmd is undefined, input again!\n");
		break;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	unsigned int cmd = 0;
	char tmp[100] = {0};

	printf("Sqlite3 version:%s\n", (char *)sqlite3_libversion());
	while(1) {
		print_info();

		gets(tmp);
		cmd = atoi(tmp);

		if (cmd == 0) {
			printf("your input is 0, so exit\n");
			goto exit;	
		}

		log_exec(cmd);
	}

exit:
	return 0;
}


