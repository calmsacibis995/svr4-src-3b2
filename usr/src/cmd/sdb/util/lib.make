#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#ident	"@(#)sdb:util/lib.make	1.3"

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	$(AR) -qc $(TARGET) $(OBJECTS)
	@if [ $(MACH) = sparc ] ; then echo "\tranlib $(TARGET)" ;\
		ranlib $(TARGET) ; fi
	chmod 664 $(TARGET)

all:	$(TARGET)

install:	all
