/***********************************************************
 * test_pca9554.c
 *
 * History:
 *	20102/10/18 - [Yuan Lulu] created file
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

#define PNUM_MAX 7
#define IN_PNUM 3
#define OUT_PNUM 4

int main(int argc, char *argv[])
{
	int level = -1;
	int ret = 0;

	if (argc != 2) {
		printf("%s_%d:argc != 2\n", __FILE__, __LINE__);
		return -1;
	}

	level = atoi(argv[1]);
	if (level < 0) {
		printf("%s_%d:invalid level %d\n", __FILE__, __LINE__, level);
		return -1;
	}
	
	if( !mw_pca9554_probe() )
	{
		printf("have no pca9554\n");
		return -1;
	}
	
	ret = mw_pca9554_open(NULL, 0);
	if (ret < 0) {
		printf("open pca9554 failed\n");
		return -1;
	}

	printf("set pcat9554's Pin%d to level %d\n", OUT_PNUM, level);
	/* set output pin */
	ret = mw_pca9554_asoutput_set(OUT_PNUM, !!level);
	if (ret < 0) {
		mw_pca9554_close();
		printf("set pca9554's P%d failed\n", OUT_PNUM);
		return -1;
	}

	mw_pca9554_close();
	return 0;
}

