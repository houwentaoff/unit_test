/***********************************************************
 * test_cgi_log.c
 *
 * History:
 *	2012/12/31 - [Yuan Lulu] created file
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

#define MW_ERROR  printf

static int cgi_log_getcond_from_str(const char *cond_str, log_cond_t *cond)
{
	if (!cond_str || !cond) {
		MW_ERROR("invalid BULL ponter!\n");
		return -1;
	}
	log_cond_t cond_l;
	memset((char *)&cond_l, 0, sizeof(log_cond_t));

	char *type_comb = "type_comb";
	char *start_date = "start_date";
	char *start_time = "start_time";
	char *end_date = "end_date";
	char *end_time = "end_time";
	char *start_row = "start_row";
	char *row_limit = "row_limit";

	char *namep = NULL;
	char *valuep = NULL;
	int retv = 0;

	namep = strstr(cond_str, type_comb);
	if (!namep) {
		MW_ERROR("can't find %s in \"%s\"\n", type_comb, cond_str);
		return -1;
	}
	valuep = strstr(namep, "=");
	if (!valuep) {
		MW_ERROR("can't find \'=\' in \"%s\"\n", cond_str);
		return -1;
	}
	valuep++;
	retv = sscanf(valuep, "%d", &(cond_l.type_comb));
	if (retv != 1) {
		MW_ERROR("sscanf \'%s\' from \"%s\" failed\n", type_comb, cond_str);
		return -1;	
	}

	namep = valuep = NULL;
	namep = strstr(cond_str, start_date);
	if (!namep) {
		MW_ERROR("can't find %s in \"%s\"\n", start_date, cond_str);
		return -1;
	}
	valuep = strstr(namep, "=");
	if (!valuep) {
		MW_ERROR("can't find \'=\' in \"%s\"\n", cond_str);
		return -1;
	}
	valuep++;
	retv = sscanf(valuep, "%d", &cond_l.start_date);
	if (retv != 1) {
		MW_ERROR("sscanf \'%s\' from \"%s\" failed\n", start_date, cond_str);
		return -1;	
	}

	namep = valuep = NULL;
	namep = strstr(cond_str, start_time);
	if (!namep) {
		MW_ERROR("can't find %s in \"%s\"\n", start_time, cond_str);
		return -1;
	}
	valuep = strstr(namep, "=");
	if (!valuep) {
		MW_ERROR("can't find \'=\' in \"%s\"\n", cond_str);
		return -1;
	}
	valuep++;
	retv = sscanf(valuep, "%d", &cond_l.start_time);
	if (retv != 1) {
		MW_ERROR("sscanf \'%s\' from \"%s\" failed\n", start_time, cond_str);
		return -1;	
	}

	namep = valuep = NULL;
	namep = strstr(cond_str, end_date);
	if (!namep) {
		MW_ERROR("can't find %s in \"%s\"\n", end_date, cond_str);
		return -1;
	}
	valuep = strstr(namep, "=");
	if (!valuep) {
		MW_ERROR("can't find \'=\' in \"%s\"\n", cond_str);
		return -1;
	}
	valuep++;
	retv = sscanf(valuep, "%d", &cond_l.end_date);
	if (retv != 1) {
		MW_ERROR("sscanf \'%s\' from \"%s\" failed\n", end_date, cond_str);
		return -1;	
	}

	namep = valuep = NULL;
	namep = strstr(cond_str, end_time);
	if (!namep) {
		MW_ERROR("can't find %s in \"%s\"\n", end_time, cond_str);
		return -1;
	}
	valuep = strstr(namep, "=");
	if (!valuep) {
		MW_ERROR("can't find \'=\' in \"%s\"\n", cond_str);
		return -1;
	}
	valuep++;
	retv = sscanf(valuep, "%d", &cond_l.end_time);
	if (retv != 1) {
		MW_ERROR("sscanf \'%s\' from \"%s\" failed\n", end_time, cond_str);
		return -1;	
	}

	namep = valuep = NULL;
	namep = strstr(cond_str, start_row);
	if (!namep) {
		MW_ERROR("can't find %s in \"%s\"\n", start_row, cond_str);
		return -1;
	}
	valuep = strstr(namep, "=");
	if (!valuep) {
		MW_ERROR("can't find \'=\' in \"%s\"\n", cond_str);
		return -1;
	}
	valuep++;
	retv = sscanf(valuep, "%d", &cond_l.start_row);
	if (retv != 1) {
		MW_ERROR("sscanf \'%s\' from \"%s\" failed\n", start_row, cond_str);
		return -1;	
	}

	namep = valuep = NULL;
	namep = strstr(cond_str, row_limit);
	if (!namep) {
		MW_ERROR("can't find %s in \"%s\"\n", row_limit, cond_str);
		return -1;
	}
	valuep = strstr(namep, "=");
	if (!valuep) {
		MW_ERROR("can't find \'=\' in \"%s\"\n", cond_str);
		return -1;
	}
	valuep++;
	retv = sscanf(valuep, "%d", &cond_l.row_limit);
	if (retv != 1) {
		MW_ERROR("sscanf \'%s\' from \"%s\" failed\n", row_limit, cond_str);
		return -1;	
	}

	memcpy((char *)cond, (char *)&cond_l, sizeof(log_cond_t));
	return 0;
}

/* 
	input: type_comb = xxx\n start_date = xxx\n start_time = xxx\n end_date = xxx\n end_time = xxx\n start_row = xxx\n row_limit = xxx\n 
*/
static int cgi_log_query_rownum(char *cond_str, const char *secname) 
{
	log_cond_t cond;
	int retv = 0;
	int rownum = 0;

	if (!cond_str || !secname) {
		printf("get&no_secname&-1&invalid param:secname/cond_str is NULL\n");
		return -1;	
	}

	retv = cgi_log_getcond_from_str(cond_str, &cond);
	if (retv != 0) {
		printf("get&%s&-1&getcond_from_str failed\n", secname);
		return -1;
	}
	
	retv = mw_log_query_rownum(&cond, &rownum);
	if (retv != 0) {
		printf("get&%s&-1&mw_log_query_rownum failed\n", secname);
		return -1;	
	}

	printf("get&%s&0&{\"rownum\":\"%d\"}\n", secname, rownum);

	return 0;
}

