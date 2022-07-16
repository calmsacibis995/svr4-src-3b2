#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nsadmin:Makefile	1.6"

DIR = $(ROOT)/etc

SYMLINK = :
INS = install

CMDS = savecpio 

DIRS = save.d

PARTS = $(CMDS) TIMEZONE disketteparm profile rc.d/setup \
save.d/except stdprofile 

all:		$(PARTS)

install:	all $(DIR)
	-rm -f $(DIR)/savecpio
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u root -g sys savecpio
	$(SYMLINK) /usr/sbin/savecpio $(DIR)/savecpio
	$(INS) -f $(DIR) -m 0444 -u root -g sys TIMEZONE
	$(INS) -f $(DIR) -m 0444 -u root -g sys disketteparm
	$(INS) -f $(DIR) -m 0644 -u root -g sys profile
	$(INS) -f $(DIR)/rc.d -m 0444 -u root -g sys rc.d/setup
	$(INS) -f $(DIR)/save.d -m 0444 -u root -g sys save.d/except
	$(INS) -f $(DIR) -m 0444 -u root -g sys stdprofile

$(DIR):
	mkdir $(DIR);  chmod 755 $(DIR)

remove:
	cd $(DIR);  rm -f $(PARTS)
	-cd $(DIR);  if test -d $(DIRS); then if test `ls $(DIRS) | wc -l` -eq 0; then rmdir $(DIRS); fi; fi

partslist:
	@echo Makefile SKELMakefile $(PARTS)  |  tr ' ' '\012'  |  sort
	
product:
	@echo $(PARTS)  |  tr ' ' '\012'  |  sed "s;^;$(DIR)/;"

productdir:
	@echo $(DIR)
	@echo $(DIRS)  |  tr ' ' '\012'  |  sed "s;^;$(DIR)/;"

clean:

clobber:

srcaudit:
	@(	find * -print;  \
		ls -d $(DIRS) $(PARTS) SKELMakefile Makefile  \
	)  |  sort  |  uniq -u  |  sed 's/$$/	unexpected/'

newmakefile:	../tools/genmakefile SKELMakefile
	cp Makefile OMakefile
	../tools/genmakefile SKELMakefile *Makefile >Makefile
	rm -f OMakefile
