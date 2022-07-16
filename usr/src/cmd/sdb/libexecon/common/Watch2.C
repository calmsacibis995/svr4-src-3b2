//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libexecon/common/Watch2.C	1.1"
#include	"Flags.h"
#include	"Interface.h"
#include	"Locdesc.h"
#include	"Watchlist.h"
#include	<memory.h>
#include	<string.h>

int
Watchpoint::operator>( Avlnode & node )
{
	Watchpoint &	w = *((Watchpoint*)&node);

	if ( symname > w.symname )
		return 1;
	else if ( symname < w.symname )
		return 0;
	else
		return lvalue > w.lvalue;
}

int
Watchpoint::operator<( Avlnode & node )
{
	Watchpoint &	w = *((Watchpoint*)&node);

	if ( symname < w.symname )
		return 1;
	else if ( symname > w.symname )
		return 0;
	else
		return lvalue < w.lvalue;
}

Watchpoint &
Watchpoint::operator=( Watchpoint & w )
{
	expr = w.expr;
	lvalue = w.lvalue;
	flags = w.flags;
	rvalue = w.rvalue;
	frameid = w.frameid;
	lopc = w.lopc;
	hipc = w.hipc;
	return *this;
}

void
Watchpoint::value_swap( Avlnode * node )
{
	Watchpoint	temp( expr );
	Watchpoint *	w;

	w = (Watchpoint*)&node;
	temp =  *w;
	*w = *this;
	*this = temp;
}

Avlnode *
Watchpoint::makenode()
{
	Watchpoint *	w;

	w = new Watchpoint( expr );
	*w = *this;
	return w;
}