static int cgi_log_query(char *cond_str, const char *secname) 
{
	log_cond_t cond;
	int retv = 0;
	char **result = NULL;
	int row = 0;
	int column  = 0;
	int i = 0;
	int j = 0;

	if (!cond_str || !secname) {
		printf("get&no_secname&-1&invalid param:secname/cond_str is NULL\n");
		return -1;	
	}

	retv = cgi_log_getcond_from_str(cond_str, &cond);
	if (retv != 0) {
		printf("get&%s&-1&getcond_from_str failed\n", secname);
		return -1;
	}
	
	retv = mw_log_query_do(&cond, &result, &row, &column);
	if (retv != 0) {
		printf("get&%s&-1&mw_log_query_do failed\n", secname);
		return -1;	
	}

	printf("get&%s&0&[", secname);
	for (i = 1; i <= row; i ++) {
		if (i > 1) 
			printf(", ");
		printf("{");
		for (j = 0; j < column; j++) {
			if (j > 0)
				printf(", ");
			printf("\"%s\":\"%s\"", result[j], result[i*column + j]);
		}
		printf("}");
	}
	printf("]\n");

	mw_log_query_undo(&cond, &result);
	return 0;
}

static int cgi_log_print_cb(void *para, int column, char  **values, char **names)
{
	int i = 0;
	char **sep = (char **)para;

	printf("%s ", *sep);
	*sep = (char *)",";

	printf("{");
	for (i = 0; i < column; i++) {
		if (i > 0) 
			printf(", ");
		printf("\"%s\":\"%s\"", names[i], values[i]);
	}
	printf("}");

	return 0;
}

