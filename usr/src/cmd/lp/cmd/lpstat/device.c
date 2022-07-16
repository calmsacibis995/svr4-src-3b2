/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpstat/device.c	1.6"

#include "sys/types.h"
#include "string.h"

#include "lp.h"
#include "printers.h"

#define	WHO_AM_I	I_AM_LPSTAT
#include "oam.h"

#include "lpstat.h"


static void		putdline();

/**
 ** do_device()
 **/

void			do_device (list)
	char			**list;
{
	register PRINTER	*pp;


	while (*list) {
		if (STREQU(NAME_ALL, *list))
			while ((pp = getprinter(NAME_ALL)))
				putdline (pp);

		else if ((pp = getprinter(*list)))
			putdline (pp);

		else {
extern unsigned long badprinter;
printf ("badprinter is %#6.4x\n", badprinter);
			LP_ERRMSG1 (ERROR, E_LP_NOPRINTER, *list);
			exit_rc = 1;
		}

		list++;
	}
	return;
}

static void		putdline (pp)
	register PRINTER	*pp;
{
	if (!pp->device && !pp->dial_info && !pp->remote) {
		LP_ERRMSG1 (ERROR, E_LP_PGONE, pp->name);

	} else if (pp->remote) {
		char *			cp = strchr(pp->remote, BANG_C);


		if (cp)
			*cp++ = 0;
		(void)printf ("system for %s: %s", pp->name, pp->remote);
		if (cp)
			(void)printf (" (as printer %s)", cp);
		(void)printf ("\n");

	} else if (pp->dial_info) {
		(void)printf ("dial token for %s: %s", pp->name, pp->dial_info);
		if (pp->device)
			(void)printf (" (on port %s)", pp->device);
		(void)printf ("\n");

	} else
		(void)printf ("device for %s: %s\n", pp->name, pp->device);

	return;
}
