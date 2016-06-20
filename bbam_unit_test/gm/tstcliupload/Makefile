
default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/gm/, ,$(PWD)))
AMBABUILD_TOPDIR	?= $(PWD)/../../..
MODULE_NAME		:= "tstcliupload"
LINK_SRCS = 
OBJ_SRCS = 
MODULE_SRCS = tstcliupload.cpp $(OBJ_SRCS)
MODULE_OBJS = $(MODULE_SRCS:%.cpp=%.o)

export AMBABUILD_TOPDIR

-include $(AMBABUILD_TOPDIR)/.config 
include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
MODULE_CFLAGS = -DGM_UNIX -I$(AMBABUILD_TOPDIR)/mw/uploadwarn/include/  -Wall -g
UNIT_TEST_CFLAG	?= $(AMBARELLA_CFLAGS) -O0 -Wall -g 
LINK_FLAG = -lpthread -L$(AMBABUILD_TOPDIR)/prebuild/mw/ -luploadwarn


.PHONY: all clean

all:check_env
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME) ]:"
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(UNIT_TEST_CFLAG) $(MODULE_CFLAGS) -o $(MODULE_NAME) $(MODULE_SRCS) $(UNIT_TEST_LDFLAG) $(LINK_FLAG)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f *.o $(MODULE_NAME)
	$(AMBA_MAKEFILE_V)rm -f $(LINK_SRCS)


check_env:$(LINK_SRCS)



