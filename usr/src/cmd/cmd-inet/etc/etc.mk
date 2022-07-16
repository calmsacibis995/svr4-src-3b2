#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:etc/etc.mk	1.8"

ETC=		$(ROOT)/etc
INITD=		$(ROOT)/etc/init.d
INET=		$(ETC)/inet
STARTINET=	$(ETC)/rc2.d/S69inet
STOPINET1=	$(ETC)/rc1.d/K69inet
STOPINET0=	$(ETC)/rc0.d/K69inet
INSTALL=	install
SYMLINK=	ln -s

FILES=		hosts inetd.conf named.boot networks protocols rc.inet \
		services strcf

all:		$(FILES)

install:	all
		@if [ ! -d $(INET) ];\
		then\
			mkdir $(INET);\
		fi;\
		for i in $(FILES);\
		do\
			$(INSTALL) -f $(INET) -m 0444 -u root -g sys $$i;\
			rm -f $(ETC)/$$i;\
			$(SYMLINK) $(INET)/$$i $(ETC)/$$i;\
		done;\
		cd init.d;\
		$(INSTALL) -f $(INITD) -m 0444 -u root -g sys inetinit;\
		rm -f $(STARTINET) $(STOPINET1) $(STOPINET0);\
		ln $(INITD)/inetinit $(STARTINET);\
		ln $(INITD)/inetinit $(STOPINET1);\
		ln $(INITD)/inetinit $(STOPINET0)

clean:

clobber:
