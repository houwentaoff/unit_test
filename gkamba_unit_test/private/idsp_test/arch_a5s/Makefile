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
AMBABUILD_TOPDIR	?= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "IDSP Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk
IMGPROC_PATH		:= $(CONFIG_IMGPROC_INSTALL_DIR)

LINK_FLAG		:= -L $(IMGPROC_PATH) -limg_algo_a5s -lpthread

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -g -O0 -Wall


.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
ifeq ($(BUILD_AMBARELLA_IMGPROC_PACKAGE), y)
ifeq ($(CONFIG_ARCH_A5S),y)
	@echo "    compile test_idsp..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_idsp test_idsp.c $(LINK_FLAG)
##	@echo "    compile test_stilcap..."
##	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_stilcap test_stilcap.c $(LINK_FLAG) \
##		-L $(AMBABUILD_TOPDIR)/prebuild/datatx -ldatatx
	@echo "    compile test_tuning..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_tuning test_tuning.c $(LINK_FLAG)
##	@echo "    compile test_af..."
##	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_af test_af.c $(LINK_FLAG)	
	@echo "    compile customer_loop..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o customer_loop customer_loop.c $(LINK_FLAG)	
endif
endif

ifeq ($(BUILD_AMBARELLA_IMGPROC_PACKAGE), y)
ifeq ($(CONFIG_ARCH_A5S),y)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_idsp test_tuning $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_IMGPROC_PATH)/
	$(AMBA_MAKEFILE_V)cp -a 3D.bin reg.bin $(UNIT_TEST_IMGPROC_PATH)/
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_IMGPROC_PATH)/sensors/
	$(AMBA_MAKEFILE_V)cp -a *_3D.bin $(UNIT_TEST_IMGPROC_PATH)/sensors/
endif
endif

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_idsp test_tuning
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/test_i1idsp
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/test_i1tuning
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/*.bin
