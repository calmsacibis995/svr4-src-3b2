/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/lib/static/_dtor.c	1.1"
/*ident	"@(#)cfront:lib/static/_dtor.c	1.3" */
typedef void (*PFV)();
extern PFV _dtors[] = { 0 };
