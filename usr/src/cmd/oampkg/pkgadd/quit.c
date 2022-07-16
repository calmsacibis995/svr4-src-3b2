/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:pkgadd/quit.c	1.8.1.1"

#include <stdio.h>
#include <signal.h>
#include <pkgdev.h>

extern struct pkgdev
		pkgdev;
extern int	intfchg,
		npkgs,
		failflag,
		warnflag,
		intrflag,
		admnflag,
		nullflag,
		reboot, ireboot;

extern void	exit(),
		echo(),
		ptext(),
		intf_reloc(),
		ckreturn();
extern int	pkgumount(),
		rrmdir(),
		chdir();

#define MSG_REBOOT \
"\\n*** IMPORTANT NOTICE ***\\n\
\\tThis machine must now be rebooted in order to ensure\\n\
\\tsane operation.  Execute\\n\\t\\tshutdown -y -i6 -g0\\n\
\\tand wait for the \"Console Login:\" prompt."

void		trap(), quit();

void
trap(signo)
int signo;
{
	if((signo == SIGINT) || (signo == SIGHUP))
		quit(3);
	else
		quit(1);

}

void
quit(retcode)
int retcode;
{
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);

	if(retcode != 99) {
		ckreturn(retcode);
		if(failflag)
			retcode = 1;
		else if(warnflag)
			retcode = 2;
		else if(intrflag)
			retcode = 3;
		else if(admnflag)
			retcode = 4;
		else if(nullflag)
			retcode = 5;
		else
			retcode = 0;
		if(ireboot)
			retcode = (retcode % 10) + 20;
		if(reboot)
			retcode = (retcode % 10) + 10;
	}

	if(reboot || ireboot)
		ptext(stderr, MSG_REBOOT);

	(void) chdir("/");
	if(pkgdev.mount)
		(void) pkgumount(&pkgdev);
	else if(pkgdev.cdevice)
		(void) rrmdir(pkgdev.dirname);  /* from tempnam */

	if(npkgs == 1)
		echo("\n1 package was not processed!\n");
	else if(npkgs)
		echo("\n%d packages were not processed!\n", npkgs);

	if(intfchg)
		intf_reloc();

	exit(retcode);
}
