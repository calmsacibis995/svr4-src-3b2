/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/mkdead.c	1.4"
#ident "@(#)mkdead.c	2.6 'attmail mail(1) command'"
#include "mail.h"
/*
	Routine creates dead.letter
*/
void mkdead()
{
	static char pn[] = "mkdead";
	int aret;
	malf = 0;

	/*
		Try to create dead letter in current directory
		or in home directory
	*/
	umask(umsave);
	if ((aret = legal(&dead[1]))) malf = fopen(&dead[1], "a");
	if ((malf == NULL) || (aret == 0)) {
		/*
			try to create in $HOME
		*/
		if((hmdead = malloc(strlen(home) + strlen(dead) + 1)) == NULL) {
			fprintf(stderr, "%s: Can't malloc\n",program);
			Dout(pn, 0, "Cannot malloc.\n");
			goto out;
		}
		cat(hmdead, home, dead);
		if ((aret=legal(hmdead))) malf = fopen(hmdead, "a");
		if ((malf == NULL) || (aret == 0)) {
			fprintf(stderr,
				"%s: Cannot create %s\n",
				program,&dead[1]);
			Dout(pn, 0, "Cannot create %s\n", &dead[1]);
		out:
			fclose(tmpf);
			error = E_FILE;
			Dout(pn, 0, "error set to %d\n", error);
			umask(7);
			return;
		}  else {
			chmod(hmdead, DEADPERM);
			fprintf(stderr,"%s: Mail saved in %s\n",program,hmdead);
		}
	} else {
		chmod(&dead[1], DEADPERM);
		fprintf(stderr,"%s: Mail saved in %s\n",program,&dead[1]);
	}

	/*
		Copy letter into dead letter box
	*/
	umask(7);
	rewind(tmpf);
	if (!copystream(tmpf, malf)) {
		errmsg(E_DEAD,"");
	}
	fclose(malf);
}

void savdead()
{
	static char pn[] = "savdead";
	setsig(SIGINT, saveint);
	dflag = 2;	/* do not send back letter on interrupt */
	Dout(pn, 0, "dflag set to 2\n");
	if (!error) {
		error = E_REMOTE;
		Dout(pn, 0, "error set to %d\n", error);
	}
	maxerr = error;
	Dout(pn, 0, "maxerr set to %d\n", maxerr);
}
