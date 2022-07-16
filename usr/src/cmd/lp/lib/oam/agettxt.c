/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/oam/agettxt.c	1.5"
/* LINTLIBRARY */

#include "oam.h"

char			**_oam_msg_base_	= 0;

char *
#if	defined(__STDC__)
agettxt (
	long			msg_id,
	char *			buf,
	int			buflen
)
#else
agettxt (msg_id, buf, buflen)
	long			msg_id;
	char			*buf;
	int			buflen;
#endif
{
	if (_oam_msg_base_)
		strncpy (buf, _oam_msg_base_[msg_id], buflen-1);
	else
		strncpy (buf, "No message defined--get help!", buflen-1);
	buf[buflen-1] = 0;
	return (buf);
}
