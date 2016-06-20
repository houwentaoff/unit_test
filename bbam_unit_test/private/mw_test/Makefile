##
## Makefile
##
## History:
##    2010/01/18 - [Jian Tang]
##
## Copyright (C) 2004-2010, Ambarella, Inc.
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
MODULE_NAME		:= "Middleware Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk
include $(AMBABUILD_TOPDIR)/.config

IMGPROC_PATH		:= $(CONFIG_IMGPROC_INSTALL_DIR)
MW_PATH				:= $(AMBABUILD_TOPDIR)/prebuild/mw
IRCUT_PATH			:= $(AMBABUILD_TOPDIR)/prebuild/ircut
LINK_FLAG           = -Wl,--start-group
LINK_FLAG			+= -L $(IMGPROC_PATH) -L $(MW_PATH) -lamp  -lhwcheck -limg_algo_a5s -lpthread -lm -L $(IRCUT_PATH) \
							 -I$(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/inc \
							 -L$(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib -lsqlite3 -I$(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/inc/
LINK_FLAG		+= -lIrCutAdc
LINK_FLAG		+= -lIrCutAe
ifeq ($(BUILD_GMI_UNIT_ZHUOYANG), y)
LINK_FLAG		+= -laccvisio -l_CM
endif
LINK_FLAG           += -L$(MW_PATH) -lcapability
LINK_FLAG           += -L$(AMBABUILD_TOPDIR)/prebuild/third-party/libtinyxml/lib -ltinyxml -lstdc++
LINK_FLAG           += -Wl,--end-group

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -O0 -Wall #-g 

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	-@$(AMBA_MAKEFILE_V)touch $(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib/libsqlite3.so 
	-@$(AMBA_MAKEFILE_V)rm $(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib/libsqlite3.so 
	-@$(AMBA_MAKEFILE_V)ln -s $(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib/libsqlite3.so.0.8.6 $(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib/libsqlite3.so 
	@echo "    compile test_image..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_image test_image.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_image
	@echo "    compile test_amp..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_amp test_amp.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_amp
	@echo "    compile test_motion_detect..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_motion_detect test_motion_detect.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_motion_detect
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_image test_image.cfg $(UNIT_TEST_PATH) #test_motion_detect 
	@echo "    compile test_motor_lens..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_motor_lens test_motor_lens.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_motor_lens
	$(AMBA_MAKEFILE_V)cp -a test_motor_lens $(UNIT_TEST_PATH) 
	@echo "    compile test_log..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_log test_log.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_log
	$(AMBA_MAKEFILE_V)cp -a test_log $(UNIT_TEST_PATH) 
	@echo "    compile test_cgi_log..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_cgi_log test_cgi_log.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_cgi_log
#$(AMBA_MAKEFILE_V)cp -a test_cgi_log $(UNIT_TEST_PATH) 
	@echo "    compile test_pca9554..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_pca9554_input test_pca9554_input.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_pca9554_input
	$(AMBA_MAKEFILE_V)cp -a test_pca9554_input $(UNIT_TEST_PATH) 
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_pca9554_output test_pca9554_output.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_pca9554_output
	$(AMBA_MAKEFILE_V)cp -a test_pca9554_output $(UNIT_TEST_PATH) 
	
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_pca9554_probe test_pca9554_probe.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_pca9554_probe
	$(AMBA_MAKEFILE_V)cp -a test_pca9554_probe $(UNIT_TEST_PATH) 

ifeq ($(BUILD_GMI_UNIT_ZHUOYANG), y)
	@echo "    compile test_accvisio..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_accvisio test_accvisio.c lib_Support.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_accvisio
	$(AMBA_MAKEFILE_V)cp -a test_accvisio $(UNIT_TEST_PATH) 
endif
	@echo "    compile test hw_check..."
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_hw_check test_hw_check.c $(LINK_FLAG) $(UNIT_TEST_LDFLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_hw_check
	$(AMBA_MAKEFILE_V)cp -a test_hw_check $(UNIT_TEST_PATH) 
clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_image test_amp test_motion_detect test_motor_lens test_accvisio test_log test_pca9554_input test_pca9554_output test_cgi_log
