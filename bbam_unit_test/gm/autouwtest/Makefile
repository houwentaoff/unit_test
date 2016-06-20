

default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /app/ipcam, ,$(PWD)))
AMBABUILD_TOPDIR	?= $(PWD)/../../..

export AMBABUILD_TOPDIR

-include $(AMBABUILD_TOPDIR)/build/app/common.mk
-include $(AMBABUILD_TOPDIR)/.config
-include $(AMBABUILD_TOPDIR)/app/ipcam/mediaserver/config.mk

PRE_SCRIPTS= grepversion.sh
MODULE_SCRIPTS= autotestuw.sh testmulfunc.sh testvercmp.sh testzoompreset.sh testuartcfg.sh testddns.sh testupcfg.sh testminihttp.sh 
EXIST_SCRIPTS=stopall.sh startall.sh downserver.sh atuwoutput.sh testminihttp.sh killproc.sh testsvr.sh callserver.sh \
	avahimonitest.sh cpauto.sh func.sh testinputport.sh testipset.sh testsyslogd.sh testcs.sh testwait.sh  ddns.awk
ifeq ($(BUILD_GMI_PRESET_CRUISE),y)
BUILD_CRUISE_FUNCTION=RunProcessOnStdoutFind
BUILD_CRUISE_ENABLE=enabled
else
BUILD_CRUISE_FUNCTION=RunProcessOnStdoutFindMustFailed
BUILD_CRUISE_ENABLE=disabled
endif

ifeq ($(CONFIG_DDNS_CENTER_INFO),y)
BUILD_DDNS_CENTER_INFO=1
else
BUILD_DDNS_CENTER_INFO=0
endif


ifeq ($(BUILD_GMI_UPLOAD_WARN),y)
CONFIG_GMI_UPLOAD_WARN=1
else
CONFIG_GMI_UPLOAD_WARN=0
endif

NOQUOTE_CONFIG_APP_IPCAM_CONFIG_DIR :=$(shell echo -n "$(CONFIG_APP_IPCAM_CONFIG_DIR)" | sed 's|\x22||g')
_BUILD_APP_IPCAM_CONFIG_DIR :=$(word 2,$(subst rootfs/fakeroot, ,$(NOQUOTE_CONFIG_APP_IPCAM_CONFIG_DIR)))
BUILD_APP_IPCAM_CONFIG_DIR :=$(_BUILD_APP_IPCAM_CONFIG_DIR)/mediaserver

CONFIG_MULFUNC_SET_MUST_EQUAL_NOT=\!\=
CONFIG_MULFUNC_SET_MUST_EQAL=\=\=
CONFIG_APP_FW_VERSION=$(shell ./grepversion.sh $(CONFIG_MULFUNC_VERSION) )
CONFIG_MULFUNC_IS=1
BUILD_GMI_BAUDERATE=9600

.PHONY: all clean

all:$(PRE_SCRIPTS) $(MODULE_SCRIPTS)
	echo "null"
	
clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_SCRIPTS)]:"
	$(AMBA_MAKEFILE_V)rm -f  $(MODULE_SCRIPTS)


autotestuw.sh:autotestuw.sh.tmpl	
	cat $< | sed 's/BUILD_CRUISE_FUNCTION/$(BUILD_CRUISE_FUNCTION)/g' | sed 's/BUILD_CRUISE_ENABLE/$(BUILD_CRUISE_ENABLE)/g' | sed 's|BUILD_APP_IPCAM_CONFIG_DIR|$(BUILD_APP_IPCAM_CONFIG_DIR)|' | sed 's/CONFIG_MULFUNC_IS/$(CONFIG_MULFUNC_IS)/g'  > $@.tmp
	cat $@.tmp |  sed 's|BUILD_DDNS_CENTER_INFO|$(BUILD_DDNS_CENTER_INFO)|g' | sed 's|CONFIG_GMI_UPLOAD_WARN|$(CONFIG_GMI_UPLOAD_WARN)|g' > $@
	rm -f $@.tmp


testmulfunc.sh:testmulfunc.sh.tmpl
	cat $< | sed 's!BUILD_APP_IPCAM_CONFIG_DIR!$(BUILD_APP_IPCAM_CONFIG_DIR)!g'  |sed 's/CONFIG_MULFUNC_SET_MUST_EQUAL_NOT/$(CONFIG_MULFUNC_SET_MUST_EQUAL_NOT)/g' | sed 's/CONFIG_MULFUNC_SET_MUST_EQAL/$(CONFIG_MULFUNC_SET_MUST_EQAL)/g' |  sed 's/CONFIG_APP_FW_VERSION/$(CONFIG_APP_FW_VERSION)/g' >$@

testvercmp.sh:testvercmp.sh.tmpl
	cat $< | sed 's!BUILD_APP_IPCAM_CONFIG_DIR!$(BUILD_APP_IPCAM_CONFIG_DIR)!g' > $@

testzoompreset.sh:testzoompreset.sh.tmpl
	cat $< | sed 's!BUILD_APP_IPCAM_CONFIG_DIR!$(BUILD_APP_IPCAM_CONFIG_DIR)!g' > $@


testuartcfg.sh:testuartcfg.sh.tmpl
	cat $< | sed 's!BUILD_APP_IPCAM_CONFIG_DIR!$(BUILD_APP_IPCAM_CONFIG_DIR)!g' | sed 's!BUILD_GMI_BAUDERATE!$(BUILD_GMI_BAUDERATE)!g' > $@

grepversion.sh:
	echo "null"

testddns.sh:testddns.sh.tmpl
	cat $< | sed 's!BUILD_APP_IPCAM_CONFIG_DIR!$(BUILD_APP_IPCAM_CONFIG_DIR)!g' > $@

testupcfg.sh:testupcfg.sh.tmpl
	cat $< | sed 's!BUILD_APP_IPCAM_CONFIG_DIR!$(BUILD_APP_IPCAM_CONFIG_DIR)!g' > $@

testminihttp.sh:testminihttp.sh.tmpl
	cat $< | sed 's|CONFIG_GMI_UPLOAD_WARN|$(CONFIG_GMI_UPLOAD_WARN)|g' > $@