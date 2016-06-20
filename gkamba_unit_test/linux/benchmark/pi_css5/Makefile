# Makefile for pi_fftcs5, Takuyo Ooura's
# Pi calculation for *nix or DOS
# Warning: this file has tabs in it.  It is required for GNU Make.
#
# This version may not produce optimal runs.  You'll need to experiment
# with the compiler options.
PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/linux/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "Misc Test"
export AMBABUILD_TOPDIR
include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk
CC=@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc
AS=@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)as
STRIP=@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)strip

targets=pi_css5

ifeq ($(BUILD_AMBARELLA_UNIT_CODE_OPT_NEON), y)
targets += pi_css5_soft
endif

all: $(targets)
	@echo "    compile CPU PI_CSS5 test ..."
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a pi_css5 $(UNIT_TEST_PATH)/
ifeq ($(BUILD_AMBARELLA_UNIT_CODE_OPT_NEON), y)
	$(AMBA_MAKEFILE_V)cp -a pi_css5_soft $(UNIT_TEST_PATH)/
endif

#Set this to the proper math libraries to link against
MATH_LIBS= -lm

#Set this to best optimization flags for your system. Defaults are for gcc.
CFLAGS = -pedantic -fomit-frame-pointer -funroll-loops

#Set this to the cpu to target
CFLAGS += $(UNIT_TEST_CFLAG)

#If you would like to minimize program output uncomment this line
#CFLAGS += -DQUIET_OUT

#Set this to important linker flags
LDFLAGS = -static

# Leave items below this point unchanged!

pi_css5 : pi_fftcs.o fftsg_h.o
	$(CC) $(CFLAGS) $(UNIT_TEST_CFLAG) pi_fftcs.o fftsg_h.o ${MATH_LIBS} ${LDFLAGS} -o pi_css5
	${STRIP} pi_css5

pi_fftcs.o : pi_fftcs.c
	$(CC) $(CFLAGS) $(UNIT_TEST_CFLAG) -c pi_fftcs.c -o pi_fftcs.o

fftsg_h.o : fftsg_h.c
	$(CC) $(CFLAGS) $(UNIT_TEST_CFLAG) -c fftsg_h.c -o fftsg_h.o

ifeq ($(BUILD_AMBARELLA_UNIT_CODE_OPT_NEON), y)
pi_css5_soft: pi_fftcs_soft.o fftsg_h_soft.o
	$(CC) $(CFLAGS) -march=armv7-a -mlittle-endian -msoft-float pi_fftcs_soft.o fftsg_h_soft.o ${MATH_LIBS} ${LDFLAGS} -o pi_css5_soft
	${STRIP} pi_css5_soft

pi_fftcs_soft.o: pi_fftcs.c
	$(CC) $(CFLAGS) -march=armv7-a -mlittle-endian -msoft-float -c pi_fftcs.c -o pi_fftcs_soft.o

fftsg_h_soft.o : fftsg_h.c
	$(CC) $(CFLAGS) -march=armv7-a -mlittle-endian -msoft-float -c fftsg_h.c -o fftsg_h_soft.o
endif

clean:
	@$(AMBA_MAKEFILE_V)rm -f pi_fftcs.o fftsg_h.o pi_css5

