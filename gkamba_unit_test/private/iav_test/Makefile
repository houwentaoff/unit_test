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
MODULE_NAME		:= "IAV Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk
IMGPROC_PATH		:= $(CONFIG_IMGPROC_INSTALL_DIR)
LINK_FLAG		:= -lpthread

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -g -O0 -Wall

.PHONY: build_common_app build_ione_app clean_common_app clean_ione_app
.PHONY: all clean 


build_common_app:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@echo "    compile load_ucode..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o load_ucode load_ucode.c
	@echo "    compile test2..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test2 test2.c $(UNIT_TEST_LDFLAG)
	@echo "    compile test3..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test3 test3.c
	@echo "    compile dsplog..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o dsplog dsplog.c
	@echo "    compile dsplog2..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o dsplog2 dsplog2.c
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a load_ucode test2 test3 dsplog dsplog2 $(UNIT_TEST_PATH)/
	if [ ! -e $(UNIT_TEST_PATH)/capture_log ]; then ln -s dsplog $(UNIT_TEST_PATH)/capture_log; fi
	if [ ! -e $(UNIT_TEST_PATH)/parse_log ]; then ln -s dsplog2 $(UNIT_TEST_PATH)/parse_log; fi

build_ione_app:
ifeq ($(CONFIG_ARCH_I1),y)
	@echo "    compile udec..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o udec udec.c $(LINK_FLAG)
	@echo "    compile mudec..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o mudec mudec.c $(LINK_FLAG)
	@echo "    compile duplex..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o duplex duplex.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a udec duplex mudec $(UNIT_TEST_PATH)/
endif

clean_common_app:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f load_ucode test2 test3 dsplog dsplog2
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/load_ucode
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/test2
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/test3
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/dsplog
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/dsplog2
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/capture_log
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/parse_log

clean_ione_app:
ifeq ($(CONFIG_ARCH_I1),y)
	$(AMBA_MAKEFILE_V)rm -f udec duplex
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/udec
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/mudec
	$(AMBA_MAKEFILE_V)rm -f $(UNIT_TEST_PATH)/duplex
endif

all: build_common_app build_ione_app

clean: clean_common_app clean_ione_app


