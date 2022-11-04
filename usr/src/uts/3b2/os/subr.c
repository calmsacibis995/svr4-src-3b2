/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/subr.c	1.37.1.5"

#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/vfs.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/kmem.h"
#include "sys/user.h"
#include "sys/buf.h"
#include "sys/var.h"
#include "sys/conf.h"
#include "sys/debug.h"
#include "sys/proc.h"
#include "sys/acct.h"
#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/procfs.h"
#include "sys/dl.h"
#include "sys/cmn_err.h"
#include "sys/tuneable.h"

/*
 * Routine which sets a user error; placed in
 * illegal entries in the bdevsw and cdevsw tables.
 */

int
nodev()
{
	return (u.u_error = ENODEV);
}

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */

int
nulldev()
{
	return 0;
}

/*
 * Generate an unused major device number.
 */

#define NDEV 128
#define max(a, b)	((a) > (b) ? (a) : (b))

int
getudev()
{
	extern char MAJOR[];
	static int next = 0;
	int maxdevcnt = max(bdevcnt, cdevcnt);

	for ( ; next < NDEV; next++)
		if (MAJOR[next] >= maxdevcnt)
			return next++;
	return -1;
}

/*
 * Find a proc table pointer given a process id.  The code attempts
 * to optimize the case in which repeated calls return successive
 * proc table slots (as with ps(1)) by remembering where the search
 * left off the last time.  Interspersed calls can defeat the
 * optimization but ordinarily this won't happen.
 */
proc_t *
prfind(pid)
	register pid_t pid;
{

	unsigned int index;

	index = GET_INDEX(pid);
	if (index >= v.v_proc || nproc[index] == NULL || nproc[index]->p_stat ==0){
		return NULL;
	} else{
		return(nproc[index]);
	}
}

/*
 * C-library string functions.  Assembler versions of others are in
 * ml/string.s.
 */

/*
 * Copy s2 to s1, truncating or null-padding to always copy n bytes.
 * Return s1.
 */

char *
strncpy(s1, s2, n)
	register char *s1, *s2;
	register size_t n;
{
	register char *os1 = s1;

	n++;
	while (--n != 0 && (*s1++ = *s2++) != '\0')
		;
	if (n != 0)
		while (--n != 0)
			*s1++ = '\0';
	return os1;
}

/*
 * Compare strings (at most n bytes): return *s1-*s2 for the last
 * characters in s1 and s2 which were compared.
 */
int
strncmp(s1, s2, n)
	register char *s1, *s2;
	register size_t n;
{
	if (s1 == s2)
		return 0;
	n++;
	while (--n != 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return 0;
	return (n == 0) ? 0 : *s1 - *--s2;
}

/*
 * Compare two byte streams.  Returns 0 if they're identical, 1
 * if they're not.
 */
int
bcmp(s1, s2, len)
	register char *s1, *s2;
	register size_t len;
{
	while (len--)
		if (*s1++ != *s2++)
			return 1;
	return 0;
}

int
intrerr(v)
{
	register proc_t	*prp;

	if (!v)
		return 0;

	prp = u.u_procp;
	if (prp->p_cursig) {
		if (sigismember(&u.u_sigrestart, prp->p_cursig)) {
			return ERESTART;
		}
	} 
	 else if (ev_intr_restart(u.u_procp)) {
		return ERESTART;
	}

	return EINTR;
}

int
memlow()
{
	return freemem <= tune.t_gpgslo;
}

/* takes a numeric char, yields an int */
#define	CTOI(c)		((c) & 0xf)
/* takes an int, yields an int */
#define TEN_TIMES(n)	(((n) << 3) + ((n) << 1))

/*
 * Returns the integer value of the string of decimal numeric
 * chars beginning at **str.
 * Does no overflow checking.
 * Note: updates *str to point at the last character examined.
 */
int
stoi(str)
	register char	**str;
{
	register char	*p = *str;
	register int	n;
	register int	c;

	for (n = 0; (c = *p) >= '0' && c <= '9'; p++) {
		n = TEN_TIMES(n) + CTOI(c);
	}
	*str = p;
	return n;
}

int
rlimit(resource, softlimit, hardlimit)
	int resource;
	rlim_t softlimit, hardlimit;
{
	if (softlimit > hardlimit)
		return EINVAL;

	if (hardlimit > u.u_rlimit[resource].rlim_max && !suser(u.u_cred))
		return EPERM;

	u.u_rlimit[resource].rlim_cur = softlimit;
	u.u_rlimit[resource].rlim_max = hardlimit;

	return 0;
}
