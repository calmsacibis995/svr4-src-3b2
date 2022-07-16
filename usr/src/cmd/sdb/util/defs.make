#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#ident	"@(#)sdb:util/defs.make	1.21"

#ROOT 	=
OWN	= bin
GRP	= bin

MACH	= m32

CC	= cc
CPLUS	= ../../util/CC
CFLAGS	= 
CCFLAGS	= $(CFLAGS)
LINK_MODE =
LDLIBS	=
LINT	= lint
LINTFLAGS =
CCSBIN	= $(ROOT)/usr/ccs/bin
CCSLIB	= $(ROOT)/usr/ccs/lib
CMDBASE = ../../..
INS	= $(CMDBASE)/install/install.sh
INSDIR	= $(CCSBIN)
STRIP	= strip

AR	= ar
NM	= nm

BIN	= $(ROOT)/bin
USRBIN	= $(ROOT)/usr/bin
SGSBASE	= $(PRODDIR)/../sgs
COMINC	= $(SGSBASE)/inc/common
MACHINC = $(SGSBASE)/inc/$(MACH)

SHELL	= /bin/sh

PRODDIR = ../..

PRODLIB	= $(PRODDIR)/lib
PRODINC	= $(PRODDIR)/inc
INCCOM	= $(PRODDIR)/inc/common
INCMACH	= $(PRODDIR)/inc/$(MACH)
COMMON	= ../common

INC	= $(ROOT)/usr/include
INCC	= $(PRODDIR)/inc/CC

INCLIST	= -I. -I$(COMMON) -I$(INCMACH) -I$(INCCOM) -I$(MACHINC) -I$(COMINC)
DEFLIST	= -DWEITEK

DFLAGS	=

CC_CMD_FLAGS = $(CFLAGS) $(INCLIST) -I$(INC) $(DEFLIST) $(DFLAGS)
CC_CMD	= $(CC) $(CC_CMD_FLAGS)

CPLUS_CMD_FLAGS = $(CCFLAGS) $(INCLIST) -I$(INCC) -I$(INC) \
							$(DEFLIST) $(DFLAGS)
CPLUS_CMD = $(CPLUS) $(CPLUS_CMD_FLAGS)

ARFLAGS	= qc
YFLAGS	= -ld

# The default target is used if no target is given; it must be first.

default: all
