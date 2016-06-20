##
## unit_test/Makefile
##
## History:
##    2009/07/08 - [Anthony Ginger] Create
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
AMBABUILD_TOPDIR	?= $(word 1, $(subst /unit_test, ,$(PWD)))
MODULE_NAME		:= "Unit Tests"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

.PHONY: all clean

all clean:
	$(AMBA_MAKEFILE_V)echo "[$(MODULE_NAME)]:"
ifeq ($(BUILD_AMBARELLA_PRIVATE_LINUX_UNIT_TESTS), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private $@
endif
ifeq ($(BUILD_AMBARELLA_PUBLIC_LINUX_UNIT_TESTS), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/linux $@
endif
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/gm $@

