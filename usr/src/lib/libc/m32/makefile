#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-m32:makefile	1.46.1.31"
#
#  makefile for libc/m32
#
#
# The variable PROF is null by default, causing profiled object to be
# maintained.  If profiled object is not desired, the reassignment
# PROF=@# should appear in the make command line.
#

.SUFFIXES: .p .P .A

MAC=
M4=m4
CC=cc
LD=ld
MAKE=make
CFLAGS=-O
PROF=
NONPROF=
PIC=
ABI=
INCLIBC=../inc
DEFLIST=
INCLIST=-I$(INCLIBC)
VERSDEF=ansi.def
M4DEFS=m4.def $(VERSDEF)

OBJECTS=\
crt/cerror.o      crt/mcount.o      \
csu/crt0.o        csu/mcrt0.o	\
csu/fcrt0.o	  csu/fmcrt0.o	    csu/crti.o \
csu/crt1.o	  csu/fcrt1.o       csu/fmcrt1.o      csu/mcrt1.o	\
csu/crtn.o	  csu/pcrt1.o	    csu/pcrti.o       csu/pcrtn.o       \
fp/decconv.o      fp/scalb.o        \
gen/abs.o	  gen/atof.o        \
gen/biglitpow.o	  gen/dtop.o	    gen/ecvt.o        \
gen/fptrap.o      \
gen/frexp.o       \
gen/getctxt.o	  gen/ladd.o	    gen/lconstants.o  gen/ldexp.o	\
gen/ldivide.o     gen/lexp10.o      gen/llog10.o      gen/lmul.o	\
gen/lshiftl.o     gen/lsign.o       gen/lsub.o        gen/ltostr.o      \
gen/makectxt.o	  gen/memcpy.o	    gen/ptod.o 	      gen/setjmp.o	\
gen/sigsetjmp.o   gen/strcpy.o      gen/strcmp.o      gen/strtod.o	\
gen/strlen.o	  gen/swapctxt.o    print/doprnt.o    print/fprintf.o   \
gen/cuexit.o      gen/m32_data.o    gen/sh_data.o     \
print/printf.o    print/sprintf.o   \
print/vfprintf.o  print/vprintf.o   print/vsprintf.o  print/setpospar.o \
sys/_rename.o     sys/_sigaction.o  sys/_signal.o     \
sys/acancel.o	  sys/access.o      sys/acct.o        sys/adjtime.o	\
sys/alarm.o	  sys/brkbase.o     \
sys/chdir.o       sys/chmod.o       sys/chown.o	      \
sys/chroot.o	  sys/close.o	    sys/creat.o	      sys/dup.o		\
sys/evsys.o	  sys/execl.o	    sys/execle.o      sys/execv.o       \
sys/execve.o	  sys/exit.o	    sys/fchdir.o      sys/fchmod.o	\
sys/fchown.o	  sys/fcntl.o	    sys/fmount.o      sys/fork.o	\
sys/fstat.o	  sys/fstatvfs.o    sys/fsync.o	      sys/funmount.o	\
sys/fxstat.o	  sys/getegid.o	    sys/geteuid.o     sys/getgid.o	\
sys/getgroups.o	  sys/getmsg.o	    sys/getpagesize.o sys/getpid.o	\
sys/getpmsg.o	  sys/getppid.o	    sys/getrlimit.o   sys/getuid.o      \
sys/gtty.o	  sys/ioctl.o	    sys/kill.o	      sys/lchown.o	\
sys/link.o	  sys/lseek.o	\
sys/lstat.o       sys/lxstat.o	    sys/mctl.o	      sys/memcntl.o	\
sys/mincore.o	  sys/mknod.o	\
sys/mmap.o        sys/mount.o       sys/mprotect.o    sys/msgsys.o	\
sys/munmap.o      sys/_nfssys.o	    sys/nice.o	      sys/nuname.o	\
sys/open.o	  sys/pause.o	    sys/pipe.o	      sys/poll.o	\
sys/priocntlset.o sys/profil.o	    sys/plock.o	      sys/ptrace.o	\
sys/putmsg.o	  sys/putpmsg.o	    sys/read.o	      sys/readlink.o    \
sys/readv.o	  sys/rfsys.o	    \
sys/sbrk.o        sys/semsys.o	    sys/seteguid.o    \
sys/setgid.o	  sys/setgroups.o   sys/setpgrp.o     sys/setrlimit.o	\
sys/setsid.o	  sys/setuid.o	    sys/shmsys.o      \
sys/sigaction.o   sys/sigaltstk.o   sys/signal.o      \
sys/sigpending.o  sys/sigprocmsk.o  sys/sigsendset.o  sys/sigsuspend.o  \
sys/stat.o        sys/statvfs.o	    sys/stime.o	      sys/stty.o	\
sys/symlink.o	  sys/sync.o	    sys/sys3b.o	      sys/syscall.o	\
sys/sysconfig.o	  sys/sysfs.o	    sys/sysinfo.o     \
sys/time.o        sys/times.o       sys/uadmin.o      sys/ucontext.o    \
sys/ulimit.o      sys/umask.o       sys/umount.o      \
sys/uname.o	  sys/unlink.o	    sys/ustat.o	      sys/utime.o	\
sys/utssys.o	  \
sys/vfork.o	  sys/wait.o	    sys/waitid.o      sys/write.o	\
sys/writev.o	  sys/xmknod.o	    sys/xstat.o	      \
sys/fstatfs.o	  sys/getdents.o    sys/mkdir.o	      \
sys/rmdir.o	  sys/statfs.o      \
fp/faddd.o	  fp/fmuld.o	    fp/fdivd.o	      fp/fcmpd.o 	\
fp/fnegd.o	  fp/fltod.o	    fp/fltos.o	  	      		\
fp/fcmps.o	  fp/fdtos.o 	    \
fp/fstod.o	  fp/ftdtol.o	    fp/ftdtou.o	      \
fp/ftstol.o	  fp/ftstou.o	    \
fp/dround.o	  fp/dspecial.o     \
fp/fadds.o	  fp/fmuls.o	    fp/fdivs.o	      \
fp/fnegs.o	  fp/sround.o	    fp/sspecial.o     \
fp/fp_data.o      fp/flth1_data.o   \
fp/fpstart.o	  fp/fpstart0.o	    fp/fpstart1.o     \
fp/getflts0.o	  fp/getflts1.o	    fp/getflth0.o     fp/getflth1.o	\
fp/fpgetmask.o	  fp/fpsetmask.o    fp/fpgetrnd.o     fp/fpsetrnd.o	\
fp/fpgetsticky.o  fp/fpsetsticky.o  fp/isnand.o	      fp/isnanf.o	\
fp/finite.o	  fp/logb.o

