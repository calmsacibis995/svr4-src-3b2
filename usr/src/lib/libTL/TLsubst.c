/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libTL:TLsubst.c	1.2"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];

int
TLsubst( tid, entry, fieldname, pattern, replace )
int tid;
entry_t *entry;
unsigned char *fieldname, *pattern, *replace;
{
	register rc;

	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( TLBADID );
	if( !entry || !fieldname ) return( TLARGS );
	if( TLe_diffformat( tid, entry ) ) return( TLDIFFFORMAT );

	if( (rc = TLs_replace( tid, entry, fieldname, pattern, replace )) == TLOK )
		TLtables[ tid ].status |= MODIFIED;
	return( rc );
}
