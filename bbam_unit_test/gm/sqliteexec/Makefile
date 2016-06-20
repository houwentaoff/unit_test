


default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/gm/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/../../..
MODULE_NAME		:= "sqlite3exec"
MODULE_SRCS = $(wildcard *.cpp)
MODULE_OBJS = $(MODULE_SRCS:%.cpp=%.o)

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
MODULE_CFLAGS = -I$(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/
UNIT_TEST_CFLAG	?= $(AMBARELLA_CFLAGS) -O0 -Wall #-g 
LINK_FLAG = -L$(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib -lsqlite3

.PHONY: all clean

all:check_env
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME) ]:"
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	@echo " compile testlocalProfile ..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(UNIT_TEST_CFLAG) $(MODULE_CFLAGS) -o $(MODULE_NAME) $(MODULE_SRCS) $(UNIT_TEST_LDFLAG) $(LINK_FLAG)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f *.o $(MODULE_NAME)
	$(AMBA_MAKEFILE_V)rm -f $(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib/libsqlite3.so


check_env:$(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib/libsqlite3.so

$(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib/libsqlite3.so:$(AMBABUILD_TOPDIR)/prebuild/third-party/sqlite/lib/libsqlite3.so.0.8.6
	ln -s $< $@



