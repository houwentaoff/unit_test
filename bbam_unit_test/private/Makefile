##
## kernel/private/Makefile
##
## History:
##    2008/10/23 - [Anthony Ginger] Create
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
AMBABUILD_TOPDIR	?= $(word 1, $(subst /unit_test/private, ,$(PWD)))

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

.PHONY: all clean

all clean:
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IONE), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/ione_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_CONSUMER), y)
#	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/consumer_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IAV), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/iav_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IAV_SERVER), y)
#	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/iav_server $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_VIN), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/vin_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_VOUT), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/vout_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_AMBADEBUG), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/amba_debug $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_DSPBRINGUP), y)
#	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/dsp_bringup $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IPCAM), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/ipcam_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_MW), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/mw_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_QTAMFCMD), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/test_qtamf $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_THIRDPARTY), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/packages $@ 
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_CALIBRATION), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/calibration $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_VSYNC), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/vsync_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_CUSTOM_AAA), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/demo $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/custom_af $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_GYRO), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/gyro_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/idsp_test $@
endif

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_MDIN380_4D1), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/MDIN3xx_test $@
endif

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IR_DEMO), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/ir_demo $@
endif

#ifeq ($(BUILD_GMI_UNIT_TESTS_IRCUT), y)
#	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/ircut_test $@
#endif

	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/hw_watchdog $@

link_arch:
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IPCAM), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/ipcam_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/idsp_test $@
endif

unlink_arch:
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IPCAM), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/ipcam_test $@
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/unit_test/private/idsp_test $@
endif

