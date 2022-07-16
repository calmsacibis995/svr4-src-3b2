#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)osadmin:admin/Makefile	1.2"
#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Makefile for admin
#    @(#)SKELMakefile  2.3  /sccs/src/cmd/sadmin/admin/s.SKELMakefile

ADMINHOME = $(ROOT)/usr/admin
BIN = $(ROOT)/usr/bin

CMDS = sysadm r3_sysadm

C_CMD = r3_sysadm

DIRECTORIES = menu menu/diagnostics menu/diskmgmt menu/filemgmt \
menu/machinemgmt menu/packagemgmt menu/softwaremgmt menu/syssetup \
menu/ttymgmt menu/usermgmt menu/usermgmt/modgroup menu/usermgmt/moduser 

PARTS = $(CMDS) menu/DESC menu/diagnostics/DESC menu/diskmgmt/DESC \
menu/diskmgmt/checkfsys menu/diskmgmt/cpdisk menu/diskmgmt/erase \
menu/diskmgmt/makefsys menu/diskmgmt/mountfsys menu/diskmgmt/umountfsys \
menu/filemgmt/DESC menu/filemgmt/backup menu/filemgmt/diskuse \
menu/filemgmt/fileage menu/filemgmt/filesize menu/filemgmt/restore \
menu/filemgmt/store menu/machinemgmt/DESC menu/packagemgmt/DESC \
menu/softwaremgmt/DESC menu/softwaremgmt/install \
menu/softwaremgmt/installpkg menu/softwaremgmt/listpkg \
menu/softwaremgmt/removepkg menu/softwaremgmt/runpkg \
menu/softwaremgmt/uninstall menu/syssetup/DESC menu/ttymgmt/DESC \
menu/usermgmt/DESC menu/usermgmt/modgroup/DESC menu/usermgmt/moduser/DESC 

ALL: \
		$(PARTS) $(C_CMD)

install: \
		$(PARTS) $(BIN) $(C_CMD) remove
	: 'Must be run by root or the owner of /usr and $(BIN) (sys)'
	-umask 022;  if test ! -d $(ADMINHOME); then mkdir $(ADMINHOME); fi;  cd $(ADMINHOME)
	for dir in $(DIRECTORIES); do if test ! -d $$dir; then mkdir $$dir; fi; done
	umask 022;  ls $(PARTS)  |  cpio -pd $(ADMINHOME)
	cd $(ADMINHOME);  chmod u+xw,go-xw $(CMDS)

$(C_CMD): shade.c
	$(CC) -s -o r3_sysadm shade.c

remove:
	if test -d $(ADMINHOME); then cd $(ADMINHOME);  rm -f $(PARTS); \
	A=`echo $(DIRECTORIES)  |  tr ' ' '\012'  |  sort -r`; \
	for dir in $$A; do if test -d $$dir; then if test `ls $$dir | wc -l` -eq 0; then rmdir $$dir; fi; fi; done;  \
	cd $(BIN); \
	if test `ls $(ADMINHOME) | wc -l` -eq 0; then rmdir $(ADMINHOME); fi; \
	fi

$(BIN):
	mkdir $(BIN)
	chmod 755 $(BIN)

clean clobber:
	rm -f r3_sysadm

partslist:
	@echo Makefile $(PARTS)  |  tr ' ' '\012'  |  sort

product:
	@echo $(PARTS)  |  tr ' ' '\012'  | \
		sed 's;^;$(ADMINHOME)/;'

productdir:
	@echo $(BIN)
	@echo $(ADMINHOME)
	@echo $(DIRECTORIES)  |  tr ' ' '\012'  | \
		sed 's;^;$(ADMINHOME)/;'

srcaudit:
	@(	find * -print;  \
		ls -d $(DIRECTORIES) $(PARTS) Makefile  \
	)  |  sort  |  uniq -u  |  sed 's/$$/	unexpected/'

