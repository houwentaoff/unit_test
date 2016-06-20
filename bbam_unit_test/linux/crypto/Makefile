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

default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/linux/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "Crypto Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -g -O0 -Wall

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@echo "    compile test_crypto..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_crypto test_crypto.c
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_crypto $(UNIT_TEST_PATH)/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_crypto

