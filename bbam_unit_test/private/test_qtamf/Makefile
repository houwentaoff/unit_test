##
## packages/QtAMF/Makefile
##
## History:
##    2011/08/09 - [Yupeng Chang] Create
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
MODULE_NAME		:= "test_qtamf"

export AMBABUILD_TOPDIR
include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

QMAKE = $(AMBABUILD_TOPDIR)/prebuild/third-party/Qt/latest/bin/qmake

all:
	@echo "    compile test_qtamf..."
	-@rm -rf *.mk
	@$(QMAKE) -spec $(AMBABUILD_TOPDIR)/prebuild/third-party/Qt/latest/share/qt4/mkspecs/qws/linux-arm-gnueabi-g++ \
										test_qtamf.pro -o test_qtamf.mk CONFIG+=release
	@make -f test_qtamf.mk > /dev/null
	-@$(AMBA_MAKEFILE_V)cp -a test_qtamf $(UNIT_TEST_PATH)/
	@$(QMAKE) -spec $(AMBABUILD_TOPDIR)/prebuild/third-party/Qt/latest/share/qt4/mkspecs/qws/linux-arm-gnueabi-g++ \
										test_qtamf_play.pro -o test_qtamf_play.mk CONFIG+=release
	@make -f test_qtamf_play.mk > /dev/null
	-@$(AMBA_MAKEFILE_V)cp -a test_qtamf_play $(UNIT_TEST_PATH)/

clean:
	-@rm -rf *.mk
	@$(QMAKE) -spec $(AMBABUILD_TOPDIR)/prebuild/third-party/Qt/latest/share/qt4/mkspecs/qws/linux-arm-gnueabi-g++ \
										test_qtamf.pro -o test_qtamf.mk CONFIG+=release
	-@make -f test_qtamf.mk clean > /dev/null 2>&1
	-@rm -rf test_qtamf
	@$(QMAKE) -spec $(AMBABUILD_TOPDIR)/prebuild/third-party/Qt/latest/share/qt4/mkspecs/qws/linux-arm-gnueabi-g++ \
										test_qtamf_play.pro -o test_qtamf_play.mk CONFIG+=release
	-@make -f test_qtamf_play.mk clean > /dev/null 2>&1
	-@rm -rf test_qtamf_play
	-@rm -rf *.mk
	@find . -name release -type d | xargs rm -rf
	@find . -name release -type d | xargs rm -rf

distclean:
	-@rm -rf *.mk
	@$(QMAKE) -spec $(AMBABUILD_TOPDIR)/prebuild/third-party/Qt/latest/share/qt4/mkspecs/qws/linux-arm-gnueabi-g++ \
										test_qtamf.pro -o test_qtamf.mk CONFIG+=release
	-@make -f test_qtamf.mk distclean > /dev/null 2>&1
	@$(QMAKE) -spec $(AMBABUILD_TOPDIR)/prebuild/third-party/Qt/latest/share/qt4/mkspecs/qws/linux-arm-gnueabi-g++ \
										test_qtamf_play.pro -o test_qtamf_play.mk CONFIG+=release
	-@make -f test_qtamf_play.mk distclean > /dev/null 2>&1

.PHONY: all clean distclean
