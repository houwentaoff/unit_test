##
## Makefile
##

default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/wt/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "ntpclient"
MODULE_SRCS = ntpclient.c phaselock.c
MODULE_OBJS = ntpclient.o phaselock.o

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
UNIT_TEST_CFLAG		?= $(AMBARELLA_CFLAGS) -O0 -Wall #-g 

.PHONY: all clean

all: $(MODULE_NAME)

$(MODULE_NAME): $(MODULE_OBJS)
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"
	@echo "    compile ntpclient..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o $(MODULE_NAME) $(MODULE_SRCS) $(UNIT_TEST_LDFLAG) $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip $(MODULE_NAME)
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a $(MODULE_NAME) $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)chmod +x $(UNIT_TEST_PATH)/$(MODULE_NAME)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f ntpclient *.gdb *.elf *.o

