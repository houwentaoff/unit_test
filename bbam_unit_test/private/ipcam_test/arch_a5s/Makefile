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
MODULE_NAME		:= "IPCam Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -O2 -Wall #-g 
UNIT_TEST_OBJS		= test_encode test_stream test_shmoo test_dptz test_yuvcap \
					test_overlay test_mp4mux test_playback test_privacymask \
					bmp_convert test_loadbmp test_qproi test_memcpy \
					test_qp_overlay guard_sd_space
UNIT_TEST_FILES	= Ambarella-256x128-8bit.bmp
UNIT_TEST_GCC	 = $(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG)

.PHONY: all clean build_objs cp_files

build_objs:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@echo "    compile test_encode..."
	@$(UNIT_TEST_GCC) -o test_encode test_encode.c $(UNIT_TEST_LDFLAG)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_encode
	@echo "    compile test_playback..."
	@$(UNIT_TEST_GCC) -o test_playback test_playback.c $(UNIT_TEST_LDFLAG) \
		-lpthread
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_playback

ifeq ($(BUILD_AMBARELLA_BSREADER_PACKAGE), y)
	@echo "    compile test_bsreader..."
ifeq ($(CONFIG_APP_IPCAM_USE_WATCHDOG), y)	
	@$(UNIT_TEST_GCC) -o test_bsreader test_bsreader.c $(UNIT_TEST_LDFLAG)	\
		-L $(AMBABUILD_TOPDIR)/prebuild/bsreader -lbsreader -lpthread \
		-L $(AMBABUILD_TOPDIR)/prebuild/cavlc -lcavlc \
		-L $(AMBABUILD_TOPDIR)/prebuild/datatx -ldatatx -lpthread
else
	@$(UNIT_TEST_GCC) -o test_bsreader test_bsreader.c $(UNIT_TEST_LDFLAG)	\
		-L $(AMBABUILD_TOPDIR)/prebuild/bsreader -lbsreader -lpthread \
		-L $(AMBABUILD_TOPDIR)/prebuild/cavlc -lcavlc \
		-L $(AMBABUILD_TOPDIR)/prebuild/datatx -ldatatx -lphread
endif
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_bsreader
endif

	@echo "    compile test_stream..."
	@$(UNIT_TEST_GCC) -o test_stream test_stream.c $(UNIT_TEST_LDFLAG)	\
		-L $(AMBABUILD_TOPDIR)/prebuild/datatx -ldatatx -lpthread
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_stream
	@echo "    compile test_privacymask..."
	@$(UNIT_TEST_GCC) -o test_privacymask test_privacymask.c $(UNIT_TEST_LDFLAG)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_privacymask
	@echo "    compile test_dptz..."
	@$(UNIT_TEST_GCC) -o test_dptz test_dptz.c $(UNIT_TEST_LDFLAG)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_dptz
	@echo "    compile test_yuvcap..."
	@$(UNIT_TEST_GCC) -o test_yuvcap test_yuvcap.c $(UNIT_TEST_LDFLAG)	\
		-L $(AMBABUILD_TOPDIR)/prebuild/datatx -ldatatx -lpthread
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_yuvcap

	@echo "    compile test_overlay..."
	@$(UNIT_TEST_GCC) -o test_overlay test_overlay.c $(UNIT_TEST_LDFLAG)
	@echo "    compile bmp_convert..."
	@$(UNIT_TEST_GCC) -o bmp_convert bmp_convert.c $(UNIT_TEST_LDFLAG)
	@echo "    compile test_loadbmp..."
	@$(UNIT_TEST_GCC) -o test_loadbmp test_loadbmp.c $(UNIT_TEST_LDFLAG)
	@echo "    compile test_mp4mux..."
	@$(UNIT_TEST_GCC) -o test_mp4mux test_mp4mux.c $(UNIT_TEST_LDFLAG)
	@echo "    compile test_shmoo..."
	@$(UNIT_TEST_GCC) -o test_shmoo test_shmoo.c $(UNIT_TEST_LDFLAG) -lm
	@echo "    compile test_memcpy..."
	@$(UNIT_TEST_GCC) -o test_memcpy test_memcpy.c $(UNIT_TEST_LDFLAG)
	@echo "    compile test_qproi..."
	@$(UNIT_TEST_GCC) -o test_qproi test_qproi.c $(UNIT_TEST_LDFLAG)
	@echo "    compile test_qp_overlay..."
	@$(UNIT_TEST_GCC) -o test_qp_overlay test_qp_overlay.c $(UNIT_TEST_LDFLAG)
	@echo "    compile guard_sd_space..."
	@$(UNIT_TEST_GCC) -o guard_sd_space guard_sd_space.c $(UNIT_TEST_LDFLAG)  -lpthread
#	@echo "    compile test_sd..."
#	@$(UNIT_TEST_GCC) -o test_sd test_sd.c $(UNIT_TEST_LDFLAG)
#	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_sd
ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
	@echo "    compile test_textinsert..."
	@$(UNIT_TEST_GCC) -I$(AMBABUILD_TOPDIR)/packages/textinsert \
		-o test_textinsert  test_textinsert.c $(UNIT_TEST_LDFLAG) \
		-L $(AMBABUILD_TOPDIR)/prebuild/third-party/freetype/lib \
		-L $(AMBABUILD_TOPDIR)/prebuild/textinsert/ -lfreetype -ltextinsert
endif
ifeq ($(BUILD_AMBARELLA_MDMOTBUF_PACKAGE), y)
	@echo "    compile test_md_motbuf..."
	@$(UNIT_TEST_GCC) -o test_md_motbuf  test_md_motbuf.c $(UNIT_TEST_LDFLAG)  \
		-L $(AMBABUILD_TOPDIR)/prebuild/md_motbuf/  -lmd_motbuf -lpthread
endif

cp_files:
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a $(UNIT_TEST_FILES) $(UNIT_TEST_OBJS) $(UNIT_TEST_PATH)/
ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
#	$(AMBA_MAKEFILE_V)cp -a test_textinsert $(UNIT_TEST_PATH)/
endif
ifeq ($(BUILD_AMBARELLA_BSREADER_PACKAGE), y)
	$(AMBA_MAKEFILE_V)cp -a test_bsreader $(UNIT_TEST_PATH)/
endif
ifeq ($(BUILD_AMBARELLA_MDMOTBUF_PACKAGE), y)
#	$(AMBA_MAKEFILE_V)cp -a test_md_motbuf $(UNIT_TEST_PATH)/
endif

all: build_objs cp_files

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_sd $(UNIT_TEST_OBJS)
ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
	$(AMBA_MAKEFILE_V)rm -f test_textinsert
endif
ifeq ($(BUILD_AMBARELLA_BSREADER_PACKAGE), y)
	$(AMBA_MAKEFILE_V)rm -f test_bsreader
endif
ifeq ($(BUILD_AMBARELLA_MDMOTBUF_PACKAGE), y)
	$(AMBA_MAKEFILE_V)rm -f test_md_motbuf
endif
