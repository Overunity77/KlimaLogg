# Embedded-Linux (CAS EBX-FS15)
# Project: Basic Make Exercise 1.0
# Version: 1.0
# File:    Makefile
# Date:    04.10.2013
# Author   Martin Aebersold (AOM1)
#
# Last Modifications: V1.0, AOM1, 04.10.2013
# Initial release

SHELL = /bin/bash

# Include the platform (host or target) enviroments variables
# To create the App for the Host-System run:
# make HOST=1 
ifdef HOST
 include make_env_host
else
 include make_env_target
endif

# Tool names
CROSS_COMPILE	= ${TARGET}-
AS		= $(CROSS_COMPILE)as
AR 		= $(CROSS_COMPILE)ar
CC 		= $(CROSS_COMPILE)gcc
CPP 		= $(CROSS_COMPILE)g++
LD 		= $(CROSS_COMPILE)ld
NM 		= $(CROSS_COMPILE)nm
OBJCOPY 	= $(CROSS_COMPILE)objcopy
OBJDUMP 	= $(CROSS_COMPILE)objdump
RANLIB 		= $(CROSS_COMPILE)ranlib
READELF 	= $(CROSS_COMPILE)readelf
SIZE 		= $(CROSS_COMPILE)size
STRINGS 	= $(CROSS_COMPILE)strings
STRIP 		= $(CROSS_COMPILE)strip
export	AS AR CC CPP LD NM OBJCOPY OBJDUMP RANLIB READELF SIZE STRINGS STRIP

# Build settings
CFLAGS		= ${EXTRA_CFLAGS} -g -gdwarf-2 -Wall
HEADER		= -I${LOCAL_INC} -I${SYSTEM_INC}
LIBS		= -lm
LDFLAGS 	= $(LIBS) -Wl,-Map=sqlite3.map -lpthread -ldl -L${LOCAL_LIB} -L${SYSTEM_LIB}

# Installation variables
EXEC_NAME	= sqlite3
INSTALL		= install
INSTALL_DIR	= ${TARGET_ROOTFS}/usr/local/bin

# Files needed for the build
OBJS 		= shell.o sqlite3.o

# Make rules
all:		sqlite3

sqlite3:	$(OBJS)
			$(CC) -o $(EXEC_NAME) ${OBJS} $(LDFLAGS)

%.o: %.c
		$(CC) -c $(HEADER) $(CFLAGS) $<

install:	sqlite3
		test -d $(INSTALL_DIR) || $(INSTALL) -d -m 755 $(INSTALL_DIR)
		$(INSTALL) -m 755 $(EXEC_NAME) $(IMAGES) $(INSTALL_DIR)

# Generate assembly file *.S
asm:		
ifdef HOST
		@if [ -a $(EXEC_NAME) ]; then objdump -C -D -S -l $(EXEC_NAME) > $(EXEC_NAME).S; \
		else echo "You must first run make!"; fi;
else
		@if [ -a $(EXEC_NAME) ]; then arm-linux-objdump -C -D -S -l $(EXEC_NAME) > $(EXEC_NAME).S; \
		else echo "You must first run make!"; fi;
endif

doc:
		doxygen

clean:
		rm -f *.o $(EXEC_NAME)
distclean:
		rm -f *~
		rm -f *.S
		rm -f *.map
		rm -f *.o $(EXEC_NAME) 
		rm -r doc

