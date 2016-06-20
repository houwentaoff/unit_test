
default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/gm/, ,$(PWD)))
AMBABUILD_TOPDIR	?= $(PWD)/../../..
MODULE_NAME		:= "tstupload"
LINK_SRCS = uploadwarn.cpp uploadwork.cpp timesock.cpp messagequeue.cpp condwait.cpp condwait.h timesock.h gbutf8.hxx
OBJ_SRCS = uploadwarn.cpp timesock.cpp condwait.cpp
MODULE_SRCS = testupload.cpp $(OBJ_SRCS)
MODULE_OBJS = $(MODULE_SRCS:%.cpp=%.o)

export AMBABUILD_TOPDIR

-include $(AMBABUILD_TOPDIR)/.config 
include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
MODULE_CFLAGS = -I{AMBABUILD_TOPDIR} -I$(AMBABUILD_TOPDIR)/mw/Profile/inc/ -I$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/ -I$(AMBABUILD_TOPDIR)/mw/uploadwarn/include/ -I$(AMBABUILD_TOPDIR)/app/ipcam/cameraserver/utils -Wall -g
UNIT_TEST_CFLAG	?= $(AMBARELLA_CFLAGS) -O0 -Wall -g 
LINK_FLAG = -lpthread -L$(AMBABUILD_TOPDIR)/prebuild/mw/ -lProfile -lamp

ifeq ($(CONFIG_DDNS_CENTER_INFO),y)
LINK_SRCS += uploadddns.cpp uploadddns.h
LINK_FLAG += -luploadwarn
OBJ_SRCS += uploadddns.cpp
endif


ifneq ($(CONFIG_APP_IPCAM_CONFIG_DIR),)
GET_CONFIG_DIR:=$(shell echo $(CONFIG_APP_IPCAM_CONFIG_DIR))
ROOT_CONFIG_PATH :=$(word 2, $(subst rootfs/fakeroot, ,$(GET_CONFIG_DIR)))
MODULE_CFLAGS += -DGMI_UPLOAD_WARN_CFG=\"$(ROOT_CONFIG_PATH)/mediaserver/upcenter.cfg\"
MODULE_CFLAGS += -DGMI_UPLOAD_WARN_DEV_CFG=\"$(ROOT_CONFIG_PATH)/mediaserver/dev.cfg\"
MODULE_CFLAGS += -DGMI_UPLOAD_EMERGENCY_BIN=\"$(ROOT_APP_PATH)/uploadwarnemergency\"
MODULE_CFLAGS += -DCONFIG_ETC_PROTO_CFG=\"$(ROOT_CONFIG_PATH)/mediaserver/proto.cfg\"
endif


.PHONY: all clean

all:check_env
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME) ]:"
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	@echo " compile [$(MODULE_NAME)] ..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(UNIT_TEST_CFLAG) $(MODULE_CFLAGS) -o $(MODULE_NAME) $(MODULE_SRCS) $(UNIT_TEST_LDFLAG) $(LINK_FLAG)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip $(MODULE_NAME)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f *.o $(MODULE_NAME)
	$(AMBA_MAKEFILE_V)rm -f $(LINK_SRCS)


check_env:$(LINK_SRCS)

uploadwarn.cpp:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/uploadwarn.cpp

timesock.cpp:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/timesock.cpp

messagequeue.cpp:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/messagequeue.cpp

uploadwork.cpp:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/uploadwork.cpp

condwait.cpp:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/condwait.cpp

uploadddns.cpp:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/uploadddns.cpp

timesock.h:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/timesock.h

uploadddns.h:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/uploadddns.h

condwait.h:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/condwait.h

gbutf8.hxx:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/gbutf8.hxx

%.cpp:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/%.cpp
	$(AMBA_MAKEFILE_V)ln -s $< $@

%.h:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/%.h
	$(AMBA_MAKEFILE_V)ln -s $< $@

%.hxx:$(AMBABUILD_TOPDIR)/app/ipcam/uploadwarn/%.hxx
	$(AMBA_MAKEFILE_V)ln -s $< $@
