#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:Makefile	1.16"
OAMBASE=/usr/sadm/sysadm
DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
FILEDIR = $(ROOT)$(OAMBASE)/menu/files
BINDIR = $(ROOT)$(OAMBASE)/bin
INS = install
STRIP = strip
SIZE = size

TASKS = check defaults diskuse display fileage filesize identify \
	list make mount unmount

O_CFILES = sysfs

O_SHFILES = dirchk filesearch getdiskuse getfilesize getfsname rmfile \
	    unmountfs ckerrs checkfs makefs makesys mountfs chkrun \
	    adddef checkfilsys defdev deffs devices devlst dispdef\
	    moddef filsys findtype fsnames getdefdev rmdisk \
	    getdeffs Identify indeftab invfstab rmdef valdefdev \
	    valdeffs validdev validfs fstyp_special fstype \
	    mkfs_s5 mkfs_ufs numblocks numfiles ufs_fragsiz  \
	    existsfs ifdiskette call_fsck

check = Form.check Form.checkfs Text.ckerrs Text.ckok Text.cknok \
	 Text.findtype Help

defaults = Form.add Form.add2 Form.display Form.modify \
	Form.modify2 Form.remove Text.display Text.adddef \
	defaults.menu  Help

diskuse = Form.diskuse Text.diskuse Text.duse2 Help

display = Text.dispfstyp Help

fileage = Form.fileage Text.searchnok Text.searchok  Help

filesize = Form.filesize Text.sizenok Text.sizeok  Help

identify = Form.identify Text.identify  Help

list = Text.listfiles  Help

make = Form.make Form.make2 Form.mntpt Form.s5 Form.s52 Form.ufs Form.ufs2 \
       Form.existsfs Text.make Help

mount = Form.mount Form.mount2 Form.mount3 \
	Text.findtype Text.mountok  Help

unmount = Form.unmount Form.umountfs \
          Text.unmount  Help

O_DFILES = file.menu  Help

all: $(O_CFILES) $(O_SHFILES) 


$(O_CFILES):
	$(CC) -O bin/$(@).c  -o bin/$(@)

$(O_SHFILES):
	cp bin/$(@).sh  bin/$(@)

clean:

clobber:
	rm -f $(O_SHFILES)

install: all $(FILEDIR) 
# check
	if [ ! -d $(FILEDIR)/check ] ;\
	then \
		mkdir $(FILEDIR)/check ;\
	fi
	for i in $(check) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/check check/$$i ;\
	done


# defaults  
	if [ ! -d $(FILEDIR)/defaults ] ;\
	then \
		mkdir $(FILEDIR)/defaults ;\
	fi
	for i in $(defaults) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/defaults defaults/$$i ;\
	done


#diskuse
	if [ ! -d $(FILEDIR)/diskuse ] ;\
	then \
		mkdir $(FILEDIR)/diskuse ;\
	fi
	for i in $(diskuse) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/diskuse diskuse/$$i ;\
	done


#display
	if [ ! -d $(FILEDIR)/display ] ;\
	then \
		mkdir $(FILEDIR)/display ;\
	fi
	for i in $(display) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/display display/$$i ;\
	done


#fileage 
	if [ ! -d $(FILEDIR)/fileage ] ;\
	then \
		mkdir $(FILEDIR)/fileage ;\
	fi
	for i in $(fileage) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/fileage fileage/$$i ;\
	done


#filesize 
	if [ ! -d $(FILEDIR)/filesize ] ;\
	then \
		mkdir $(FILEDIR)/filesize ;\
	fi
	for i in $(filesize) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/filesize filesize/$$i ;\
	done


#identify
	if [ ! -d $(FILEDIR)/identify ] ;\
	then \
		mkdir $(FILEDIR)/identify ;\
	fi
	for i in $(identify) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/identify identify/$$i ;\
	done

#list 
	if [ ! -d $(FILEDIR)/list ] ;\
	then \
		mkdir $(FILEDIR)/list ;\
	fi
	for i in $(list) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/list list/$$i ;\
	done

#make 
	if [ ! -d $(FILEDIR)/make ] ;\
	then \
		mkdir $(FILEDIR)/make ;\
	fi
	for i in $(make) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/make make/$$i ;\
	done

#mount 
	if [ ! -d $(FILEDIR)/mount ] ;\
	then \
		mkdir $(FILEDIR)/mount ;\
	fi
	for i in $(mount) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/mount mount/$$i ;\
	done

#unmount
	if [ ! -d $(FILEDIR)/unmount ] ;\
	then \
		mkdir $(FILEDIR)/unmount ;\
	fi
	for i in $(unmount) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR)/unmount unmount/$$i ;\
	done


#main
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR) $$i ;\
	done

	for i in $(O_SHFILES) ;\
	do \
		$(INS) -m 755 -g bin -u bin -f $(BINDIR) bin/$$i ;\
	done

	for i in $(O_CFILES) ;\
	do \
		$(INS) -m 755 -g bin -u bin -f $(BINDIR) bin/$$i ;\
	done

size: all
	$(SIZE)

strip: all
	$(STRIP)


$(FILEDIR):
	if [ ! -d `dirname $(FILEDIR)` ] ;\
	then \
		mkdir `dirname $(FILEDIR)` ;\
	fi
	if [ ! -d $(FILEDIR) ] ;\
	then \
		mkdir $(FILEDIR) ;\
	fi

$(TASKS):
	if [ ! -d $(FILEDIR)/$(@) ] ;\
	then \
		mkdir $(FILEDIR)/$(@) ;\
	fi
