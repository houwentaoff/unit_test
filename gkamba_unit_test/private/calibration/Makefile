##
## Makefile
##
## History:
##    2009/07/08 - [Anthony Ginger]
##
## Copyright (C) 2004-2009, Ambarella, Inc.
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
AMBABUILD_TOPDIR	:= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "Calibration"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

IMGPROC_PATH		:= $(CONFIG_IMGPROC_INSTALL_DIR)
MW_PATH				:= $(AMBABUILD_TOPDIR)/prebuild/mw
ADJ_PARAM_PATH		:= $(AMBABUILD_TOPDIR)/unit_test/private/idsp_test/arch/adj_params
LINK_FLAG			:= -L $(IMGPROC_PATH) -limg_algo_a5s -lpthread
LINK_FLAG2			:= -L $(IMGPROC_PATH) -L $(MW_PATH) -lamp -limg_algo_a5s -lpthread

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -g -O0 -Wall

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
ifeq ($(BUILD_AMBARELLA_IMGPROC_PACKAGE), y)
	@echo "    compile cali_bad_pixel..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o cali_bad_pixel cali_bad_pixel.c $(LINK_FLAG)
	@echo "    compile cali_lens_shading..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o cali_lens_shading cali_lens_shading.c $(LINK_FLAG2)
	@echo "    compile cali_awb..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o cali_awb cali_awb.c $(LINK_FLAG2) -I$(ADJ_PARAM_PATH)
	@echo "    compile bitmap_merger..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o bitmap_merger bitmap_merger.c 
	@$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	@$(AMBA_MAKEFILE_V)cp -a cali_bad_pixel $(UNIT_TEST_PATH)/
	@$(AMBA_MAKEFILE_V)cp -a cali_lens_shading $(UNIT_TEST_PATH)/
	@$(AMBA_MAKEFILE_V)cp -a cali_awb $(UNIT_TEST_PATH)/
	@$(AMBA_MAKEFILE_V)cp -a bitmap_merger $(UNIT_TEST_PATH)/
	@$(AMBA_MAKEFILE_V)cp -a *.sh $(UNIT_TEST_PATH)/
endif

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f cali_bad_pixel cali_lens_shading cali_awb bitmap_merger

