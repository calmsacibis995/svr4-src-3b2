#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)osadmin3b2:admin/Makefile	1.2"
#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Makefile for admin
#	Generated Tue Apr 23 19:01:19 EST 1985 from skeleton makefile:
#ident  "sadmin3b2:admin/SKELMakefile  1.2"

ADMINHOME = $(ROOT)/usr/admin

CMDS = 

DIRECTORIES = menu menu/diskmgmt menu/diskmgmt/harddisk menu/filemgmt \
menu/filemgmt/bupsched

PARTS = $(CMDS) bupsched menu/diskmgmt/format menu/diskmgmt/harddisk/DESC \
menu/diskmgmt/harddisk/display menu/diskmgmt/harddisk/makehdfsys \
menu/diskmgmt/harddisk/partitioning menu/diskmgmt/harddisk/rmdisk \
menu/filemgmt/bupsched/DESC menu/filemgmt/bupsched/schedcheck \
menu/filemgmt/bupsched/schedmsg 

ALL: \
		$(PARTS)

install: \
		$(PARTS) remove
	: 'Must be run by root or the owner of /usr (sys)'
	-umask 022;  if test ! -d $(ADMINHOME); then mkdir $(ADMINHOME); fi;  cd $(ADMINHOME)
	for dir in $(DIRECTORIES); do if test ! -d $$dir; then mkdir $$dir; fi; done
	umask 022;  ls $(PARTS)  |  cpio -pd $(ADMINHOME)
	chmod +x $(ADMINHOME)/bupsched
	chmod +x $(ADMINHOME)/menu/diskmgmt/harddisk/makehdfsys

remove:
	if test -d $(ADMINHOME); then cd $(ADMINHOME);  rm -f $(PARTS); \
	A=`echo $(DIRECTORIES)  |  tr ' ' '\012'  |  sort -r`; \
	for dir in $$A; do if test -d $$dir; then if test `ls $$dir | wc -l` -eq 0; then rmdir $$dir; fi; fi; done; \
	cd /; if test `ls $(ADMINHOME) | wc -l` -eq 0; then rmdir $(ADMINHOME); fi; fi

clean clobber:

partslist:
	@echo Makefile $(PARTS)  |  tr ' ' '\012'  |  sort

product:
	@echo $(PARTS)  |  tr ' ' '\012'  | \
		sed 's;^;$(ADMINHOME)/;'

productdir:
	@echo $(ADMINHOME)
	@echo $(DIRECTORIES)  |  tr ' ' '\012'  | \
		sed 's;^;$(ADMINHOME)/;'

srcaudit:
	@(	find * -print;  \
		ls -d $(DIRECTORIES) $(PARTS) Makefile  \
	)  |  sort  |  uniq -u  |  sed 's/$$/	unexpected/'
