default:all

AMBABUILD_TOPDIR  = $(word 1, $(subst /unit_test, ,$(shell pwd)))
MODULE_NAME		:= testMw
MODULE_SRCS = testMwSafe.cpp
MODULE_HDRS = $(wildcard *.h)
MODULE_OBJS = $(MODULE_SRCS:%.cpp=%.o)

export AMBABUILD_TOPDIR

-include $(AMBABUILD_TOPDIR)/build/app/common.mk
-include $(AMBABUILD_TOPDIR)/.config
-include $(AMBABUILD_TOPDIR)/app/ipcam/mediaserver/config.mk

CROSS_COMPILE=
ifeq ($(CROSS_COMPILE),)
LINK_FLAG =
MODULE_INC = 
CFLAGS= -Wall -g
else
LINK_FLAG = -L $(MW_LIB_DIR) \
			-L $(SYS_LIB_DIR) \
			$(MW_OSA_LIB_LD) \
			$(MW_PROFILE_LIB_LD) \
			$(MW_PTZ_LIB_LD) \
			$(SYS_LIB_LD) \
			$(MW_LIB_LD) 


MODULE_INC = -I$(AMBABUILD_TOPDIR)/build/include \
			-I$(AMBABUILD_TOPDIR)/build/include/arch \
			-I$(AMBABUILD_TOPDIR) \
			-I$(AMBABUILD_TOPDIR)/mw/amp/include \
			-I$(AMBABUILD_TOPDIR)/mw/osa/inc \
			-I$(AMBABUILD_TOPDIR)/mw/Profile/inc \
			-I$(AMBABUILD_TOPDIR)/mw/PtzController/inc \
			-I.

CFLAGS = -march=armv6 -mtune=arm1136j-s $(MODULE_INC) $(APP_FLAG) -Wall -O1 -DLINUX -D__LINUX__ -fno-rtti  #-g -fno-exceptions -nostdinc++ -DDEBUG_LOCK -DDEBUG_THREAD 
endif
#CFLAGS = $(MODULE_INC) $(APP_FLAG) -Wall -O1 -DLINUX -D__LINUX__ -fno-rtti  #-g -fno-exceptions -nostdinc++ -DDEBUG_LOCK -DDEBUG_THREAD 
#CFLAGS= -Wall -g
ifeq ($(BUILD_GMI_PRESET_CRUISE), y)
CFLAGS += -DPRESET_CRUISE_ENABLE
endif

CONFIG_PATH = $(IPCAM_CONFIG_PATH)/mediaserver
# ccz 2011/8/8 +
DEFAULT_CONFIG_PATH = $(IPCAM_CONFIG_PATH)/mediaserver/default


%.o:	%.cpp
	@echo "      compiling $<..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(CFLAGS) -c $< -o $@

all:
	@echo "    compile $(MODULE_NAME)..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(CFLAGS) -o $(MODULE_NAME) $(MODULE_SRCS) $(LINK_FLAG)

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)rm -f $(MODULE_NAME)  $(MODULE_OBJS)
	