POBJECTS=$(OBJECTS:.o=.p)

PIC_OBJECTS=\
crt/cerror.P      \
fp/dround.P       fp/dspecial.P     fp/faddd.P        fp/fadds.P        \
fp/fcmpd.P        fp/fcmps.P        fp/fdivd.P        fp/fdivs.P        \
fp/fdtos.P        fp/fltod.P        fp/fltos.P        fp/fmuld.P        \
fp/fmuls.P        fp/fnegd.P        fp/fnegs.P        fp/fpstart.P      \
fp/fpstart1.P     fp/fstod.P        fp/ftdtol.P       fp/ftdtou.P       \
fp/ftstol.P       fp/ftstou.P       fp/getflth1.P     fp/getflts1.P     \
fp/isnand.P       fp/sround.P       fp/sspecial.P     gen/atof.P	\
gen/biglitpow.P   gen/cuexit.P      gen/dtop.P        \
gen/frexp.P       gen/getctxt.P     gen/ldexp.P       \
gen/ltostr.P      gen/m32_data.P    gen/memcpy.P      gen/ptod.P        \
gen/setjmp.P      gen/sh_data.P     gen/sigsetjmp.P   gen/strcmp.P      \
gen/strcpy.P      gen/strlen.P      gen/strtod.P      \
print/doprnt.P    print/fprintf.P   print/printf.P    print/setpospar.P \
print/sprintf.P   print/vfprintf.P  print/vprintf.P   print/vsprintf.P  \
sys/_rename.P     sys/_sigaction.P  sys/_signal.P     sys/access.P      \
sys/acct.P        sys/alarm.P       sys/chdir.P       sys/chmod.P       \
sys/chown.P       sys/chroot.P      sys/close.P       sys/creat.P       \
sys/dup.P         sys/execl.P       sys/execle.P      sys/execv.P       \
sys/execve.P      sys/exit.P        sys/fchdir.P      sys/fchmod.P      \
sys/fchown.P      sys/fcntl.P       sys/fork.P        sys/fstat.P       \
sys/fstatvfs.P    sys/fsync.P       sys/fxstat.P      sys/getdents.P    \
sys/getegid.P     sys/geteuid.P     sys/getgid.P      sys/getgroups.P   \
sys/getmsg.P      sys/getpagesize.P sys/getpid.P      sys/getpmsg.P     \
sys/getppid.P     \
sys/getrlimit.P   sys/getuid.P      sys/ioctl.P       sys/kill.P        \
sys/lchown.P      sys/link.P        sys/lseek.P       sys/lstat.P       \
sys/lxstat.P      sys/mctl.P        sys/memcntl.P     sys/mincore.P     \
sys/mkdir.P       sys/mknod.P       sys/mmap.P        sys/mount.P       \
sys/mprotect.P    sys/msgsys.P      sys/munmap.P      sys/nice.P        \
sys/nuname.P      sys/open.P        sys/pause.P       sys/pipe.P        \
sys/plock.P       sys/poll.P        sys/priocntlset.P sys/profil.P      \
sys/ptrace.P      sys/putmsg.P      sys/putpmsg.P     \
sys/read.P        sys/readlink.P    sys/readv.P       \
sys/rmdir.P       sys/sbrk.P        sys/semsys.P      sys/setgid.P      \
sys/setgroups.P   sys/setpgrp.P     sys/setrlimit.P   sys/setsid.P      \
sys/setuid.P      sys/shmsys.P      sys/sigaction.P   sys/sigaltstk.P   \
sys/signal.P      sys/sigpending.P  sys/sigprocmsk.P  sys/sigsendset.P  \
sys/sigsuspend.P  sys/stat.P        sys/statvfs.P     sys/stime.P       \
sys/symlink.P     sys/sync.P        sys/sys3b.P       sys/syscall.P     \
sys/sysconfig.P   sys/sysinfo.P     \
sys/time.P        sys/times.P       sys/uadmin.P      \
sys/ucontext.P    sys/ulimit.P      sys/umask.P       sys/umount.P      \
sys/uname.P       sys/unlink.P      sys/ustat.P       sys/utime.P       \
sys/wait.P        sys/waitid.P      sys/write.P       sys/writev.P      \
sys/xmknod.P      sys/xstat.P

