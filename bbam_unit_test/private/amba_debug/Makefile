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
AMBABUILD_TOPDIR	?= $(PWD)/../../..
MODULE_NAME		:= "Amba Debug"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -O0 -Wall #-g 

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@echo "    compile amba_debug..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o amba_debug amba_debug.c
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip amba_debug
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a amba_debug $(UNIT_TEST_PATH)/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f amba_debug
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/amba_debug
