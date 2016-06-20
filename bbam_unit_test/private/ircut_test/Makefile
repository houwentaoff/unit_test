##
## Makefile 
## linzhong add static_bad_pixel_correction compile info at 2011/08/05 
## linzhong add set_mask_pixel compile info at 2011/08/08
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
MODULE_NAME		:= "IRCut Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk
include $(AMBABUILD_TOPDIR)/.config

IMGPROC_PATH		:= $(CONFIG_IMGPROC_INSTALL_DIR)
MW_PATH				:= $(AMBABUILD_TOPDIR)/prebuild/mw
IRCUT_PATH				:= $(AMBABUILD_TOPDIR)/prebuild/ircut
LINK_FLAG           = -Wl,--start-group
LINK_FLAG		+= -L $(IMGPROC_PATH) -limg_algo_a5s -lpthread -lm -L $(IRCUT_PATH)
LINK_FLAG		+= -lIrCutAdc
LINK_FLAG		+= -lIrCutAe
LINK_FLAG		+= -L $(MW_PATH) -lamp
LINK_FLAG           += -L $(MW_PATH) -lcapability
LINK_FLAG           += -L $(AMBABUILD_TOPDIR)/prebuild/third-party/libtinyxml/lib -ltinyxml -lstdc++
LINK_FLAG           += -Wl,--end-group

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -O0 -Wall #-g 
UNIT_TEST_CFLAG		+= -I $(AMBABUILD_TOPDIR)/mw/capability/inc 

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@echo "    compile ircut_test..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -I$(AMBABUILD_TOPDIR) -I$(AMBABUILD_TOPDIR)/packages/ircut/inc -o test_ircut IrCut_Test.c $(LINK_FLAG)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_ircut
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_ircut $(UNIT_TEST_PATH)
	$(AMBA_MAKEFILE_V)chmod +x $(UNIT_TEST_PATH)/test_ircut 
	#$(AMBA_MAKEFILE_V)cp -a test_ircut_adc $(UNIT_TEST_PATH)
	#$(AMBA_MAKEFILE_V)chmod +x $(UNIT_TEST_PATH)/test_ircut_adc 

	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) \
		-I$(AMBABUILD_TOPDIR) \
		-I$(AMBABUILD_TOPDIR)/packages/ircut/inc \
		-o test_ircut_lux \
		ircut_test_adc.c \
		$(LINK_FLAG)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip test_ircut_lux
	$(AMBA_MAKEFILE_V)cp -a test_ircut_lux $(UNIT_TEST_PATH)
	@$(AMBA_MAKEFILE_V)chmod +x $(UNIT_TEST_PATH)/test_ircut_lux 

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_ircut
	$(AMBA_MAKEFILE_V)rm -f test_ircut_lux