ABI_OBJECTS=$(PIC_OBJECTS:.P=.A)

all:
	if [ "$(ISANSI)" = "TRUE" ] ; \
	then VERSDEF="ansi.def"; \
	else VERSDEF="nonansi.def"; \
	fi ; \
	$(MAKE) -e all_vers VERSDEF=$$VERSDEF

all_vers:
	$(PROF)$(MAKE) -e $(IGN) prof NONPROF=@# PIC=@# ABI=@#
	$(PIC)$(MAKE) -e $(IGN) pic NONPROF=@# PROF=@# ABI=@#
	$(ABI)$(MAKE) -e $(IGN) abi NONPROF=@# PROF=@# PIC=@#
	$(NONPROF)$(MAKE) -e $(IGN) nonprof PROF=@# PIC=@# ABI=@#

nonprof: $(OBJECTS)

prof: $(POBJECTS)

pic: $(PIC_OBJECTS)

abi: $(ABI_OBJECTS)

.c.o .c.p .c.P .c.A:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS)  -c $*.c
	$(NONPROF)mv $(*F).o $*.o
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -p $*.c  && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -DDSHLIB -Kpic $*.c  && mv $(*F).o $*.P
	$(ABI)@echo $*.c:
	$(ABI)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -DABI -DDSHLIB -Kpic $*.c  && mv $(*F).o $*.A

