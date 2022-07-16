#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)adm:.adm.mk	1.18.1.13"

ROOT =
LIB = $(ROOT)/usr/lib
CRONTABS = $(ROOT)/var/spool/cron/crontabs
SYMLINK = :
LIBCRON = $(ROOT)/etc/cron.d
INSDIR = $(ROOT)/etc
TOUCH=touch

CRON_ENT= adm root sys sysadm

CRON_LIB= .proto at.allow cron.allow queuedefs

ETC_SCRIPTS= bcheckrc brc checkall checklist filesave gettydefs \
	group ioctl.syscon master motd passwd powerfail \
	rc shadow shutdown system system.mtc11 system.mtc12 \
	system.un32 tapesave ttysrch

all:	etc_scripts crontab cronlib

crontab: $(CRON_ENT)

cronlib: $(CRON_LIB)

etc_scripts: $(ETC_SCRIPTS)

clean:

clobber: clean

install:
	-mkdir $(ROOT)/var/spool/cron
	-mkdir $(ROOT)/var/spool/crontabs
	-mkdir $(ROOT)/usr/lib/cron
	$(MAKE) -f .adm.mk $(ARGS)

adm::
	-rm -f $(ROOT)/usr/spool/cron/crontabs/adm
	cp adm $(CRONTABS)/adm
	$(CH)chmod 644 $(CRONTABS)/adm
	$(CH)chgrp sys $(CRONTABS)/adm
	$(TOUCH) 0101000070 $(CRONTABS)/adm
	$(CH)chown root $(CRONTABS)/adm
	$(SYMLINK) $(CRONTABS)/adm $(ROOT)/usr/spool/cron/crontabs/adm

root::
	-rm -f $(ROOT)/usr/spool/cron/crontabs/root
	cp root $(CRONTABS)/root
	$(CH)chmod 644 $(CRONTABS)/root
	$(CH)chgrp sys $(CRONTABS)/root
	$(TOUCH) 0101000070 $(CRONTABS)/root
	$(CH)chown root $(CRONTABS)/root
	$(SYMLINK) $(CRONTABS)/root $(ROOT)/usr/spool/cron/crontabs/root

sys::
	-rm -f $(ROOT)/usr/spool/cron/crontabs/sys
	cp sys $(CRONTABS)/sys
	$(CH)chmod 644 $(CRONTABS)/sys
	$(CH)chgrp sys $(CRONTABS)/sys
	$(TOUCH) 0101000070 $(CRONTABS)/sys
	$(CH)chown root $(CRONTABS)/sys
	$(SYMLINK) $(CRONTABS)/sys $(ROOT)/usr/spool/cron/crontabs/sys

sysadm::
	-rm -f $(ROOT)/usr/spool/cron/crontabs/sysadm
	cp sysadm $(CRONTABS)/sysadm
	$(CH)chmod 644 $(CRONTABS)/sysadm
	$(CH)chgrp sys $(CRONTABS)/sysadm
	$(TOUCH) 0101000070 $(CRONTABS)/sysadm
	$(CH)chown root $(CRONTABS)/sysadm
	$(SYMLINK) $(CRONTABS)/sysadm $(ROOT)/usr/spool/cron/crontabs/sysadm

.proto::
	-rm -f $(ROOT)/usr/lib/cron/.proto
	cp .proto $(LIBCRON)/.proto
	$(CH)chmod 744 $(LIBCRON)/.proto
	$(CH)chgrp sys $(LIBCRON)/.proto
	$(TOUCH) 0101000070 $(LIBCRON)/.proto
	$(CH)chown root $(LIBCRON)/.proto
	$(SYMLINK) $(LIBCRON)/.proto $(ROOT)/usr/lib/cron/.proto

at.allow::
	-rm -f $(ROOT)/usr/lib/cron/at.allow
	cp at.allow $(LIBCRON)/at.allow
	$(CH)chmod 644 $(LIBCRON)/at.allow
	$(CH)chgrp sys $(LIBCRON)/at.allow
	$(TOUCH) 0101000070 $(LIBCRON)/at.allow
	$(CH)chown root $(LIBCRON)/at.allow
	$(SYMLINK) $(LIBCRON)/at.allow $(ROOT)/usr/lib/cron/at.allow

cron.allow::
	-rm -f $(ROOT)/usr/lib/cron/cron.allow
	cp cron.allow $(LIBCRON)/cron.allow
	$(CH)chmod 644 $(LIBCRON)/cron.allow
	$(CH)chgrp sys $(LIBCRON)/cron.allow
	$(TOUCH) 0101000070 $(LIBCRON)/cron.allow
	$(CH)chown root $(LIBCRON)/cron.allow
	$(SYMLINK) $(LIBCRON)/cron.allow $(ROOT)/usr/lib/cron/cron.allow

