/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:libexecon/m32/Segment.h	1.2"
#ifndef Segment_h
#define Segment_h

#include	"Itype.h"
#include	"Link.h"
#include	"Symtab.h"
#include	"oslevel.h"

class Segment: public Link {
	Iaddr		loaddr,hiaddr;
	Key		access;
	long		base;
	char *		pathname;
	Symtab		sym;
	Segment *	next()	{	return (Segment*)Link::next();	}
	friend class	Seglist;
public:
			Segment( Key, char *, Iaddr, long, long, long );
			~Segment()	{	unlink();	}

	int		read( Iaddr, void *, int );
	int		write( Iaddr, void *, int );
	int		read( Iaddr, Stype, Itype & );
	int		write( Iaddr, Stype, const Itype & );

	int		get_symtable( Key );
};

int	stype_size( Stype );

#endif

// end of Segment.h

