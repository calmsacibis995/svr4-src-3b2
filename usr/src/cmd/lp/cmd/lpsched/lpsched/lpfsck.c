/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:cmd/lpsched/lpsched/lpfsck.c	1.3.1.2"

#if	defined(__STDC__)
#include "stdarg.h"
#else
#include "varargs.h"
#endif

#include "stdlib.h"
#include "fcntl.h"

#include "lpsched.h"


#if	defined(UNIT_TEST)

#include "sys/utsname.h"

static char		*Local_System;

main ()
{
	struct utsname		utsbuf;

	uname (&utsbuf);
	Local_System = utsbuf.nodename;

	lpfsck();
}

#undef	Stat
#undef	Symlink
#undef	Unlink
#undef	Mkdir
#undef	Close
#undef	Creat
#undef	Chmod
#undef	Chown

#define	Stat		stat
#define	Symlink(A,B)	printf("symlink(%s, %s)\n", A, B)
#define Unlink(A)	printf("unlink(%s)\n", A)
#define	Mkdir(A,M)	printf("mkdir(%s, %04o)\n", A, M)
#define Close(X)	X
#define Creat(A,M)	printf("creat(%s, %04o)\n", A, M)
#define Chmod(A,M)	printf("chmod(%s, %04o)\n", A, M)
#define Chown(A,U,G)	printf("chown(%s, %d, %d)\n", A, U, G)
#define schedlog	printf
#define fail()		exit(1)

#define Lp_Uid		98
#define Lp_Gid		99
#endif

#define	F	0
#define D	1
#define P	2
#define S	3

#if	defined(__STDC__)
static void		proto (int, int, ...);
static char *		va_makepath(va_list *);
#else
static void		proto();
static char *		va_makepath();
#endif

void
#if	defined(__STDC__)
lpfsck (
	void
)
#else
lpfsck ()
#endif
{
	char *			cmd;

	/*
	 * These lines should match what is in the prototype file
	 * for the packaging! (In fact, it probably ought to be
	 * generated from that file, but that work is for a rainy day...)
	 */

	/*
	 * DIRECTORIES:
	 */
proto (D, 0,  Lp_A, NULL,			    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_A_Classes, NULL,		    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_A_Forms, NULL,			    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_A_Interfaces, NULL,		    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_A_Printers, NULL,		    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_A_PrintWheels, NULL,		    0775, Lp_Uid, Lp_Gid);
proto (D, 0,  "/var/lp", NULL,			    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Logs, NULL,			    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Spooldir, NULL,		    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Admins, NULL,			    0775, Lp_Uid, Lp_Gid);
proto (D, 0,  Lp_Spooldir, FIFOSDIR, NULL,	    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Private_FIFOs, NULL,		    0771, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Public_FIFOs, NULL,		    0773, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Requests, NULL,		    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Requests, Local_System, NULL,	    0770, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_System, NULL,			    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Tmp, NULL,			    0771, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Tmp, ".printer", NULL,		    0770, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Tmp, ".form", NULL,		    0770, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Tmp, ".pwheel", NULL,		    0770, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_Tmp, Local_System, NULL,	    0775, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_NetTmp, NULL,			    0770, Lp_Uid, Lp_Gid);

	/*
	 * The lpNet <-> lpsched job transfer directories.
	 * Strictly used for temporary file transfer, on start-up
	 * we can safely clean them out. Indeed, we should clean
	 * them out in case we had died suddenly and are now
	 * restarting. The directories should never be very big,
	 * so we are not in danger of getting ``arglist too big''.
	 */
proto (D, 1,  Lp_NetTmp, "tmp", NULL,		    0770, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_NetTmp, "tmp", Local_System, NULL, 0770, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_NetTmp, "requests", NULL,	    0770, Lp_Uid, Lp_Gid);
proto (D, 1,  Lp_NetTmp, "requests", Local_System, NULL, 0770, Lp_Uid, Lp_Gid);
	cmd = makestr(RMCMD, " ", Lp_NetTmp, "/tmp/*/*", (char *)0);
	system (cmd);
	free (cmd);
	cmd = makestr(RMCMD, " ", Lp_NetTmp, "/requests/*/*", (char *)0);
	system (cmd);
	free (cmd);

	/*
	 * THE MAIN FIFO:
	 */
proto (P, 1,  Lp_FIFO, NULL,			    0660, Lp_Uid, Lp_Gid);

	/*
	 * SYMBOLIC LINKS:
	 * Watch out! These names are given in the reverse
	 * order found in the prototype file (sorry!)
	 */
proto (S, 1,  Lp_Logs, NULL,			"/etc/lp/logs", NULL);
proto (S, 1,  Lp_Tmp, Local_System, NULL,	Lp_Temp, NULL);
proto (S, 1,  Lp_Bin, NULL,			Lp_Spooldir, "bin", NULL);
proto (S, 1,  Lp_A, NULL,			Lp_Admins, "lp", NULL);

	/*
	 * OTHER FILES:
	 */
proto (F, 1,  Lp_NetData, NULL,			    0664, Lp_Uid, Lp_Gid);

return;
}

