/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/common/EventTable.h	1.2"
#ifndef EventTable_h
#define EventTable_h

#include	"Breaklist.h"
#include	"Watchlist.h"
#include	"Siglist.h"
#include	"TSClist.h"
#include	"oslevel.h"

struct Object;
class Process;

struct EventTable {
	Breaklist	breaklist;
	Watchlist	watchlist;
	Siglist		siglist;
	TSClist		tsclist;
	Object *	object;
	Process *	process;
			EventTable()	{ object = 0 ; process = 0; }
};

EventTable *		find_et( int );
EventTable *		dispose_et( EventTable * );

#endif

// end of EventTable.h

