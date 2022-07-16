//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libsymbol/common/AddrList.C	1.4"
#include	"Attribute.h"
#include	"builder.h"
#include	"AddrList.h"
#include	<string.h>
#include	<malloc.h>

AddrEntry::AddrEntry()
{
	loaddr = 0;
	hiaddr = 0;
	form = af_none;
	value.word = 0;
}

AddrEntry::AddrEntry( AddrEntry & addrentry )
{
	loaddr = addrentry.loaddr;
	hiaddr = addrentry.hiaddr;
	form = addrentry.form;
	value = addrentry.value;
}

// Make a AddrEntry instance
Avlnode *
AddrEntry::makenode()
{
	char *	s;

	s = ::malloc(sizeof(AddrEntry));
	::memcpy(s,(char*)this,sizeof(AddrEntry));
	return (Avlnode*)s;
}

int
AddrEntry::operator>( Avlnode & t )
{
	AddrEntry &	addrentry = *(AddrEntry*)(&t);

	if ( (loaddr <= addrentry.loaddr) && (addrentry.hiaddr < hiaddr) )
		return 0;
	else
		return loaddr > addrentry.loaddr;
}

int
AddrEntry::operator<( Avlnode & t )
{
	AddrEntry &	addrentry = *(AddrEntry*)(&t);

	if ( (loaddr <= addrentry.loaddr) && (addrentry.hiaddr < hiaddr) )
		return 0;
	else
		return loaddr < addrentry.loaddr;
}

AddrEntry &
AddrEntry::operator=( AddrEntry & addrentry )
{
	loaddr = addrentry.loaddr;
	hiaddr = addrentry.hiaddr;
	form = addrentry.form;
	value = addrentry.value;
	return *this;
}

AddrList::AddrList()
{
}

AddrEntry *
AddrList::add( Iaddr lo, Iaddr hi, long offset, Attr_form form )
{
	AddrEntry	node;
	Avlnode *	t;

	node.loaddr = lo;
	node.hiaddr = hi;		// not always true ?
	node.form = form;
	node.value.word = offset;
	t = tinsert(node);
	return (AddrEntry *)t;
}

void
AddrList::complete()
{
	AddrEntry	* last;
	AddrEntry	* x;
	AddrEntry *	addrentry;
	Iaddr		lastlo;

	last = (AddrEntry*)tlast();
	if ( last != 0 )
	{
		addrentry = last;
		addrentry->hiaddr = ~0;
		lastlo = addrentry->loaddr;
		x = (AddrEntry*)(addrentry->last());
		while ( (x != 0) && (x != last) )
		{
			addrentry = x;
			if (addrentry->hiaddr == addrentry->loaddr)
			{
				addrentry->hiaddr = lastlo - 1;
			}
			lastlo = addrentry->loaddr;
			x = (AddrEntry*)(addrentry->last());
		}
	}
}
