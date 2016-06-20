##
## unit_test/linux/Makefile
##
## History:
##    2009/09/02 - [ Cao Rongrong] Create
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

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

.PHONY: all clean

all clean:
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_AUDIO), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/audio $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_BENCHMARK), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/benchmark $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_CRYPTO), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/crypto $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_FLASH_FS), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/flash_fs $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDE), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/ide $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_MISC), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/misc $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_NAND), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/nand $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_UPGRADE), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/upgrade $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_USB_STREAM), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux/usb $@
endif

