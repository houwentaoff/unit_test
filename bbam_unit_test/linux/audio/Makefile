##
## unit_test/common.mk
##
## History:
##    2008/02/18 - [Cao Rongrong] Create
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
AMBABUILD_TOPDIR	?= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "Audio Test"

-include $(AMBABUILD_TOPDIR)/.config

INCLUDE_ALSA    := -I$(AMBABUILD_TOPDIR)/prebuild/third-party/alsa/include -Ialsautils
INCLUDE_PULSE	:= -I$(AMBABUILD_TOPDIR)/prebuild/third-party/pulseaudio/include -I.
INCLUDE_PULSE_EXTRA	:= -I$(AMBABUILD_TOPDIR)/prebuild/third-party/libsndfile/include

LINK_ALSA	:= -L$(AMBABUILD_TOPDIR)/prebuild/third-party/alsa/lib
LINK_PULSE	:= -L$(AMBABUILD_TOPDIR)/prebuild/third-party/pulseaudio/lib -lpulsecommon-1.1 

LIBS		:= -lasound -lpthread -ldl
LIB_PULSE_EXTRA	:= -L$(AMBABUILD_TOPDIR)/prebuild/third-party/libsndfile/lib -lsndfile \
					-L$(AMBABUILD_TOPDIR)/prebuild/third-party/dbus/lib -ldbus-1 \
					-L$(AMBABUILD_TOPDIR)/prebuild/third-party/libtool/lib -lltdl \
					-L$(AMBABUILD_TOPDIR)/prebuild/third-party/libsamplerate/lib -lsamplerate \
					-L$(AMBABUILD_TOPDIR)/prebuild/third-party/speex/lib -lspeexdsp \
					-L$(AMBABUILD_TOPDIR)/prebuild/third-party/json-c/lib -ljson \
					-L$(AMBABUILD_TOPDIR)/prebuild/third-party/orc/lib -lorc-0.4 \
					-L$(AMBABUILD_TOPDIR)/prebuild/third-party/glib2/lib -lglib-2.0
					
LIB_PULSE	:= -lpulsecommon-1.1 -lpulsecore-1.1 -lpulsedsp -lpulse-mainloop-glib -lpulse-simple -lpulse $(LIB_PULSE_EXTRA) 

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V)echo "  [Build $(MODULE_NAME)]:"

ifeq ($(CONFIG_AMBARELLA_PULSEAUDIO_SUPPORT), y)
	@echo "    compile pulseaudio test ...seanhou"
	echo "LINK_PULSE:$(LINK_PULSE)"
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o pacat-simple pacat-simple.c $(INCLUDE_PULSE) $(LINK_PULSE) $(LIB_PULSE)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o parec-simple parec-simple.c $(INCLUDE_PULSE) $(LINK_PULSE) $(LIB_PULSE)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -DPACKAGE -DGETTEXT_PACKAGE -DPACKAGE_VERSION $(INCLUDE_PULSE) $(INCLUDE_PULSE_EXTRA) -o pacat pacat.c $(LINK_PULSE) $(LIB_PULSE)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o aplay aplay.c $(INCLUDE_ALSA) $(LINK_ALSA) $(LIBS)
endif
	@echo "    compile test_audio ..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -c test_audio.cpp $(INCLUDE_ALSA)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -c test_audio_dev.cpp $(INCLUDE_ALSA)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -c audio_encode.cpp
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_audio test_audio.o audio_encode.o $(LINK_ALSA) $(LIBS)
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(UNIT_TEST_CFLAG) -o test_audio_dev test_audio_dev.o audio_encode.o $(LINK_ALSA) $(LIBS)
	@$(AMBA_MAKEFILE_V)rm -f test_audio.o audio_encode.o test_audio_dev.o
	@$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	@$(AMBA_MAKEFILE_V)cp -a test_audio $(UNIT_TEST_PATH)/
	@$(AMBA_MAKEFILE_V)cp -a test_audio_dev $(UNIT_TEST_PATH)/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f test_audio.o audio_encode.o test_audio pacat-simple parec-simple pacat aplay
