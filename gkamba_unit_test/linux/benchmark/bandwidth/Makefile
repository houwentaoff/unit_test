#============================================================================
# bandwidth 0.23, a benchmark to estimate memory transfer bandwidth.
# Copyright (C) 2005-2010 by Zack T Smith.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# The author may be reached at fbui@comcast.net.
#============================================================================

#CFLAGS= -O6
#CC=arm-none-linux-gnueabi-gcc
#AS=arm-none-linux-gnueabi-as



SRC=main.c BMP.c

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /unit_test/linux/, ,$(PWD)))
PRIVATE_DIR		:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/$(PRIVATE_DIR)/../..
MODULE_NAME		:= "Misc Test"
export AMBABUILD_TOPDIR
include $(AMBABUILD_TOPDIR)/build/unit_test/common.mk
CC=@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc
AS=@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)as
CFLAGS += $(UNIT_TEST_CFLAG)

message:
	@echo ""
	@echo "To compile for x86 Linux:          make bandwidth32"
	@echo "To compile for x86_64 Linux:       make bandwidth64"
	@echo "To compile for x86 Mac OS/X:       make bandwidth-mac32"
	@echo "To compile for x86_64 Mac OS/X:    make bandwidth-mac64"
	@echo "To compile for x86 Win32/Cygwin:   make bandwidth-win32"
	@echo "To compile for ARM Linux:          make bandwidth-arm"
	@echo "To compile for ARM Windows Mobile: make bandwidth-winmo"
	@echo ""

bandwidth64:	main.c routines64.asm
	nasm -f elf64 routines64.asm -o routines64.o
	${CC} -g routines64.o ${SRC} ${CFLAGS} -o bandwidth64

bandwidth32:	main.c routines32.asm 
	nasm -f elf routines32.asm -o routines32.o
	${CC} -mtune=pentium routines32.o ${SRC} ${CFLAGS} -o bandwidth32

bandwidth-mac64:	main.c routines64.asm
	nasm -f macho64 routines64.asm -o routines64.o
	${CC} -Wall routines64.o ${SRC} ${CFLAGS} -o bandwidth-mac64

bandwidth-mac32:	main.c routines32.asm
	nasm -f macho32 routines32.asm -o routines32.o
	${CC} -m32 routines32.o ${SRC} ${CFLAGS} -o bandwidth-mac32

bandwidth-win32:	main.c routines32.asm 
	nasm -f win32 routines32.asm -o routines32.o
	gcc -Wall -O6 -D__WIN32__ -DWINVER=0x0600 routines32.o ${SRC} -o bandwidth-win32
	
all:
	@echo "    compile mem bandwidth test ..."
	${AS} routinesARM.asm -o routinesARM.o
	${CC} routinesARM.o ${SRC} ${CFLAGS} -o bandwidth-arm
	$(AMBA_MAKEFILE_V)mkdir -p $(UNIT_TEST_PATH)/
	$(AMBA_MAKEFILE_V)cp -a bandwidth-arm $(UNIT_TEST_PATH)/

bandwidth-arm:	main.c routinesARM.asm
	${AS} routinesARM.asm -o routinesARM.o
	${CC} routinesARM.o ${SRC} ${CFLAGS} -o bandwidth-arm

bandwidth-arm-test:	main.c routinesARM.asm
	arm-mingw32ce-as routinesARM.asm -mcpu=xscale -o routinesARM.o
	arm-mingw32ce-gcc routinesARM.o ${SRC} ${CFLAGS} -o bandwidth-arm

# Windows Mobile
bandwidth-winmo:	main.c 
	arm-mingw32ce-as routinesARM.asm -o routinesARM.o
	arm-mingw32ce-gcc routinesARM.o -D__CYGWIN__ ${SRC} ${CFLAGS} -o bandwidth-winmo

clean:
	rm -f bandwidth bandwidth32 bandwidth64 bandwidth-arm bandwidth-winmo.exe routines32.o routines64.o routinesARM.o bandwidth-win32.exe bandwidth.bmp

