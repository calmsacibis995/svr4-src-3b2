#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)boot:boot/boot.fast.mk	11.6"
STRIPOPT = -x -r

ROOT =
DASHO = -O
INC = $(ROOT)/usr/include
#CFLAGS = -I$(INC) -DINKERNEL $(MORECPP)
CFLAGS = $(DASHO) -I$(INC) -D_KERNEL $(MORECPP)
FRC =

all:
	@for i in boot mboot olboot filledt;\
	do\
		cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk \"ROOT=$(ROOT)\" \"MAKE=$(MAKE)\" \"AS=$(AS)\" \"CC=$(CC)\" \"LD=$(LD)\" \"FRC=$(FRC)\" \"INC=$(INC)\" \"MORECPP=$(MORECPP)\" \"DASHO=$(DASHO)\" &";\
		$(MAKE) -f $$i.mk "ROOT=$(ROOT)" "MAKE=$(MAKE)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" &\
		cd .. ;\
	done;\
	wait

install:
	@for i in boot mboot olboot filledt;\
	do\
		cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk install \"ROOT=$(ROOT)\" \"MAKE=$(MAKE)\" \"AS=$(AS)\" \"CC=$(CC)\" \"LD=$(LD)\" \"FRC=$(FRC)\" \"INC=$(INC)\" \"MORECPP=$(MORECPP)\" \"DASHO=$(DASHO)\" &";\
		$(MAKE) -f $$i.mk install "ROOT=$(ROOT)" "MAKE=$(MAKE)" "AS=$(AS)" "CC=$(CC)" "LD=$(LD)" "FRC=$(FRC)" "INC=$(INC)" "MORECPP=$(MORECPP)" "DASHO=$(DASHO)" &\
		cd .. ;\
	done;\
	wait

clean:
	@for i in boot mboot olboot filledt;\
	do\
		cd $$i;\
		$(MAKE) -f $$i.mk clean; \
		cd .. ;\
	done

clobber:
	@for i in boot mboot olboot filledt;\
	do\
		cd $$i;\
		$(MAKE) -f $$i.mk clobber ;\
		cd .. ;\
	done

FRC:
