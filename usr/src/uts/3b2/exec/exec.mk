#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)exec:exec/exec.mk	1.1"
ROOT =
STRIP = strip
INC = $(ROOT)/usr/include
MKBOOT = mkboot
MASTERD = ../master.d
CC = cc

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
DEFLIST =
FRC =

all:	exectypes

exectypes:
	@for i in `ls`;\
	do\
		if [ -d $$i -a -f $$i/$$i.mk ];then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk \"MAKE=$(MAKE)\" \"AS=$(AS)\" \"CC=$(CC)\" \"LD=$(LD)\" \"FRC=$(FRC)\" \"INC=$(INC)\" \"MORECPP=$(MORECPP)\" \"DASHO=$(DASHO)\"";\
		$(MAKE) -f $$i.mk "MAKE=$(MAKE)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)"; \
		cd .. ;; \
		esac;\
		fi;\
	done

clean:
	-rm -f *.o
	@for i in `ls`; \
	do \
		if [ -d $$i -a -f $$i/$$i.mk ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clean; \
			cd ..; \
		fi; \
	done

clobber:	clean
	@for i in `ls`; \
	do \
		if [ -d $$i -a -f $$i/$$i.mk ]; then \
			cd $$i; \
			$(MAKE) -f $$i.mk clobber; \
			cd ..; \
		fi; \
	done
