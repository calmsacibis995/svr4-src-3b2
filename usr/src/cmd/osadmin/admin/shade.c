/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osadmin:admin/shade.c	1.2"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	This version of shade.c was "hacked" from the original version
 *	for use with the new oam interface.  The "hack" is to simply
 *	have shade.c check to make sure the euid is the same as the
 *	sysadm command BEFORE it (1) alters the environment,
 *	(2) changes the current working directory, and (3)
 *	exec's the "/usr/admin/sysadm" script.  This will afford a 
 *	measure of security against someone simply executing the script
 *	and getting into whatever is in old simple administration
 *	The idea is that this should NEVER be invoked except through
 *	the new sysadm command, so permissions will have already been
 *	checked there.  At this point, if the euid doesn't match the
 *	sysadm command, then someone is trying to get in via other means.
 */


/* for OAM 18 ONLY!!! - change all NSYSADM refs to SYSADM in next load */
/* next load is when old sysadm really goes away  */

# define	NSYSADM	"nsysadm"
# define	SYSADM	"sysadm"
# define	SYSADMDIR	"/usr/admin"

# include	<string.h>
# include	<sys/types.h>
# include	<stdio.h>
# include	<pwd.h>

struct	passwd	*getpwnam();
struct	passwd	*userp;

char	Lname[BUFSIZ];
char	*lnamep = &Lname[0];
char	Command[BUFSIZ];
char	*commandp = &Command[0];
char	Home[BUFSIZ];
char	*homep = &Home[0];
char	Aptr[BUFSIZ];
char	*ap = &Aptr[0];

/*
	The following three assignments are needed for the sysadm
	commands.  They are used in envalt().
*/

char	pt[BUFSIZ] = "PATH=/bin:/usr/bin:/usr/lbin";
char	mt[BUFSIZ] = "MENUTOP=/usr/admin";
char	sh[BUFSIZ] = "SHELL=/bin/sh";

char	*getenv();

main(argc,argv)
int	argc;
char	*argv[];
{
	char	*nlnamep;


	/*
	  Get information about the "sysadm" command from
	  the /etc/passwd file.
	*/
	nlnamep = SYSADM;

	if ((userp = getpwnam(nlnamep)) == NULL) {
		fprintf(stderr,"Cannot find `%s' in password file\n",
			nlnamep);
		exit(1);
	}


	/* save any argument */
	if (argv[1]) strcat(ap,argv[1]);

	/*
	  If the user's effective uid and the sysadm command uid from 
	  /etc/passwd don't match, deny execution permission.
	*/

	if (geteuid() != userp->pw_uid) {
		fprintf(stderr, "Permission denied\n");
		exit(1);
	}

	/*
	 * If user has PATH, SHELL, MENUTOP, or HOME variable in their
	 * environment, change its value. If not, add it to the user's
	 * environment.  If either of the above fail, an error message
	 * printed.
	 */

	envalt();

	/*
	   Build the command string that "exec" will execute 
	   when called.
	*/

	sprintf(commandp,"%s/%s",SYSADMDIR, SYSADM);

	/*
	   change current working directory to the simple admin
	   home directory in order for certain commands to work
	*/

	if (chdir(SYSADMDIR) != 0) {
		fprintf(stderr,"Unable to change directory to \"%s\"\n",
			SYSADMDIR);
		exit(1);
	}

	if (*ap)
		execl("/bin/sh", "/bin/sh", commandp, ap, 0);
	else
		execl("/bin/sh", "/bin/sh", commandp, 0);
}


envalt()
{
	int putenv();

	/*
	   setup "home" variable for use later.
	*/
	sprintf(homep,"HOME=%s",userp->pw_dir);

	if ((putenv(pt)) != 0)
		env_err();

	if ((putenv(sh)) != 0)
		env_err();

	if ((putenv(mt)) != 0)
		env_err();

	if ((putenv(homep)) != 0)
		env_err();
	return;
}


env_err()
{
	fprintf(stderr,"unable to obtain memory to expand environment\n");
		exit(1);
}
