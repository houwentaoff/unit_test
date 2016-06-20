##
## Makefile
##

default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/gm/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "test_pelco"
MODULE_SRCS = $(wildcard *.c)
MODULE_OBJS = $(MODULE_SRCS:%.c=%.o)

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG	?= $(AMBARELLA_CFLAGS) -O0 -Wall #-g 

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build Pelco Test]:"
#	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	@echo " compile $(MODULE_NAME) ..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o $(MODULE_NAME) $(MODULE_SRCS) $(UNIT_TEST_LDFLAG) $(LINK_FLAG) -lpthread
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip $(MODULE_NAME)
#	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
#	$(AMBA_MAKEFILE_V)cp -a $(MODULE_NAME) $(UNIT_TEST_PATH)/
#	$(AMBA_MAKEFILE_V)chmod +x $(UNIT_TEST_PATH)/$(MODULE_NAME)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f *.o $(MODULE_NAME)