int cgi_log_query_callback(const char *cond_str, const char *secname)
{
	log_cond_t cond;
	int retv = 0;
	char *sep = (char *)"0&[";
	char *fail = (char *)"-1&";

	if (!cond_str || !secname) {
		printf("get&no_secname&-1&invalid param:secname/cond_str is NULL\n");
		return -1;	
	}

	retv = cgi_log_getcond_from_str(cond_str, &cond);
	if (retv != 0) {
		printf("get&%s&-1&getcond_from_str failed\n", secname);
		return -1;
	}

	printf("get&%s&", secname);
	retv = mw_log_query_callback(&cond, cgi_log_print_cb, &sep);
	if (retv != 0) {
		printf("%smw_log_query_callback failed\n", fail);
		return -1;	
	}
	printf("]\n");

	return 0;
}

static int cgi_log_delete(char *cond_str, const char *secname) 
{
	log_cond_t cond;
	int retv = 0;

	if (!cond_str || !secname) {
		printf("set&no_secname&-1&invalid param:secname/cond_str is NULL\n");
		return -1;	
	}

	retv = cgi_log_getcond_from_str(cond_str, &cond);
	if (retv != 0) {
		printf("set&%s&-1&getcond_from_str failed\n", secname);
		return -1;
	}
	
	retv = mw_log_delete(&cond);
	if (retv != 0) {
		printf("set&%s&-1&mw_log_delete failed\n", secname);
		return -1;	
	}

	printf("set&%s&0&delete suceess\n", secname);

	return 0;
}

/* input:"xxx" */
static int cgi_log_delete_id(char *id, const char *secname) 
{
	int retv = 0;
	long long rowid = 0;

	if  (!id || !secname) {
		printf("set&no_secname&-1&invalid param:secname/id is NULL\n");
		return -1;	
	}
	
	retv = sscanf(id, "%lld", &rowid);
	if (retv != 1) {
		printf("set&%s&-1&sscanf rowid from \"%s\"failed\n", secname, id);
		return -1;		
	}

	retv = mw_log_delete_id(rowid);
	if (retv != 0) {
		printf("set&%s&-1&delete log(rowid=%lld)failed\n", secname, rowid);
		return -1;			
	}

	printf("set&%s&0&delete log(rowid=%lld)suceess\n", secname, rowid);
	return 0;
}

/* input:"xx, xx, xx, ..." */
static int cgi_log_delete_ids(char *ids, const char *secname) 
{
	int retv = 0;

	if  (!ids || !secname) {
		printf("set&no_secname&-1&invalid param:secname/ids is NULL\n");
		return -1;	
	}
	
	retv = mw_log_delete_ids(ids);
	if (retv != 0) {
		printf("set&%s&-1&delete log(ids:%s)failed\n", secname, ids);
		return -1;			
	}

	printf("set&%s&0&delete log(ids:%s)suceess\n", secname, ids);
	
	return 0;
}

static int cgi_log_delete_all(const char *secname) 
{
	int retv = 0;

	if  (!secname) {
		printf("set&no_secname&-1&invalid param:secname is NULL\n");
		return -1;	
	}
	
	retv = mw_log_delete_all();
	if (retv != 0) {
		printf("set&%s&-1&delete all log failed\n", secname);
		return -1;			
	}

	printf("set&%s&0&delete all log suceess\n", secname);
	
	return 0;
}

static int cgi_log_adjust_size(const char *secname)
{
	int retv = 0;

	if  (!secname) {
		printf("set&no_secname&-1&invalid param:secname is NULL\n");
		return -1;	
	}
	
	retv = mw_log_adjust_size();
	if (retv != 0) {
		printf("set&%s&-1&adjust size failed\n", secname);
		return -1;			
	}

	printf("set&%s&0&adjust size suceess\n", secname);
	
	return 0;
}

