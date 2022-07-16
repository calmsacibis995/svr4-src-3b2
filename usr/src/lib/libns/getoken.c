/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:getoken.c	1.3.2.1"

#include "nserve.h"
#include "sys/types.h"
#include "sys/rf_cirmgr.h"
#include <stdio.h>
#include "nslog.h"

int
getoken(s)
	struct rf_token *s;
{
	char *sp;

	LOG2(L_TRACE, "(%5d) enter: getoken\n", Logstamp);
	s->t_id = 0;
	sp = (char *)&s->t_uname[0];
	LOG2(L_TRACE, "(%5d) leave: getoken\n", Logstamp);
	return netname(sp);
}
