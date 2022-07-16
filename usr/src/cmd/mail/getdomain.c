/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:getdomain.c	1.5"
#ident "@(#)getdomain.c	1.3 'attmail mail(1) command'"
#include "mail.h"
#ifdef SVR4
# include <sys/utsname.h>
# include <sys/systeminfo.h>
#endif

/*
    NAME
	maildomain() - retrieve the domain name

    SYNOPSIS
	char *maildomain()

    DESCRIPTION
	Retrieve the domain name from xgetenv("DOMAIN").
	If that is not set, use sysinfo(SI_SRPC_DOMAIN) from
	-lnsl. Otherwise, return the null string.
*/

char *maildomain()
{
    static char *domain = 0;
#ifdef SVR4
    static char dombuf[SYS_NMLN];
#endif
    
    if (domain != 0)
	return domain;

    if ((domain = xgetenv("DOMAIN")) != 0)
	return domain;

#ifdef SVR4
    if (sysinfo(SI_SRPC_DOMAIN, dombuf, sizeof(dombuf)) >= 0)
	return (domain = dombuf);
#endif

    return (domain = "");
}
