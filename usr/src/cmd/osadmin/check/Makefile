#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)osadmin:check/Makefile	1.1"
#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Makefile for check routines.

DIR = $(ROOT)/usr/lbin

INC = $(ROOT)/usr/include

LDFLAGS = ../csub/csub.a $(LDLIBS)

CFLAGS = -O -I ../include -I $(INC)

STRIP = strip

EVERYTHING:	ALLPARTS ALL

#top#
# Generated by makefile 1.41    Thu Mar 22 21:59:38 EST 1984


MAINS = checklist checkre checkyn

OBJECTS =  check.o checklist.o checkre.o checkyn.o regerror.o rightlist.o \
	sesystem.o

SOURCES =  check.c checklist.c checkre.c checkyn.c regerror.c rightlist.c \
	sesystem.c

ALL:		$(MAINS)

checklist:	checklist.o check.o rightlist.o	sesystem.o 
	$(CC) $(CFLAGS)  -o checklist  checklist.o check.o rightlist.o \
	sesystem.o   $(LDFLAGS)

checkre:	checkre.o check.o regerror.o rightlist.o \
		sesystem.o 
	$(CC) $(CFLAGS)  -o checkre  checkre.o check.o regerror.o \
	rightlist.o sesystem.o   $(LDFLAGS)

checkyn:	checkyn.o check.o sesystem.o 
	$(CC) $(CFLAGS)  -o checkyn  checkyn.o check.o sesystem.o \
	$(LDFLAGS)


check.o:	 check.h $(INC)/ctype.h $(INC)/signal.h \
		 $(INC)/sys/signal.h $(INC)/stdio.h	\
		 ../include/stddef.h 

checklist.o:	 $(INC)/stdio.h $(INC)/string.h check.h \
		 ../include/stddef.h 

checkre.o:	 $(INC)/stdio.h check.h ../include/cmderr.h \
		 ../include/stddef.h ../include/switch.h \
		 $(INC)/regexp.h 

checkyn.o:	 $(INC)/stdio.h check.h ../include/stddef.h 

regerror.o:	 

rightlist.o:	 check.h $(INC)/stdio.h $(INC)/string.h 

sesystem.o:	 $(INC)/signal.h $(INC)/sys/signal.h 

GLOBALINCS = ../include/cmderr.h ../include/stddef.h ../include/switch.h \
	$(INC)/ctype.h $(INC)/regexp.h $(INC)/signal.h \
	$(INC)/stdio.h $(INC)/string.h $(INC)/sys/signal.h

LOCALINCS = check.h 

clean:
	rm -f $(OBJECTS)
	
clobber:	
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile  -m -f Makefile 
#bottom#
	echo '1,/^newmakefile/s;$(INC)/;\044(INC)/;g\nw\nq'  | \
		ed - Makefile

install:	$(MAINS) $(DIR) remove
	cp $(MAINS) $(DIR)
	cd $(DIR);  $(STRIP) $(MAINS);  chmod +x,u+w,go-w $(MAINS)

remove:
	cd $(DIR);  rm -f $(MAINS)

$(DIR):
	mkdir $(DIR);  chmod 775 $(DIR)

#	Check to see if everything is here.
ALLPARTS:	$(GLOBALINCS) $(LOCALINCS) $(SOURCES)

partslist:
	@echo Makefile $(LOCALINCS) $(SOURCES)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed 's;^;$(DIR)/;'

productdir:
	@echo $(DIR)

srcaudit:	# will not report missing nor present object or product files.
	@(	find * -print;  \
		ls -d Makefile $(LOCALINCS) $(SOURCES) $(OBJECTS) $(MAINS) \
			$(OBJECTS) $(MAINS) 2>&1  \
	)  |  sort  |  uniq -u  |  sed '/ /!s/$$/	unexpected/'
