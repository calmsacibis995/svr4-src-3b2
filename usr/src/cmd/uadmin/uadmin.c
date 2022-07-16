/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uadmin:uadmin.c	1.4"

#include <stdio.h>
#include <signal.h>
#include <sys/uadmin.h>

char *Usage = "Usage: %s cmd fcn\n";

main(argc, argv)
char *argv[];
{
	register cmd, fcn;
	sigset_t set, oset;


	if (argc != 3) {
		fprintf(stderr, Usage, argv[0]);
		exit(1);
	}

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &oset);

	cmd = atoi(argv[1]);
	fcn = atoi(argv[2]);

	if (uadmin(cmd, fcn, 0) < 0)
		perror("uadmin");

	sigprocmask(SIG_BLOCK, &oset, (sigset_t *)0);

}
