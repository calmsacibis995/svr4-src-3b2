/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head:pw.h	1.2"

#if defined(__STDC__)

extern char *logname(void);
extern char *regcmp(const char *, ... );
extern char *regex(const char *, const char *, ...);
#else
extern char *logname();
extern char *regcmp();
extern char *regex();

#endif
