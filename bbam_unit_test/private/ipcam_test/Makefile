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

.PHONY: all clean remove_temp_files

all:
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/ipcam_test/arch $@

remove_temp_files:
	$(AMBA_MAKEFILE_V)rm -f test_encode test_stream test_shmoo test_dptz test_yuvcap test_overlay test_mp4mux test_playback test_privacymask bmp_convert test_loadbmp test_memcpy test_sd test_ipcam test_ipcam_ex
ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
	$(AMBA_MAKEFILE_V)rm -f test_textinsert
endif
ifeq ($(BUILD_AMBARELLA_BSREADER_PACKAGE), y)
	$(AMBA_MAKEFILE_V)rm -f test_bsreader
endif
ifeq ($(BUILD_AMBARELLA_MDMOTBUF_PACKAGE), y)
	$(AMBA_MAKEFILE_V)rm -f test_md_motbuf
endif

clean: remove_temp_files
	if [ -d $(AMBABUILD_TOPDIR)/unit_test/private/ipcam_test/arch ]; then $(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/ipcam_test/arch $@; fi

link_arch:
ifeq ($(CONFIG_ARCH_A5S),y)
	$(AMBA_MAKEFILE_V)echo "  SYMLINK[arch -> arch_a5s] @ $(PWD)"
	-$(AMBA_MAKEFILE_V)ln -fsn arch_a5s arch
else
ifeq ($(CONFIG_ARCH_A7),y)
	$(AMBA_MAKEFILE_V)echo "  SYMLINK[arch -> arch_a7] @ $(PWD)"
	-$(AMBA_MAKEFILE_V)ln -fsn arch_a7 arch
else
	$(MAKE) common_link_arch
endif
endif

unlink_arch:
	$(MAKE) common_unlink_arch
