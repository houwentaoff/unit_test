##
## Makefile
##
## History:
##    2010/01/18 - [Jian Tang]
##
## Copyright (C) 2004-2010, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/private/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "Middleware Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk
IMGPROC_PATH		:= $(CONFIG_IMGPROC_INSTALL_DIR)
MW_PATH				:= $(AMBABUILD_TOPDIR)/prebuild/mw
LINK_FLAG			:= -L $(IMGPROC_PATH) -L $(MW_PATH) -lamp -limg_algo_a5s -lpthread

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -g -O0 -Wall

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@echo "    compile test_image..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_image test_image.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	@echo "    compile test_motion_detect..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_motion_detect test_motion_detect.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_image test_image.cfg test_motion_detect $(UNIT_TEST_PATH)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_image test_amp test_motion_detect
