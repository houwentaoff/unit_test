##
## unit_test/linux/benchmark/Makefile
##
## History:
##    2009/09/02 - [Cao Rongrong] Create
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
AMBABUILD_TOPDIR	?= $(word 1, $(subst /unit_test/linux, ,$(PWD)))
MODULE_NAME		:= "Benchmark"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_BANDWIDTH), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/bandwidth $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_NBENCH), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/nbench $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_PI_CSS5), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/pi_css5 $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_MEM), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/mem_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_SD_WRITE), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/sd_write $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_SD_TEST), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/sd_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_NET_TEST), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/net_test $@
endif

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_BANDWIDTH), y)
	@echo "    clean mem bandwidth test ..."
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/bandwidth $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_NBENCH), y)
	@echo "    clean CPU nbench test ..."
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/nbench $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_PI_CSS5), y)
	@echo "    clean CPU Pi CSS5 test ..."
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/pi_css5 $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_MEM), y)
	@echo "    clean memory test ..."
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/mem_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_SD_WRITE), y)
	@echo "    clean sd_write test ..."
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/sd_write $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_SD_TEST), y)
	@echo "    clean sd_test test ..."
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/sd_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_NET_TEST), y)
	@echo "    clean net_test test ..."
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark/net_test $@
endif
