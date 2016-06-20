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

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -O2 -Wall #-g 

.PHONY: all clean

all clean:
	if [ -d $(AMBABUILD_TOPDIR)/unit_test/private/idsp_test/arch ]; then $(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/idsp_test/arch $@; fi

link_arch:
ifeq ($(CONFIG_A5S_IAV_FOR_DV),y)
	$(AMBA_MAKEFILE_V)echo "  SYMLINK[arch -> arch_a5s_dv] @ $(PWD)"
	-$(AMBA_MAKEFILE_V)ln -fsn arch_a5s_dv arch
else
ifeq ($(CONFIG_ARCH_A5S),y)
	$(AMBA_MAKEFILE_V)echo "  SYMLINK[arch -> arch_a5s] @ $(PWD)"
	-$(AMBA_MAKEFILE_V)ln -fsn arch_a5s arch
else
ifeq ($(CONFIG_ARCH_A7),y)
	$(AMBA_MAKEFILE_V)echo "  SYMLINK[arch -> arch_a7] @ $(PWD)"
	-$(AMBA_MAKEFILE_V)ln -fsn arch_a7 arch
else
ifeq ($(CONFIG_ARCH_I1),y)
	$(AMBA_MAKEFILE_V)echo "  SYMLINK[arch -> arch_i1] @ $(PWD)"
	-$(AMBA_MAKEFILE_V)ln -fsn arch_i1 arch
else
	$(MAKE) common_link_arch
endif
endif
endif
endif

unlink_arch:
	$(MAKE) common_unlink_arch