/* user_info: "user = xxxx\nbrief = xxxx\ndetail = xxxx\n */
static int cgi_log_analyse_log(const char *infostr, const char * valname, char **valpp, const char *secname)
{
	if (!infostr || !valname || !valpp) {
		printf("set&%s&-1&cgi_log_analyse_log:invalid NULL pointer\n", secname ? secname : "unknown secname");
		return -1;
	}

	char *str_head = NULL;
	char *valp = NULL;


	str_head = strstr(infostr, valname);
	if (!str_head) { 
		printf("set&%s&-1&cgi_log_analyse_log:can't find \"%s\" in \"%s\"\n", secname, valname, infostr);
		return -1;
	}
	valp = strstr(str_head, "=");
	if (!valp) {
		printf("set&%s&-1&cgi_log_analyse_log:can't find \"=\" in \"%s\"\n", secname, infostr);
		return -1;
	}
	valp ++;	
	while (isspace(*valp))
		valp ++;	

	*valpp = valp;
	return 0;
}

int cgi_log_login_insert(const char * user_info, const char *secname)
{
	int retv = 0;
	char *user = NULL;
	char *brief = NULL;
	char *detail = NULL;

	if (!secname) {
		printf("set&no_secname&-1&invalid param:secname is NULL\n");
		return -1;	
	}
	
	if (cgi_log_analyse_log(user_info, "user", &user, secname) ||
			cgi_log_analyse_log(user_info, "brief", &brief, secname) ||
			cgi_log_analyse_log(user_info, "detail", &detail, secname))
		return -1;
	
	int i = 0;
	char *tmp[4] = {NULL};
	for (i = 0; i < 4; i++) {
		tmp[i] = strstr(user_info, "\n");
		if (tmp[i])
			*tmp[i] = ' ';
	}
	for (i = 0; i < 4; i++) {
		if (tmp[i])
			*tmp[i] = '\0';
	}

	retv = mw_log_insert_visit(user, brief, detail);
	if (retv != 0) {
		printf("set&%s&-1&mw_log_insert_log failed: %s %s %s \n", secname, user, brief, detail);
		return -1;
	}
	
	return 0;
}

/************************************************/
static char *log_test_print_cond_do(log_cond_t *cond)
{
	char *cond_str = NULL;
	char *cmd_f = "type_comb = %d\nstart_date = %d\nstart_time = %d\nend_date = %d\nend_time = %d\nstart_row = %d\n row_limit = %d\n";

	if (!cond) {
		printf("%s:invalid NULL pointer\n", __func__);
		return NULL;
	}

	cond_str = sqlite3_mprintf(cmd_f, cond->type_comb, cond->start_date, cond->start_time, cond->end_date,
			cond->end_time, cond->start_row, cond->row_limit);

	return cond_str;
}

static int log_test_print_cond_undo(char *cond_str) 
{
	if (cond_str)
		sqlite3_free(cond_str);	
	return 0;
}

/************************************************/

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
	log_cond_t cond;
	char *cond_str = NULL;

	log_test_get_cond(&cond);
	cond_str = log_test_print_cond_do(&cond);
	if (!cond_str) {
		printf("error:cond_str is NULL\n");
		return -1;
	}

	retv = cgi_log_query_rownum(cond_str, "GetLog");
	if (retv != 0) {
		printf("mw_log_query_rownum failed\n");
		log_test_print_cond_undo(cond_str);
		return -1;
	}

	log_test_print_cond_undo(cond_str);

	return 0;
}

