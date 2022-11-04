/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libadm:ckint.c	1.1"

#include <stdio.h>
#include <string.h>

extern int	ckquit;

extern long	strtol();
extern void	puterror(), 
		puthelp(),
		putprmpt();
extern int	getinput();

static void
setmsg(msg, base)
char	*msg;
short	base;
{
	if((base == 0) || (base == 10))
		(void) sprintf(msg, "Please enter an integer.");
	else
		(void) sprintf(msg, "Please enter a base %d integer.", base);
}

static void
setprmpt(prmpt, base)
char	*prmpt;
short	base;
{
	if((base == 0) || (base == 10))
		(void) sprintf(prmpt, "Enter an integer.");
	else
		(void) sprintf(prmpt, "Enter a base %d integer.", base);
}

int
ckint_val(value, base)
char	*value;
short	base;
{
	char	*ptr;

	(void) strtol(value, &ptr, base);
	return(*ptr != '\0');
}

void
ckint_err(base, error)
short	base;
char	*error;
{
	char	defmesg[64];

	setmsg(defmesg, base);
	puterror(stderr, defmesg, error);
}

void
ckint_hlp(base, help)
short	base;
char	*help;
{
	char	defmesg[64];

	setmsg(defmesg, base);
	puthelp(stderr, defmesg, help);
}
	
int
ckint(intval, base, defstr, error, help, prompt)
long	*intval;
short	base;
char	*defstr, *error, *help, *prompt;
{
	long	value;
	char	*ptr,
		input[128],
		defmesg[64],
		temp[64];

	if(!prompt) {
		setprmpt(temp, base);
		prompt = temp;
	}
	setmsg(defmesg, base);

start:
	putprmpt(stderr, prompt, NULL, defstr);
	if(getinput(input))
		return(1);

	if(strlen(input) == 0) {
		if(defstr) {
			*intval = strtol(defstr, NULL, base);
			return(0);
		}
		puterror(stderr, defmesg, error);
		goto start;
	} else if(!strcmp(input, "?")) {
		puthelp(stderr, defmesg, help);
		goto start;
	} else if(ckquit && !strcmp(input, "q"))
		return(3);

	value = strtol(input, &ptr, base);
	if(*ptr != '\0') {
		puterror(stderr, defmesg, error);
		goto start;
	}
	*intval = value;
	return(0);
}
