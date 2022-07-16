/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libTL:space.c	1.3"
#include <table.h>
#include <internal.h>
#include <setjmp.h>

tbl_t TLtables[ TL_MAXTABLES ];
int TLinitialized = 0;
unsigned char TLinbuffer[ 2 * TL_MAXLINESZ + 1 ];
unsigned char TLgenbuf[ 2 * TL_MAXLINESZ + 1 ];
jmp_buf TLenv;
