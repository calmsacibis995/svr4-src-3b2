/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:libbrmeth.d/brcontrol.c	1.3"

#include	<signal.h>
#include	<bkrs.h>

extern int brtype;
extern int bkcancel();
extern int bksuspend();

int
brcancel()
{
	switch( brtype ) {
	case BACKUP_T:
		return( bkcancel() );
	case RESTORE_T:
		return( BRSUCCESS );
	default:
		return( BRNOTINITIALIZED );
	}
}

int
brsuspend()
{
	switch( brtype ) {
	case BACKUP_T:
		return( bksuspend() );
	case RESTORE_T:
		return( BRSUCCESS );
	default:
		return( BRNOTINITIALIZED );
	}
}