static int log_test_show_cond(void)
{
	int retv = 0;
	log_cond_t cond;
	char *cond_str = NULL;

	log_test_get_cond(&cond);
	cond_str = log_test_print_cond_do(&cond);
	if (!cond_str) {
		printf("error:cond_str is NULL\n");
		return -1;
	}

	//retv = 	cgi_log_query(cond_str, "GetQuery");
	retv = 	cgi_log_query_callback(cond_str, (char *)"GetQuery");
	if (retv != 0) {
		log_test_print_cond_undo(cond_str);
		printf("mw_log_query_do failed\n");
		return -1;
	}

	log_test_print_cond_undo(cond_str);
	return 0;
}

static int log_test_show_all(void)
{
	int retv = 0;
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
	printf("%s succeed\n", __func__);
	return 0;
}

static int log_test_delete_cond(void)
{
	int retv = 0;
	log_cond_t cond;
	char *cond_str = NULL;

	log_test_get_cond(&cond);
	cond_str = log_test_print_cond_do(&cond);
	if (!cond_str) {
		printf("error:cond_str is NULL\n");
		return -1;
	}

	retv = 	cgi_log_delete(cond_str, "SetDelete");
	if (retv != 0) {
		printf("mw_log_delete_cond failed\n");
		log_test_print_cond_undo(cond_str);
		return -1;
	}

	log_test_print_cond_undo(cond_str);
	return 0;
}

static int log_test_delete_id(void)
{
	int retv = 0;
	char tmp[100] = {0};

	printf("input the id you want to delete\n");
	gets(tmp);

	retv = cgi_log_delete_id(tmp, "DeleteId");
	if (retv != 0) {
		printf("mw_log_delete_id failed\n");
		return -1;
	}

	return 0;
}
static int log_test_delete_ids(void) 
{
	int retv = 0;
	char ids[150] = {0};

	printf("input the ids you want to delete(use \",\" to seprate them)\n");
	gets(ids);

	retv = cgi_log_delete_ids(ids, "DeleteIds");
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

	retv = cgi_log_delete_all("DeleteALL");
	if (retv != 0) {
		printf("mw_log_delete_all failed\n");
		return -1;
	}
	
	printf("delete all succeed!\n");
	return 0;
}

static int log_test_insert(void)
{
	char user[36] = {0};
	char brief[64] = {0};
	char detail[256] = {0};
	
	log_format_t log;
	unsigned int type = 0;
	int retv = 0;

	memset((void *)&log, 0, sizeof(log));

	printf("input a num to select the log type:\n");
	printf("visit:%d\nalarm:%d\nsystem:%d\n", MW_LOG_TYPE_VISIT, MW_LOG_TYPE_ALARM, MW_LOG_TYPE_SYSTEM);
	gets(user);
	type = atoi(user);
	log.type = type; 

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

	memset((void *)&log, 0, sizeof(log));

	printf("how many logs do you want to insert? input the number:\n");
	gets(tmp);
	batch_num = atoi(tmp);

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
		if (i%4 == 0)
			log.type = 4;
		else if (i%2 == 0)
			log.type = 2;
		else
			log.type = 1;

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
	char log_str[512] = {0};
	int retv = 0;

	printf("input user name(just a single word)\n");
	gets(user);
	user[sizeof(user)-1] = 0;

	printf("input brief description(<64 chars)\n");
	gets(brief);
	brief[sizeof(brief)-1] = 0;
	
	printf("input detail description(<256 chars)\n");
	gets(detail);
	detail[sizeof(detail)-1] = 0;

	sprintf(log_str, "user = %s\nbrief = %s\ndetail = %s\n", user, brief, detail);

	retv = cgi_log_login_insert(log_str, "SET_MW_LOG");
	if (retv != 0) {
		printf("insert to log file failed\n");
		return -1;
	}
	
	printf("insert succeed!\n");
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

	for (i = 0; i < column_count; i++) {
		printf("%-15.64s	", column_values[i]);	
	}
	printf("\n");

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
	cgi_log_adjust_size("AdjustSize");

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
		"36	Adjust log file's size to a suitable size\n",
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

	printf("Sqlite3 version:%s\n", sqlite3_libversion());
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


