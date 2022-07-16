/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/oam/llib-llpoam	1.8"
/*LINTLIBRARY*/

#include "oam.h"

/*	from file agettxt.c */

char * agettxt ( long msg_id, char *buf, int buflen)
{
    static char			* _returned_value;
    return _returned_value;
}

/*	from file fmtmsg.c */

/**
 ** fmtmsg()
 **/
int fmtmsg ( long classification, char *label, int severity, char *text,
	     char *tag, char *action)
{
    static int			 _returned_value;
    return _returned_value;
}