.s.o .s.p .s.P .s.A:
	$(NONPROF)@echo $*.s:
	$(NONPROF)$(M4) $(M4DEFS) -DMCOUNT=# nonpic.def $*.s   > $(*F).s   
	$(NONPROF)$(CC) $(DEFLIST) $(CFLAGS) -c $(*F).s && mv $(*F).o $*.o
	$(PROF)@echo $*.s:
	$(PROF)$(M4) $(M4DEFS) mcount.def nonpic.def $*.s   > $(*F).s 
	$(PROF)$(CC) $(DEFLIST) $(CFLAGS) -c -p $(*F).s  && mv $(*F).o $*.p
	$(PIC)@echo $*.s:
	$(PIC)$(M4) -DDSHLIB $(M4DEFS) -DMCOUNT=# pic.def  $*.s   > $(*F).s 
	$(PIC)$(CC) $(DEFLIST) $(CFLAGS) -c -Kpic $(*F).s  && mv $(*F).o $*.P
	$(ABI)@echo $*.s:
	$(ABI)$(M4) -DABI -DDSHLIB $(M4DEFS) -DMCOUNT=# pic.def  $*.s   > $(*F).s 
	$(ABI)$(CC) $(DEFLIST) $(CFLAGS) -c -Kpic $(*F).s  && mv $(*F).o $*.A
	-rm $(*F).s

archive:
	#
	# Note that "archive" is invoked with libsys/object as current directory.
	#
	# figure out the correct ordering for all the archive modules
	mv cuexit.o cuexit.x
	mv fpstart.o fpstart.x
	mv fpstart0.o fpstart0.x
	mv signal.o signal.x
	mv getflth0.o getflth0.x
	mv getflth1.o getflth1.x
	mv getflts0.o getflts0.x
	mv getflts1.o getflts1.x
	mv kill.o kill.x
	mv getpid.o getpid.x
	mv cerror.o cerror.x
	mv fpsetmask.o fpsetmask.x
	mv atexit.o atexit.x
	-rm -f old_toupper.o old_tolower.o
	ls | grep '\.o$$' | xargs $(LORDER) | tsort >objlist
	mv cuexit.x cuexit.o
	mv fpstart.x fpstart.o
	mv fpstart0.x fpstart0.o
	mv signal.x signal.o
	mv getflth0.x getflth0.o
	mv getflth1.x getflth1.o
	mv getflts0.x getflts0.o
	mv getflts1.x getflts1.o
	mv kill.x kill.o
	mv getpid.x getpid.o
	mv cerror.x cerror.o
	mv fpsetmask.x fpsetmask.o
	mv atexit.x atexit.o
	#
	# build the archive with the modules in correct order.
	$(AR) q ../lib.libc getflts1.o getflth1.o atexit.o
	xargs $(AR) q ../lib.libc <objlist
	$(AR) q ../lib.libc signal.o fpstart.o fpstart0.o \
	getflth0.o getflts0.o fpsetmask.o kill.o getpid.o \
	cerror.o cuexit.o
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../lib.libc fpstart0.o ; \
	fi
	$(PROF)#
	$(PROF)# build the profiled library archive, first renaming the
	$(PROF)#	.p (profiled object) modules to .o
	$(PROF)find . -name '*.p' -print | \
	while read i; do mv $$i `basename $$i .p`.o ; done
	if [ "$(PROF)" != "@#" ]; \
	then \
	$(PROF)$(AR) q ../libp.libc getflts1.o getflth1.o atexit.o; \
	$(PROF)xargs $(AR) q ../libp.libc <objlist; \
	$(PROF)$(AR) q ../libp.libc signal.o fpstart.o fpstart0.o \
	getflth0.o getflts0.o \
	fpsetmask.o kill.o getpid.o cerror.o cuexit.o ; \
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../libp.libc fpstart0.o ; \
	fi \
	fi

