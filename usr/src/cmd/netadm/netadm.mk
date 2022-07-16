#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netadm:netadm.mk	1.6"

DIR = $(ROOT)/etc $(ROOT)/etc/saf
SCRIPTS = _sactab.sh _sysconf.sh iu.ap.sh ttydefs.sh
FILES = _sactab _sysconfig iu.ap ttydefs

all:	$(FILES)

_sactab:	_sactab.sh
	sh _sactab.sh

_sysconfig:	_sysconf.sh
	sh _sysconf.sh

iu.ap:	iu.ap.sh
	sh iu.ap.sh

ttydefs:	ttydefs.sh
	sh ttydefs.sh

install:	all $(DIR)
	install -f $(ROOT)/etc/saf -m 644 -u root -g sys _sactab
	install -f $(ROOT)/etc/saf -m 644 -u root -g sys _sysconfig
	install -f $(ROOT)/etc -m 644 -u root -g sys iu.ap
	install -f $(ROOT)/etc -m 644 -u root -g sys ttydefs

clobber:	clean
	rm $(FILES)

clean:

$(DIR):
	mkdir ${@}
