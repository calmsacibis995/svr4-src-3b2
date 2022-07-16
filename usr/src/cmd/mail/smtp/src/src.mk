#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:smtp/src/src.mk	1.14"
# "@(#)src.mk	1.10 'attmail mail(1) command'"
# To build the SMTP programs:
#
# For a System V Release 3 3B2 running WIN/3B TCP/IP:
#	Define CFLAGS=$(SVR3CFLAGS)
#	   and NETLIB=$(SVR3NETLIB)
#	   and USR_SPOOL=$(SVR3USR_SPOOL)
#	   and XSMTPDOBJ=s5syslog.o
#
# For a `standard' System V Release 4 (using TLI and netdir):
#	Define CFLAGS=$(SVR4CFLAGS)
#	   and NETLIB=$(SVR4NETLIB)
#	   and USR_SPOOL=$(SVR4USR_SPOOL)
#	   and XSMTPDOBJ=
#

INC=		$(ROOT)/usr/include
USR_LIB=	$(ROOT)/usr/lib
COMMONCFLAGS=	-O

# System V Release 3 Flags
SVR3CFLAGS=	-DSVR3 -I../.. $(COMMONCFLAGS) -I$(ROOT)/usr/netinclude
SVR3NETLIB=	-lnet -lnsl_s
SVR3USR_SPOOL=	$(ROOT)/usr/spool
SVR3MAIL=	$(ROOT)/usr/mail
SVR3LDFLAGS=
REALSVR3MAIL=	/usr/mail
REALSVR3USR_SPOOL=	/usr/spool

# System V Release 4 Flags
SVR4CFLAGS=	-Xa -DSVR4 -DTLI -I../.. $(COMMONCFLAGS)
SVR4NETLIB=	-Bdynamic -lnsl -Bstatic -lelf -lc -Bdynamic
#SVR4NETLIB=	-linet -lnsl -lelf
SVR4USR_SPOOL=	$(ROOT)/var/spool
SVR4MAIL=	$(ROOT)/var/mail
SVR4LDFLAGS=	
#SVR4LDFLAGS=	-dy
REALSVR4MAIL=	/var/mail
REALSVR4USR_SPOOL=	/var/spool

CFLAGS=		$(SVR4CFLAGS)
NETLIB=		$(SVR4NETLIB)
USR_SPOOL=	$(SVR4USR_SPOOL)
XSMTPDOBJ=	
LDFLAGS=	$(SVR4LDFLAGS)

STRIP=		$(PFX)strip
INSTALL=	install

# Other possible -D options:
# -DHOOTING for printout of transaction (needed for "Transaction of session")
# -DBSD on 4.[23] just to keep lint happy.
# -DBIND to use resolver routines & resolver gethostbyname()
# -DDEBUG for more smtpd debugging
# -DSOCKET for sockets in smtpd (else use -DINETD, and change SMTPD to in.smtpd)

USR_LIBMAIL=	$(USR_LIB)/mail
MAILSURRCMD=	$(USR_LIBMAIL)/surrcmd
MAIL=		$(SVR4MAIL)
SMTPQ=		$(USR_SPOOL)/smtpq
REALMAILSURRCMD=	/usr/lib/mail/surrcmd
REALMAIL=	$(REALSVR4MAIL)
REALSMTPQ=	$(REALSVR4USR_SPOOL)/smtpq

LIBMAIL=	../../libmail.a
SMTPLIB=	smtplib.a
SMTPLIBOBJS=	$(SMTPLIB)(aux.o) \
		$(SMTPLIB)(config.o) \
		$(SMTPLIB)(dup2.o) \
		$(SMTPLIB)(from822.o) \
		$(SMTPLIB)(from822ad.o) \
		$(SMTPLIB)(genopen.o) \
		$(SMTPLIB)(getlog.o) \
		$(SMTPLIB)(header.o) \
		$(SMTPLIB)(mail.o) \
		$(SMTPLIB)(mx.o) \
		$(SMTPLIB)(netio.o) \
		$(SMTPLIB)(nsysfile.o) \
		$(SMTPLIB)(regcomp.o) \
		$(SMTPLIB)(regerror.o) \
		$(SMTPLIB)(regexec.o) \
		$(SMTPLIB)(s5date.o) \
		$(SMTPLIB)(s5dirsel.o) \
		$(SMTPLIB)(s5mboxown.o) \
		$(SMTPLIB)(s5sysname.o) \
		$(SMTPLIB)(setlogname.o) \
		$(SMTPLIB)(smtplog.o) \
		$(SMTPLIB)(to822.o)

SMTPQER=	smtpqer
SMTPQEROBJ=	smtpqer.o qlib.o

SMTPSCHED=	smtpsched
SMTPSCHEDOBJ=	smtpsched.o qlib.o

FROMSMTP=	fromsmtp
FROMSMTPOBJ=	fromsmtp.o

TOSMTP=		tosmtp
TOSMTPOBJ=	tosmtp.o to822addr.o