shared:
	#
	# Note that "shared" is invoked with libc/object as current directory
	#
	-rm -rf shared
	mkdir shared
	cp `cat ../$(MACHINE)/shared_objects | grep -v "\#"` shared
	rm -f `cat ../$(MACHINE)/shared_objects | grep "^[^\.\#]" | sed 's/\.P/.o/'`
	#
	# create the shared portion of the library
	cd shared; \
		$(LD) -o ../../libc.so.1 -dy -G -e _rt_boot -h /usr/lib/libc.so.1 -ztext -zdefs -Bsymbolic `cat ../../$(MACHINE)/shared_objects | grep -v "\#"`
	#
	# figure out the correct ordering for all the archive modules
	mv fpstart0.o fpstart0.x
	mv getflth0.o getflth0.x
	mv getflts0.o getflts0.x
	mv fpsetmask.o fpsetmask.x
	-rm -f old_toupper.o old_tolower.o
	ls | grep '\.o$$' | xargs $(LORDER) | tsort >objlist
	mv fpstart0.x fpstart0.o
	mv getflth0.x getflth0.o
	mv getflts0.x getflts0.o
	mv fpsetmask.x fpsetmask.o
	#
	# build the archive with the modules in correct order.
	xargs $(AR) q ../libc.so <objlist
	$(AR) q ../libc.so ../libc.so.1 \
		fpstart0.o getflth0.o getflts0.o fpsetmask.o
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../libc.so fpstart0.o ; \
	fi

abi_lib:
	#
	# Note that "abi" is invoked with libc/object as current directory
	#
	-rm -rf shared
	mkdir shared
	cp `cat ../$(MACHINE)/abi_objects | grep -v "\#"` shared
	rm -f `cat ../$(MACHINE)/abi_objects | grep "^[^\.\#]" | sed 's/\.A/.o/'`
	#
	# create the shared portion of the library
	cd shared; \
		$(LD) -o ../../ld.so.1 -dy -G -e _rt_boot -h /usr/lib/ld.so.1 -ztext -zdefs -Bsymbolic `cat ../../$(MACHINE)/abi_objects | grep -v "\#"`
	#
	# figure out the correct ordering for all the archive modules
	mv fpstart0.o fpstart0.x
	mv getflth0.o getflth0.x
	mv getflts0.o getflts0.x
	mv fpsetmask.o fpsetmask.x
	-rm -f old_toupper.o old_tolower.o
	ls | grep '\.o$$' | xargs $(LORDER) | tsort >objlist
	mv fpstart0.x fpstart0.o
	mv getflth0.x getflth0.o
	mv getflts0.x getflts0.o
	mv fpsetmask.x fpsetmask.o
	#
	# build the archive with the modules in correct order.
	xargs $(AR) q ../libabi.so <objlist
	$(AR) q ../libabi.so ../ld.so.1 \
		fpstart0.o getflth0.o getflts0.o fpsetmask.o
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../libabi.so fpstart0.o ; \
	fi

clean:
	-rm -f *.o
	-rm -f *.p
	-rm -f *.P
	-rm -f *.s

clobber: clean
	-rm -f */*.o
	-rm -f */*.p
	-rm -f */*.P
