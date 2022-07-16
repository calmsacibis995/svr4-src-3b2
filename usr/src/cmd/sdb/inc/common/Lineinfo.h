/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/common/Lineinfo.h	1.1"
#ifndef Lineinfo_h
#define Lineinfo_h

#include "Itype.h"

struct LineEntry {
	Iaddr		addr;
	long		linenum;
};

struct Lineinfo {
	int		entrycount;
	LineEntry *	addrpart;
	LineEntry *	linepart;
};

#define BIG_LINE	1000000000

#endif

// end of Lineinfo.h