queuedefs::
	-rm -f $(ROOT)/usr/lib/cron/queuedefs
	cp queuedefs $(LIBCRON)/queuedefs
	$(CH)chmod 644 $(LIBCRON)/queuedefs
	$(CH)chgrp sys $(LIBCRON)/queuedefs
	$(TOUCH) 0101000070 $(LIBCRON)/queuedefs
	$(CH)chown root $(LIBCRON)/queuedefs
	$(SYMLINK) $(LIBCRON)/queuedefs $(ROOT)/usr/lib/cron/queuedefs


bcheckrc::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		-rm $(INSDIR)/bcheckrc;\
		cp bcheckrc.sh $(ROOT)/sbin/bcheckrc;\
		$(CH)chmod 744 $(ROOT)/sbin/bcheckrc;\
		$(CH)chgrp sys $(ROOT)/sbin/bcheckrc;\
		$(TOUCH) 0101000070 $(ROOT)/sbin/bcheckrc;\
		$(CH)chown root $(ROOT)/sbin/bcheckrc;\
		cp $(ROOT)/sbin/bcheckrc $(ROOT)/usr/sbin/bcheckrc;\
		$(SYMLINK) $(ROOT)/sbin/bcheckrc $(INSDIR)/bcheckrc;\
	fi

brc::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		-rm $(INSDIR)/brc;\
		cp brc.sh $(ROOT)/sbin/brc;\
		$(CH)chmod 744 $(ROOT)/sbin/brc;\
		$(CH)chgrp sys $(ROOT)/sbin/brc;\
		$(TOUCH) 0101000070 $(ROOT)/sbin/brc;\
		$(CH)chown root $(ROOT)/sbin/brc;\
		cp $(ROOT)/sbin/brc $(ROOT)/usr/sbin/brc;\
		$(SYMLINK) $(ROOT)/sbin/brc $(INSDIR)/brc;\
	fi

checkall::
	-if vax || pdp11 || u3b15 || u3b;\
	then\
		cp checkall.sh $(INSDIR)/checkall;\
		$(CH)chmod 744 $(INSDIR)/checkall;\
		$(CH)chgrp bin $(INSDIR)/checkall;\
		$(TOUCH) 0101000070 $(INSDIR)/checkall;\
		$(CH)chown bin $(INSDIR)/checkall;\
	fi

checklist::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		cp checklist $(INSDIR)/checklist;\
		$(CH)chmod 664 $(INSDIR)/checklist;\
		$(CH)chgrp sys $(INSDIR)/checklist;\
		$(TOUCH) 0101000070 $(INSDIR)/checklist;\
		$(CH)chown root $(INSDIR)/checklist;\
	fi

filesave::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		cp filesave.sh $(INSDIR)/filesave;\
		$(CH)chmod 744 $(INSDIR)/filesave;\
		$(CH)chgrp sys $(INSDIR)/filesave;\
		$(TOUCH) 0101000070 $(INSDIR)/filesave;\
		$(CH)chown root $(INSDIR)/filesave;\
	fi

gettydefs::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
	cp gettydefs $(INSDIR)/gettydefs;\
	$(CH)chmod 644 $(INSDIR)/gettydefs;\
	$(CH)chgrp sys $(INSDIR)/gettydefs;\
	$(TOUCH) 0101000070 $(INSDIR)/gettydefs;\
	$(CH)chown root $(INSDIR)/gettydefs;\
	fi

group::
	cp group $(INSDIR)/group
	$(CH)chmod 644 $(INSDIR)/group
	$(CH)chgrp sys $(INSDIR)/group
	$(TOUCH) 0101000070 $(INSDIR)/group
	$(CH)chown root $(INSDIR)/group

ioctl.syscon::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
	else cd u3b2;\
	fi;\
	cp ioctl.syscon $(INSDIR)/ioctl.syscon;\
	$(CH)chmod 644 $(INSDIR)/ioctl.syscon;\
	$(CH)chgrp sys $(INSDIR)/ioctl.syscon;\
	$(TOUCH) 0101000070 $(INSDIR)/ioctl.syscon;\
	$(CH)chown root $(INSDIR)/ioctl.syscon

master::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		cp master $(INSDIR)/master;\
		$(CH)chmod 644 $(INSDIR)/master;\
		$(CH)chgrp sys $(INSDIR)/master;\
		$(CH)chown root $(INSDIR)/master;\
	fi

motd::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
	else cd u3b2;\
	fi;\
	sed 1d motd > $(INSDIR)/motd;\
	$(CH)chmod 644 $(INSDIR)/motd;\
	$(CH)chgrp sys $(INSDIR)/motd;\
	$(CH)chown root $(INSDIR)/motd

