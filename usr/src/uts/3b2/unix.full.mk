#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:unix.full.mk	1.19"
MORECPP=-DDEBUG

ROOT = 
DIS = dis
NM = nm
SIZE = size
STRIP = strip
INC=$(ROOT)/usr/include

LIBS= lib.os lib.io lib.vm
NODE = kernel
REL = 
VER = 
NAME = $(NODE)$(VER)

INS=install
INSDIR=.
MKBOOT = mkboot
MASTERD=./master.d
DASHO = -O
CFLAGS= $(DASHO) -I$(INC) -DDEBUG -D_KERNEL -DREM $(MORECPP)
FRC =

LD_KERNEL_ELF = \
	$(LD) -r -o $(NAME) -u bt_availbit -u reclock -u setmask -u fbread \
		-u ls_remove -u segdev_create -u dma_breakup -u dma_pageio \
		locore.o ddi.o name.o syms.o \
		getsizes.o fs/fs.o disp/gendisp.o \
		$(LIBS)

LD_KERNEL_COFF = \
	$(LD) -r -o $(NAME) -u bt_availbit -u reclock -u setmask -u fbread \
		-u  ls_remove -u segdev_create -u dma_breakup -u dma_pageio \
		vuifile start.o gate.o locore.o ddi.o name.o \
		getsizes.o fs/fs.o disp/gendisp.o \
		$(LIBS)

all:FRC machine system vmsys fs exec drivers debug disp netinet rpc ktli des klm boot
	$(MAKE) -f unix.mk NAME=$(NAME) unix

machine:FRC
	cd ml; $(MAKE) -f ml.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

system:FRC
	cd os; $(MAKE) -f os.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

vmsys:FRC
	cd vm; $(MAKE) -f vm.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

exec:FRC
	cd exec; $(MAKE) -f exec.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

drivers:FRC
	cd io; $(MAKE) -f io.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

debug:FRC
	cd debug; $(MAKE) -f debug.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

boot:FRC
	cd boot; $(MAKE) -f boot.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

fs:FRC
	cd fs; $(MAKE) -f fs.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

disp:FRC
	cd disp; $(MAKE) -f disp.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

netinet:FRC
	cd netinet; $(MAKE) -f netinet.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

rpc:FRC
	cd rpc; $(MAKE) -f rpc.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

ktli:FRC
	cd ktli; $(MAKE) -f ktli.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

des:FRC
	cd des; $(MAKE) -f des.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

klm:FRC
	cd klm; $(MAKE) -f klm.mk "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)"

unix:   $(LIBS) vuifile start.o gate.o locore.o ddi.o name.o getsizes.o fs/fs.o disp/gendisp.o $(MASTERD)/kernel
	-rm -f $(NAME)

	if [ x$(CCSTYPE) = xCOFF ] ; \
	then	$(LD_KERNEL_COFF) ; \
	else	$(LD_KERNEL_ELF) ; \
	fi

	$(MKBOOT) -m $(MASTERD) -d . -k $(NAME);
	@echo $(NAME) made.

clean:
	cd ml; $(MAKE) -f ml.mk clean
	cd os; $(MAKE) -f os.mk clean
	cd vm; $(MAKE) -f vm.mk clean
	cd fs; $(MAKE) -f fs.mk clean
	cd exec; $(MAKE) -f exec.mk clean
	cd io; $(MAKE) -f io.mk clean
	cd debug; $(MAKE) -f debug.mk clean
	cd disp; $(MAKE) -f disp.mk clean
	cd netinet; $(MAKE) -f netinet.mk clean
	cd rpc; $(MAKE) -f rpc.mk clean
	cd ktli; $(MAKE) -f ktli.mk clean
	cd des; $(MAKE) -f des.mk clean
	cd klm; $(MAKE) -f klm.mk clean
	cd boot; $(MAKE) -f boot.mk clean
	-rm -f *.o

clobber:
	cd ml; $(MAKE) -f ml.mk clobber
	cd os; $(MAKE) -f os.mk clobber
	cd vm; $(MAKE) -f vm.mk clobber
	cd fs; $(MAKE) -f fs.mk clobber
	cd exec; $(MAKE) -f exec.mk clobber
	cd io; $(MAKE) -f io.mk clobber
	cd debug; $(MAKE) -f debug.mk clobber
	cd disp; $(MAKE) -f disp.mk clobber
	cd netinet; $(MAKE) -f netinet.mk clobber
	cd rpc; $(MAKE) -f rpc.mk clobber
	cd ktli; $(MAKE) -f ktli.mk clobber
	cd des; $(MAKE) -f des.mk clobber
	cd klm; $(MAKE) -f klm.mk clobber
	cd boot; $(MAKE) -f boot.mk clobber
	-rm -f KERNEL $(NAME) $(NAME).* ld.out load.*


install:all
#	$(INS) -f $(INSDIR) "$(NAME)"
	cd boot; $(MAKE) -f boot.mk install "FRC=$(FRC)" "DASHO=$(DASHO)" "MORECPP=$(MORECPP)" "INC=$(INC)" ; \
	wait


FRC:
