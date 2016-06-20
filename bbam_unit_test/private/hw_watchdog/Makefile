##
## Makefile 
## linzhong add static_bad_pixel_correction compile info at 2011/08/05 
## linzhong add set_mask_pixel compile info at 2011/08/08
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
AMBABUILD_TOPDIR	?= $(PWD)/../../..
MODULE_NAME		:= "HW WATCHDOG Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk
include $(AMBABUILD_TOPDIR)/.config

IMGPROC_PATH		:= $(CONFIG_IMGPROC_INSTALL_DIR)
MW_PATH				:= $(AMBABUILD_TOPDIR)/prebuild/mw
HW_WATCHDOG_PATH				:= $(AMBABUILD_TOPDIR)/prebuild/ledwatchdog
LINK_FLAG		:= -L $(IMGPROC_PATH) -limg_algo_a5s -lpthread -lm -L $(HW_WATCHDOG_PATH)
LINK_FLAG		+= -lledwatchdog

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -O0 -Wall #-g 

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc hw_watchdog_reboot.c \
		$(UNIT_TEST_CFLAG) \
		-I$(AMBABUILD_TOPDIR) \
		-I$(AMBABUILD_TOPDIR)/packages/ledwatchdog/include \
		$(LINK_FLAG) \
		-o hw_watchdog_reboot
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip hw_watchdog_reboot
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a hw_watchdog_reboot $(UNIT_TEST_PATH)
	@$(AMBA_MAKEFILE_V)chmod +x $(UNIT_TEST_PATH)/hw_watchdog_reboot 

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f hw_watchdog_reboot