SMTP=		smtp
SMTPOBJ=	smtp.o converse.o smtpaux.o to822addr.o

SMTPD=		smtpd
SMTPDOBJ=	smtpd.o conversed.o qlib.o

INSMTPD=	in.smtpd
INSMTPDOBJ=	in.smtpd.o conversed.o

OBJS=		$(SMTPQER) $(SMTPSCHED) $(FROMSMTP) $(TOSMTP)\
		$(SMTP) $(SMTPD)

.c.o: ; $(CC) -c $(CFLAGS) $*.c

all:	$(OBJS)

config.c:
	rm -f config.c
	echo 'char *UPASROOT = "$(REALMAILSURRCMD)/";' >> config.c
	echo 'char *MAILROOT = "$(REALMAIL)/";' >> config.c
	echo 'char *SMTPQROOT = "$(REALSMTPQ)/";' >> config.c

$(SMTPLIB): $(SMTPLIBOBJS)

$(SMTPQER): $(SMTPLIB) $(LIBMAIL) $(SMTPQEROBJ)
	$(CC) $(LDFLAGS) -o $(SMTPQER) $(SMTPQEROBJ) $(SMTPLIB) $(LIBMAIL) $(NETLIB)

$(SMTPSCHED): $(SMTPLIB) $(LIBMAIL) $(SMTPSCHEDOBJ)
	$(CC) $(LDFLAGS) -o $(SMTPSCHED) $(SMTPSCHEDOBJ) $(SMTPLIB) $(LIBMAIL) $(NETLIB)

$(FROMSMTP): $(SMTPLIB) $(LIBMAIL) $(FROMSMTPOBJ)
	$(CC) $(LDFLAGS) -o $(FROMSMTP) $(FROMSMTPOBJ) $(SMTPLIB) $(LIBMAIL)

$(TOSMTP): $(SMTPLIB) $(LIBMAIL) $(TOSMTPOBJ)
	$(CC) $(LDFLAGS) -o $(TOSMTP) $(TOSMTPOBJ) $(SMTPLIB) $(LIBMAIL)

$(SMTP): $(SMTPLIB) $(LIBMAIL) $(SMTPOBJ)
	$(CC) $(LDFLAGS) -o $(SMTP) $(SMTPOBJ) $(SMTPLIB) $(LIBMAIL) $(NETLIB)

smtpd.o: smtpd.c
	$(CC) -c $(CFLAGS) -DSOCKET $*.c

$(SMTPD): $(SMTPLIB) $(LIBMAIL) $(SMTPDOBJ) $(XSMTPDOBJ)
	$(CC) $(LDFLAGS) -o $(SMTPD) $(SMTPDOBJ) $(XSMTPDOBJ) $(SMTPLIB) $(LIBMAIL) $(NETLIB)

in.smtpd.o: smtpd.c
	$(CC) -c $(CFLAGS) -DSOCKET -DINETD smtpd.c
	mv smtpd.o in.smtpd.o

$(INSMTPD): $(INSMTPDOBJ) $(XSMTPDOBJ)
	$(CC) $(LDFLAGS) -o $(INSMTPD) $(INSMTPDOBJ) $(XSMTPDOBJ) $(SMTPLIB) $(LIBMAIL) $(NETLIB)

$(USR_LIBMAIL):
	[ -d $@ ] || mkdir $@
	$(CH)chmod 775 $@
	$(CH)chown root $@
	$(CH)chgrp mail $@

$(MAILSURRCMD): $(USR_LIBMAIL)
	[ -d $@ ] || mkdir $@
	$(CH)chmod 775 $@
	$(CH)chown root $@
	$(CH)chgrp mail $@

$(MAIL):
	[ -d $@ ] || mkdir $@
	$(CH)chmod 775 $@
	$(CH)chown root $@
	$(CH)chgrp mail $@

$(SMTPQ):
	[ -d $@ ] || mkdir $@
	$(CH)chmod 775 $@
	$(CH)chown uucp $@
	$(CH)chgrp mail $@

install: $(OBJS) $(MAILSURRCMD) $(MAIL) $(SMTPQ)
	$(INSTALL) -f $(MAILSURRCMD) -m 2755 -u bin  -g mail ./$(SMTPQER)
	$(INSTALL) -f $(MAILSURRCMD) -m 6755 -u root -g mail ./$(SMTPSCHED)
	$(INSTALL) -f $(MAILSURRCMD) -m 0755 -u bin  -g bin  ./$(FROMSMTP)
	$(INSTALL) -f $(MAILSURRCMD) -m 0755 -u bin  -g bin  ./$(TOSMTP)
	$(INSTALL) -f $(MAILSURRCMD) -m 2755 -u bin  -g mail ./$(SMTP)
	$(INSTALL) -f $(MAILSURRCMD) -m 0755 -u bin  -g bin  ./$(SMTPD)

clean:
	rm -f *.o core config.c

clobber: clean
	rm -f $(SMTPLIB) $(OBJS)

strip:	$(OBJS)
	$(STRIP) $(OBJS)
