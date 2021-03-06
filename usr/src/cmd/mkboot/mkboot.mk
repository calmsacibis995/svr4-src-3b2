#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mkboot-3b2:mkboot.mk	1.6.6.1"
INCSYS = $(ROOT)/usr/include
SOURCES = $(INCSYS)/sys/boothdr.h mkboot.h xmkboot.l mkboot.y main.c util.c
OBJECTS = mkboot.o main.o util.o
LEXLIB = -ll -lld -lelf
SYMLINK = :
CFLAGS = -s -O -I$(INCSYS)
#CFLAGS = -g -N -I$(INCSYS) -DYYDEBUG=1 -DLEXDEBUG=1
PR = xcl -x -2 -pmini
INS = install

mkboot:	$(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LEXLIB) -o mkboot $(NOSHLIBS)

cross:	
	@if [ "$(CH)" = "#" ] ; \
	then	/bin/make -f mkboot.mk CH= CFLAGS="-I/usr/include -DCROSS $(CFLAGS)" CC=cc ROOT=$(ROOT) LEXLIB="-ll -lld $(CCSLIB)/libelf$(PFX).a" ; \
	fi

install:mkboot
	-rm -f $(ROOT)/etc/mkboot
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g root mkboot
	$(SYMLINK) /usr/sbin/mkboot $(ROOT)/etc/mkboot

main.o:	mkboot.h $(INCSYS)/sys/boothdr.h
util.o:	mkboot.h $(INCSYS)/sys/boothdr.h
mkboot.o:lex.yy.c mkboot.h $(INCSYS)/sys/boothdr.h

lex.yy.c:xmkboot.l
	$(LEX) xmkboot.l

clobber:clean
	rm -f mkboot

clean:
	rm -f lex.yy.c *.o tags cscope.out

print:	mkboot.mk $(SOURCES)
	$(PR) $?
	touch print

.PRECIOUS: print

lint:	main.c util.c mkboot.y lex.yy.c
	$(YACC) mkboot.y
	lint -b $(CFLAGS) main.c util.c y.tab.c
	rm -f y.tab.c

tags:	$(SOURCES)
	ctags $(SOURCES)

cscope.out:$(SOURCES)
	@-if [ "$(RUN)" ]; \
	then \
		cscope -I$(INCSYS) $(SOURCES) \
	else \
		cscope -I$(INCSYS) $(SOURCES) </dev/null \
	fi
