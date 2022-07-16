/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/incl/pw.h	1.1"
/*ident	"@(#)cfront:incl/pw.h	1.5"  */ 

#ifndef PWH
#define PWH

extern char* logname(),
             regcmp (const char* ...), 
             regex (const char*, const char* ...);

#endif
