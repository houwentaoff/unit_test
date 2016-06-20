/*
 * test_amp.c
 * This program can setup VIN, preview and VOUT, and start / stop
 * encoding for flexible multi streaming encoding.
 * After setup ready or start / stop encoding, this program will exit.
 *
 * History:
 *	2010/01/26 - [Jian Tang] create this file to test amp.
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "mw_struct.h"
#include "mw_api.h"


int main(int argc, char ** argv)
{
	mw_init();
	printf("Init Middleware!\n");
	return 0;
}

#define __END_OF_FILE__


