/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libTL:TLassign.c	1.2"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];

int
TLassign( tid, entry, fieldname, value )
int tid;
entry_t *entry;
unsigned char *fieldname, *value;
{
	register rc;

	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( TLBADID );
	if( !entry || !fieldname ) return( TLARGS );
	if( TLe_diffformat( tid, entry ) ) return( TLDIFFFORMAT );
	if( (rc = TLf_assign( tid, entry, fieldname, value ) ) == TLOK );
		entry->status |= IS_PARSED;
	return( rc );
}
