##
## Makefile
##
## History:
##    2010/01/08 - [Qiao Wang]
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
AMBABUILD_TOPDIR	:= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "IRQ Proc Test"
LINK_FLAG		:= -lpthread

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -g -O0 -Wall

.PHONY: all clean

all:
	@echo "    compile test_soft_vsync..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_soft_vsync test_soft_vsync.c $(UNIT_TEST_LDFLAG) $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_soft_vsync $(UNIT_TEST_PATH)/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_soft_vsync

