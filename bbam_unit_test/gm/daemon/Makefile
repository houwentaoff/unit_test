
default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /app/ipcam, ,$(PWD)))
AMBABUILD_TOPDIR	?= $(PWD)/../../..
MODULE_NAME		:= "testdaemonlib"
MODULE_SRCS = testdaemonlib.cpp
MODULE_OBJS = $(MODULE_SRCS:%.cpp=%.o)

export AMBABUILD_TOPDIR

-include $(AMBABUILD_TOPDIR)/build/app/common.mk
-include $(AMBABUILD_TOPDIR)/.config
-include $(AMBABUILD_TOPDIR)/app/ipcam/mediaserver/config.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
MODULE_CFLAGS = -Wall -g -I$(AMBABUILD_TOPDIR)/prebuild/third-party/libdaemon/include
UNIT_TEST_CFLAG	?= $(AMBARELLA_CFLAGS) -O0 -Wall -g 
LINK_FLAG = -ldaemon -L$(AMBABUILD_TOPDIR)/prebuild/third-party/libdaemon/lib/


.PHONY: all clean

all:check_env
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME) ]:"
	@echo " compile [$(MODULE_NAME)] ..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(UNIT_TEST_CFLAG) $(MODULE_CFLAGS) -o $(MODULE_NAME) $(MODULE_SRCS) $(UNIT_TEST_LDFLAG) $(LINK_FLAG)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f *.o $(MODULE_NAME)


check_env:



