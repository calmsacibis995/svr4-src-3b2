#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)devintf:mkdtab/mkdtab.mk	1.1"
INSDIR = $(ROOT)/usr/sadm/sysadm/bin
INC = $(ROOT)/usr/include
LDFLAGS = -s  -lxedt -ladm $(SHLIBS)
CFLAGS = -O -I$(INC)
INS = install

all: mkdtab

mkdtab: mkdtab.c\
	$(INC)/stdio.h\
	$(INC)/stdlib.h\
	$(INC)/string.h\
	$(INC)/fcntl.h\
	$(INC)/sys/types.h\
	$(INC)/unistd.h\
	$(INC)/devmgmt.h\
	$(INC)/sys/mkdev.h\
	$(INC)/sys/edt.h\
	$(INC)/sys/libxedt.h\
	$(INC)/sys/stat.h\
	$(INC)/sys/vtoc.h\
	$(INC)/sys/vfstab.h\
	$(INC)/sys/fs/s5param.h\
	$(INC)/sys/fs/s5filsys.h
	$(CC) $(CFLAGS) mkdtab.c -o mkdtab $(LDFLAGS)

install: all
	@if [ ! -d $(INSDIR) ]; \
		then \
		mkdir $(INSDIR); \
		fi;
	$(INS) -f $(INSDIR) -u bin -g bin -m 555 mkdtab

clean:
	rm -f *.o

clobber: clean
	rm -f mkdtab
