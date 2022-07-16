#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)col:col.mk	1.2.1.1"

DIR = $(ROOT)/usr/bin
INC = $(ROOT)/usr/include
LDFLAGS = -s
CFLAGS = -O -I$(INC)
INS = install
FILE = col

all: $(FILE)

$(FILE): $(FILE).o
	$(CC) $(CFLAGS) -o $@ $@.o $(LDFLAGS) $(PERFLIBS)

install: all
	$(INS) -f $(DIR) -m 555 -u bin -g bin $(FILE)

clean:
	rm -rf *.o

clobber: clean
	rm -f $(FILE)
