default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/gm/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/../../..
MODULE_NAME		:= "testdecoder"
MODULE_SRCS = $(wildcard *.c)
MODULE_OBJS = $(MODULE_SRCS:%.c=%.o)

export AMBABUILD_TOPDIR

-include $(AMBABUILD_TOPDIR)/.config 
include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

UNIT_TEST_LDFLAG	?= $(AMBARELLA_LDFLAGS)
MODULE_CFLAGS = -I$(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/include/libavcodec/ -I$(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/include/libavformat/ -I$(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/include/
UNIT_TEST_CFLAG	?= $(AMBARELLA_CFLAGS) -O0 -Wall -g 
LINK_FLAG = -L$(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib -lavformat -lavutil -lavcodec -lx264 -lavdevice


.PHONY: all clean

all:check_env
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME) ]:"
	@echo " compile testlocalProfile ..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(UNIT_TEST_CFLAG) $(MODULE_CFLAGS) -o $(MODULE_NAME) $(MODULE_SRCS) $(UNIT_TEST_LDFLAG) $(LINK_FLAG)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f *.o $(MODULE_NAME)


check_env:
	$(AMBA_MAKEFILE_V)rm -f $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavformat.so
	$(AMBA_MAKEFILE_V)rm -f $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavcodec.so
	$(AMBA_MAKEFILE_V)rm -f $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavutil.so
	$(AMBA_MAKEFILE_V)rm -f $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavdevice.so
	$(AMBA_MAKEFILE_V)rm -f $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libx264.so
	$(AMBA_MAKEFILE_V)ln -s $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavformat.so.52.23.1 $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavformat.so
	$(AMBA_MAKEFILE_V)ln -s $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavcodec.so.52.6.3 $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavcodec.so
	$(AMBA_MAKEFILE_V)ln -s $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavutil.so.49.12.0 $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavutil.so
	$(AMBA_MAKEFILE_V)ln -s $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavdevice.so.52.1.0 $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libavdevice.so
	$(AMBA_MAKEFILE_V)ln -s $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libx264.so.65 $(AMBABUILD_TOPDIR)/prebuild/third-party/ffmpeg/lib/libx264.so
