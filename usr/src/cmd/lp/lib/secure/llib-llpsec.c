/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:lib/secure/llib-llpsec.c	1.2"

/* LINTLIBRARY */

# include	"secure.h"

/**
 ** getsecure() - EXTRACT SECURE REQUEST STRUCTURE FROM DISK FILE
 **/

SECURE * getsecure ( const char * file )
{
    static SECURE * _returned_value;
    return _returned_value;
}

/**
 ** putsecure() - WRITE SECURE REQUEST STRUCTURE TO DISK FILE
 **/

int putsecure ( const char * file, const SECURE * secbufp )
{
    static int _returned_value;
    return _returned_value;
}

void freesecure ( SECURE * secbufp )
{
}