static void
#if	defined(__STDC__)
proto (
	int			type,
	int			rm_ok,
	...
)
#else
proto (type, rm_ok, va_alist)
	int			type,
				rm_ok;
	va_dcl
#endif
{
	va_list			ap;

	char			*path,
				*symbolic;

	int			exist;

	mode_t			mode;

	uid_t			uid;

	gid_t			gid;

	struct stat		stbuf;


#if	defined(__STDC__)
	va_start (ap, type);
#else
	va_start (ap);
#endif

	path = va_makepath(&ap);

	exist = (stat(path, &stbuf) == 0);

	switch (type) {

	case S:
		if (!exist)
			fail ("%s is missing!\n", path);
		symbolic = va_makepath(&ap);
		Symlink (path, symbolic);
		free (symbolic);
		free (path);
		return;

	case D:
		if (exist && (stbuf.st_mode & S_IFDIR) == 0)
			if (!rm_ok)
				fail ("%s is not a directory!\n", path);
			else {
				Unlink (path);
				exist = 0;
			}
		if (!exist)
			Mkdir (path, 0);
		break;

	case F:
		if (exist && (stbuf.st_mode & S_IFREG) == 0)
			if (!rm_ok)
				fail ("%s is not a file!\n", path);
			else {
				Unlink (path);
				exist = 0;
			}
		if (!exist)
			Close(Creat(path, 0));
		break;

	case P:
		/*
		 * Either a pipe or a file.
		 */
		if (exist && (stbuf.st_mode & (S_IFREG|S_IFIFO)) == 0)
			if (!rm_ok)
				fail ("%s is not a file or pipe!\n", path);
			else {
				Unlink (path);
				exist = 0;
			}
		if (!exist)
			Close(Creat(path, 0));
		break;

	}

	mode = va_arg(ap, mode_t);
	uid = va_arg(ap, uid_t);
	gid = va_arg(ap, gid_t);
	Chmod (path, mode);
	Chown (path, uid, gid);

	free (path);
	return;
}

static char *
#if	defined(__STDC__)
va_makepath (
	va_list *		pap
)
#else
va_makepath (pap)
	va_list			*pap;
#endif
{
	va_list			ap_start	= *pap;

	char			*component,
				*p,
				*q;

	int			len;

	char			*ret;


	for (len = 0; (component = va_arg((*pap), char *)); )
		len += strlen(component) + 1;

	if (!len) {
		errno = 0;
		return (0);
	}

	if (!(ret = malloc(len))) {
		errno = ENOMEM;
		return (0);
	}

	*pap = ap_start;
	for (p = ret; (component = va_arg((*pap), char *)); ) {
		for (q = component; *q; )
			*p++ = *q++;
		*p++ = '/';
	}
	p[-1] = 0;

	return (ret);
}
