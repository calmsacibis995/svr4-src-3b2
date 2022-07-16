/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ttymon:tmsig.c	1.7"

#include	<signal.h>

extern	char	Scratch[];
extern	void	log();

/*
 * catch_signals:
 *	ttymon catch some signals and ignore the rest.
 *
 *	SIGTERM	- killed by somebody
 *	SIGPOLL - got message on pmpipe, probably from sac
 *			   or on PCpipe
 *	SIGCLD	- tmchild died
 */
void
catch_signals()
{
	register int i;
	extern void sigterm();
	extern void sigchild();
	extern void sigpoll_catch();
#ifdef	DEBUG
	extern void dump_pmtab();
	extern void dump_ttydefs();
	extern void debug();

	debug("in catch_signals");
#endif

	for (i = 1; i < SIGKILL; i++)
		(void)signal(i, SIG_IGN);
	for (i = SIGKILL + 1; i < SIGTERM; i++)
		(void)signal(i, SIG_IGN);
	(void)sigset(SIGTERM,	sigterm);
#ifdef	DEBUG
	(void)sigset(SIGUSR1,	dump_pmtab);
	(void)sigset(SIGUSR2,	dump_ttydefs);
#else
	(void)signal(SIGUSR1,	SIG_IGN);
	(void)signal(SIGUSR2,	SIG_IGN);
#endif
	(void)signal(SIGCLD,	sigchild);
	for (i = SIGCLD + 1; i < SIGPOLL; i++)
		(void)signal(i, SIG_IGN);
	(void)sigset(SIGPOLL,	sigpoll_catch);
	for (i = SIGPOLL + 1; i < NSIG; i++)
		(void)signal(i, SIG_IGN);
	/* SIGWIND and SIGPHONE only on UNIX PC */
}

/*
 * child_sigcatch() - tmchild inherits some signal_catch from parent
 *		      and need to reset them
 */
void
child_sigcatch()
{
	int	i;
	extern	void	sigpoll();

	(void)sigset(SIGHUP, SIG_DFL);
	for (i = SIGINT + 1; i < SIGKILL; i++)
		(void)signal(i, SIG_DFL);
	for (i = SIGKILL + 1; i < SIGPOLL; i++)
		(void)signal(i, SIG_DFL);
	(void)sigset(SIGPOLL, sigpoll);
	for (i = SIGPOLL + 1; i < NSIG; i++)
		(void)sigset(i, SIG_DFL);
	/* temp fix */
	(void)sigset(SIGXFSZ, SIG_IGN);
}
