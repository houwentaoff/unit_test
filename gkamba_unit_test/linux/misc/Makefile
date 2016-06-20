##
## unit_test/common.mk
##
## History:
##    2008/02/18 - [Anthony Ginger] Create
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
MODULE_DIR		:= $(word 2, $(subst /unit_test/linux/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	:= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "Misc Test"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -g -O0 -Wall
LINK_FLAG	:= -L $(AMBABUILD_TOPDIR)/prebuild/lib -lpthread -lm
.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@echo "    compile test_pwm..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_pwm test_pwm.c
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_pwm $(UNIT_TEST_PATH)/
	@echo "    compile test_input..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_input test_input.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_input $(UNIT_TEST_PATH)/
	@echo "    compile test_fb..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_fb test_fb.c
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_fb $(UNIT_TEST_PATH)/
	@echo "    compile test_uart..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_uart test_uart.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_uart $(UNIT_TEST_PATH)/
	@echo "    compile test_spi..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_spi test_spi.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_spi $(UNIT_TEST_PATH)/
	@echo "    compile test_spi_slave..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_spi_slave test_spi_slave.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_spi_slave $(UNIT_TEST_PATH)/
	@echo "    compile test_led..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_led test_led.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_led $(UNIT_TEST_PATH)/
	@echo "    compile test_uhc_stg..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_uhc_stg test_uhc_stg.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_uhc_stg $(UNIT_TEST_PATH)/
	@echo "    compile cpu_full_load..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o cpu_full_load cpu_full_load.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a cpu_full_load $(UNIT_TEST_PATH)/
	@echo "    compile test_clock_gettime..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_clock_gettime test_clock_gettime.c $(LINK_FLAG) -lrt
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_clock_gettime $(UNIT_TEST_PATH)/
	@echo "    compile load50..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o load50 load50.c $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a load50 $(UNIT_TEST_PATH)/

ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
	@echo "    compile test_fbtext..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -I$(AMBABUILD_TOPDIR)/packages/textinsert \
		-o test_fbtext  test_fbtext.c $(UNIT_TEST_LDFLAG)  \
		-L $(AMBABUILD_TOPDIR)/prebuild/third-party/freetype/lib/ -lfreetype \
		-L $(AMBABUILD_TOPDIR)/prebuild/textinsert/ -ltextinsert
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a test_fbtext $(UNIT_TEST_PATH)/
endif

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_pwm test_input test_fb test_uart test_spi test_spi_slave test_uhc_stg test_led cpu_full_load test_clock_gettime
	$(AMBA_MAKEFILE_V)rm -f test_fbtext load50