passwd::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
	else cd u3b2;\
	fi;\
	cp passwd $(INSDIR)/passwd;\
	$(CH)chmod 644 $(INSDIR)/passwd;\
	$(CH)chgrp sys $(INSDIR)/passwd;\
	$(TOUCH) 0101000070 $(INSDIR)/passwd;\
	$(CH)chown root $(INSDIR)/passwd

shadow::
	-if u3b2;\
	then cd u3b2;\
		cp shadow $(INSDIR)/shadow;\
		$(CH)chmod 400 $(INSDIR)/shadow;\
		$(CH)chgrp sys $(INSDIR)/shadow;\
		$(TOUCH) 0101000070 $(INSDIR)/shadow;\
		$(CH)chown root $(INSDIR)/shadow;\
	fi

powerfail::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		cp powerfail.sh $(INSDIR)/powerfail;\
		$(CH)chmod 744 $(INSDIR)/powerfail;\
		$(CH)chgrp sys $(INSDIR)/powerfail;\
		$(TOUCH) 0101000070 $(INSDIR)/powerfail;\
		$(CH)chown root $(INSDIR)/powerfail;\
	fi

rc::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		cp rc.sh $(INSDIR)/rc;\
		$(CH)chmod 744 $(INSDIR)/rc;\
		$(CH)chgrp sys $(INSDIR)/rc;\
		$(TOUCH) 0101000070 $(INSDIR)/rc;\
		$(CH)chown root $(INSDIR)/rc;\
	fi

shutdown::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		-rm $(INSDIR)/shutdown;\
		cp shutdown.sh $(ROOT)/sbin/shutdown;\
		$(CH)chmod 744 $(ROOT)/sbin/shutdown;\
		$(CH)chgrp sys $(ROOT)/sbin/shutdown;\
		$(TOUCH) 0101000070 $(ROOT)/sbin/shutdown;\
		$(CH)chown root $(ROOT)/sbin/shutdown;\
		cp $(ROOT)/sbin/shutdown $(ROOT)/usr/sbin/shutdown;\
		$(SYMLINK) $(ROOT)/sbin/shutdown $(INSDIR)/shutdown;\
	fi

tapesave::
	-if vax;\
	then cd vax;\
	elif pdp11;\
	then cd pdp11;\
	elif u3b15;\
	then cd u3b15;\
	elif u3b;\
	then cd u3b;\
		cp tapesave.sh $(INSDIR)/tapesave;\
		$(CH)chmod 744 $(INSDIR)/tapesave;\
		$(CH)chgrp sys $(INSDIR)/tapesave;\
		$(TOUCH) 0101000070 $(INSDIR)/tapesave;\
		$(CH)chown root $(INSDIR)/tapesave;\
	fi

system::
	-if u3b;\
	then cd u3b;\
		cp system $(INSDIR)/system;\
		$(CH)chmod 644 $(INSDIR)/system;\
		$(CH)chgrp sys $(INSDIR)/system;\
		$(TOUCH) 0101000070 $(INSDIR)/system;\
		$(CH)chown root $(INSDIR)/system;\
	fi

system.mtc11::
	-if u3b;\
	then cd u3b;\
		cp system.32 $(INSDIR)/system.mtc11;\
		$(CH)chmod 644 $(INSDIR)/system.mtc11;\
		$(CH)chgrp sys $(INSDIR)/system.mtc11;\
		$(TOUCH) 0101000070 $(INSDIR)/system.mtc11;\
		$(CH)chown root $(INSDIR)/system.mtc11;\
	fi

system.mtc12::
	-if u3b;\
	then cd u3b;\
		cp system.32 $(INSDIR)/system.mtc12;\
		$(CH)chmod 644 $(INSDIR)/system.mtc12;\
		$(CH)chgrp sys $(INSDIR)/system.mtc12;\
		$(TOUCH) 0101000070 $(INSDIR)/system.mtc12;\
		$(CH)chown root $(INSDIR)/system.mtc12;\
	fi

system.un32::
	-if u3b;\
	then cd u3b;\
		cp system.32 $(INSDIR)/system.un32;\
		$(CH)chmod 644 $(INSDIR)/system.un32;\
		$(CH)chgrp sys $(INSDIR)/system.un32;\
		$(TOUCH) 0101000070 $(INSDIR)/system.un32;\
		$(CH)chown root $(INSDIR)/system.un32;\
	fi

ttysrch::
	cp ttysrch $(INSDIR)/ttysrch
	$(CH)chmod 644 $(INSDIR)/ttysrch
	$(CH)chgrp sys $(INSDIR)/ttysrch
	$(TOUCH) 0101000070 $(INSDIR)/ttysrch
	$(CH)chown root $(INSDIR)/ttysrch

