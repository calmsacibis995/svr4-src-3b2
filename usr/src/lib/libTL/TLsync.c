/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libTL:TLsync.c	1.2"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];

int
TLsync( tid )
int tid;
{
	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( TLBADID );

	/* Write out new table */
	return( TLt_sync( tid ) );
}
