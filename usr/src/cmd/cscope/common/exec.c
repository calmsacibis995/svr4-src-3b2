/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cscope:common/exec.c	1.4"
/*	cscope - interactive C symbol cross-reference
 *
 *	process execution functions
 */

#include "global.h"
#include <varargs.h>

#if !BSD
#define getdtablesize()	_NFILE
#endif	

static	SIGTYPE	(*oldsigquit)();	/* old value of quit signal */
static	SIGTYPE	(*oldsighup)();		/* old value of hangup signal */

/* execute forks and executes a program or shell script, waits for it to
 * finish, and returns its exit code.
 */

/*VARARGS1*/
execute(a, va_alist)	/* note: "exec" is already defined on u370 */
char	*a;
va_dcl
{
	va_list	ap;
	int	exitcode;
	pid_t	p;
	pid_t	myfork();

	/* fork and exec the program or shell script */
	endwin();	/* restore the terminal modes */
	mousecleanup();
	fflush(stdout);
	if ((p = myfork()) == 0) {
		va_start(ap);
		myexecvp(a, ap);	/* child */
		va_end(ap);
	}
	else {
		exitcode = join(p);	/* parent */
	}
	/* the menu and scrollbar may be changed by the command executed */
#if UNIXPC || !TERMINFO
	nonl();
	cbreak();	/* endwin() turns off cbreak mode so restore it */
	noecho();
#endif
	mousemenu();
	drawscrollbar(topline, nextline);
	return(exitcode);
}

/* myexecvp is an interface to the execvp system call to
 * close all files except stdin, stdout, and stderr; and
 * modify argv[0] to reference the last component of its path-name.
 */

myexecvp(a, args)
char	*a;
va_list	args;
{
	register char	**argv;
	register int	i;
	char	msg[MSGLEN + 1];
	
	/* close files */
	for (i = 3; i < getdtablesize() && close(i) == 0; ++i) {
		;
	}
	/* modify argv[0] to reference the last component of its path name */
	argv = (char **) args;
	argv[0] = basename(argv[0]);

	/* execute the program or shell script */
	execvp(a, argv);	/* returns only on failure */
	(void) sprintf(msg, "\nCannot exec %s", a);
	(void) perror(msg);		/* display the reason */
	askforreturn();		/* wait until the user sees the message */
	exit(1);		/* exit the child */
	/* NOTREACHED */
}

/* myfork acts like fork but also handles signals */

pid_t
myfork() 
{
	pid_t	p;		/* process number */

	p = fork();
	
	/* the parent ignores the interrupt, quit, and hangup signals */
	if (p > 0) {
		oldsigquit = signal(SIGQUIT, SIG_IGN);
		oldsighup = signal(SIGHUP, SIG_IGN);
	}
	/* so they can be used to stop the child */
	else if (p == 0) {
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
	}
	/* check for fork failure */
	if (p == -1) {
		myperror("Cannot fork");
	}
	return p;
}

/* join is the compliment of fork */

join(p) 
pid_t	p; 
{
	int	status;  
	pid_t	w;

	/* wait for the correct child to exit */
	do {
		w = wait(&status);
	} while (p != -1 && w != p);

	/* restore signal handling */
	signal(SIGQUIT, oldsigquit);
	signal(SIGHUP, oldsighup);

	/* return the child's exit code */
	return(status >> 8);
}
