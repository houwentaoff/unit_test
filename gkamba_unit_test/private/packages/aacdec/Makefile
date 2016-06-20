##
## unit_test/common.mk
##
## History:
##    2008/02/18 - [Anthony Ginger] Create
##
## Copyright (C) 2004-2008, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

PWD             := $(shell pwd)
MODULE_DIR      := $(word 2, $(subst /unit_test/, ,$(shell pwd)))
UNIT_TEST_DIR   := $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR?= $(word 1, $(subst unit_test/private/packages/aacdec/, ,$(PWD)))

INCLUDE_FLAG    := -I$(AMBABUILD_TOPDIR) \
		-I$(AMBABUILD_TOPDIR)/build/include \
		-I$(AMBABUILD_TOPDIR)/prebuild/third-party/misc/include
LINK_PREBUILD_DIR	:=$(AMBABUILD_TOPDIR)/prebuild/third-party/misc/lib

LINK_FLAG	:= -L $(LINK_PREBUILD_DIR) -laacdec
C_FLAG		:= -s -O2 -Wall

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

export AMBABUILD_TOPDIR

.PHONY: all clean link_arch unlink_arch

all:
	@echo "    compile test_aacdec..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(INCLUDE_FLAG) $(UNIT_TEST_CFLAG) -o test_aacdec test_aacdec.c $(LINK_FLAG)
	@$(AMBA_MAKEFILE_V)cp -a test_aacdec $(UNIT_TEST_PATH)/

clean:
	$(AMBA_MAKEFILE_V)echo "    clean test_aacdec..."
	$(AMBA_MAKEFILE_V)rm -f test_aacdec

link_arch:

unlink_arch:

