
default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/gm/, ,$(PWD)))
AMBABUILD_TOPDIR	?= $(PWD)/../../..
MODULE_NAME		:= "delaycmd"
MODULE_SRCS = testdelaycmd.cpp
MODULE_OBJS = $(MODULE_SRCS:%.c=%.o)

export AMBABUILD_TOPDIR

-include $(AMBABUILD_TOPDIR)/.config 
include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
MODULE_CFLAGS = -I. -Wall -g
UNIT_TEST_CFLAG	?= $(AMBARELLA_CFLAGS) -O0 -Wall -g 
LINK_FLAG = 


.PHONY: all clean

all:check_env
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME) ]:"
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	@echo " compile [$(MODULE_NAME)] ..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(UNIT_TEST_CFLAG) $(MODULE_CFLAGS) -o $(MODULE_NAME) $(MODULE_SRCS) $(UNIT_TEST_LDFLAG) $(LINK_FLAG)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f *.o $(MODULE_NAME)
	$(AMBA_MAKEFILE_V)rm -f timesock.cpp timesock.h

check_env:timesock.cpp timesock.h


timesock.cpp:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/timesock.cpp
	$(AMBA_MAKEFILE_V)ln -s $< $@

timesock.h:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/timesock.h
	$(AMBA_MAKEFILE_V)ln -s $< $@