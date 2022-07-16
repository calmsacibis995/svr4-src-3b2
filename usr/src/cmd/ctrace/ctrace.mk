#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ctrace:ctrace.mk	1.15"
#	ctrace makefile
# installation directories:
ROOT=
CCSBIN=$(ROOT)/usr/ccs/bin
CCSLIB=$(ROOT)/usr/ccs/lib
USRBIN = $(ROOT)/usr/bin
USRLIB = $(ROOT)/usr/lib
CTLIB = $(CCSLIB)/ctrace

CC=cc
DEFLIST = -DRUNTIME=\"$(CTLIB)/runtime.c\"
CFLAGS = -O
INC=$(ROOT)/usr/include
INCSYS=$(ROOT)/usr/include
CC_CMD = $(CC) -c $(CFLAGS) $(DEFLIST) -I$(INC)
LIBELF=
LINK_MODE=

# Must derive y.tab.h from acgram.y
YACC=           yacc
# add -t for yacc debug (YYDEBUG)
# add -v for yacc y.output file
YFLAGS=
YYDEBUG=
YACC_CMD=       $(YACC) $(YFLAGS) -d

LEX= lex
LFLAGS=

LINT= lint
LINTFLAGS=-b

SOURCE	 = constants.h global.h aclex.h \
	   main.c parser.y scanner.l lookup.c trace.c \
	   runtime.c ctcr
CFILES =  main.c parser.c scanner.c lookup.c trace.c
OBJECTS =  main.$o parser.$o scanner.$o lookup.$o trace.$o

all build:	ctrace 

ctrace:		$(OBJECTS:$o=o)
		$(CC) $(CFLAGS) $(OBJECTS:$o=o) $(LINK_MODE) -o $@ 

main.o:		main.c
		if u3b2 || u3b5 || u3b15 ;\
		then \
			$(CC_CMD) '-DTRACEVERS="01.01"' -I../sgs/inc/m32 main.c;\
		elif i386 ;\
		then \
			$(CC_CMD) '-DTRACEVERS="01.01"' -I../sgs/inc/i386 main.c;\
		fi

parser.c:	parser.y
		$(YACC_CMD) parser.y
		mv y.tab.c parser.c
		if cmp -s y.tab.h parser.h; then rm y.tab.h; \
		else cp y.tab.h parser.h; fi

parser.h:	parser.c

scanner.c:	scanner.l
		$(LEX) $(LFLAGS) scanner.l
		mv lex.yy.c scanner.c

parser.o:	parser.c parser.h
		$(CC_CMD) $(YYDEBUG) parser.c

scanner.o:	scanner.c
		$(CC_CMD) scanner.c	

lookup.o:	lookup.c
		$(CC_CMD) lookup.c

trace.o:	trace.c
		$(CC_CMD) trace.c
	
install: 	all
		-rm -f $(CCSBIN)/ctc $(CCSBIN)/ctcr $(CCSBIN)/ctrace
		install -f $(CCSBIN) ctrace
		install -f $(CCSBIN) ctcr
		ln $(CCSBIN)/ctcr $(CCSBIN)/ctc
		if [ ! -d $(CTLIB) ] ;\
		then \
			mkdir $(CTLIB);\
		fi
		install -f $(CTLIB) runtime.c

clean:
		rm -f *.o y.output
		rm -f lint.out
		rm -f *.ln

clobber: 	clean
		rm -f ctrace parser.[ch] scanner.c y.tab.h

lintit:		$(CFILES)
		if u3b2 || u3b5 || u3b15 ;\
		then \
			$(LINT) $(LINTFLAGS) -I../sgs/inc/m32 -I$(INC) $(CFILES) ;\
		elif i386 ;\
		then \
			$(LINT) $(LINTFLAGS) -I../sgs/inc/i386 -I$(INC) $(CFILES) ;\
		fi

ctrace.ln:	$(CFILES)
		rm -f $(OBJECTS:$o=ln)
		if u3b2 || u3b5 || u3b15 ;\
		then \
		$(LINT) $(LINTFLAGS) -c -I../sgs/inc/m32 -I$(INC) $(CFILES);\
		elif i386 ;\
		then \
		$(LINT) $(LINTFLAGS) -c -I../sgs/inc/i386 -I$(INC) $(CFILES);\
		fi
		cat $(OBJECTS:$o=ln) >ctrace.ln
