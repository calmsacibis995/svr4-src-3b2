/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#ident	"@(#)head.sys:sys/utsname.h	11.10"

/*
 * If you are compiling the kernel, the value used in initializing
 * the utsname structure in the master.d/kernel file better be the
 * same as SYS_NMLN.
 */
#if !defined(_STYPES)
#define SYS_NMLN	257	/* 4.0 size of utsname elements.*/
				/* Must be at least 257 to 	*/
				/* support Internet hostnames.  */
#else
#define SYS_NMLN	9	/* old size of utsname elements */
#endif	/* _STYPES */

struct utsname {
	char	sysname[SYS_NMLN];
	char	nodename[SYS_NMLN];
	char	release[SYS_NMLN];
	char	version[SYS_NMLN];
	char	machine[SYS_NMLN];
};

extern struct utsname utsname;

#if !defined(_KERNEL)
#if defined(__STDC__)
int uname(struct utsname *);
int nuname(struct utsname *);
#else
int uname();
int nuname();
#endif	/* !(KERNEL) */
#endif	/* (__STDC__) */

#if !defined(_KERNEL) && !defined(_STYPES)
static int
uname(buf)
struct utsname *buf;
{
	int ret;

	ret = nuname(buf);
	return ret;
}
#endif /* !defined(_KERNEL) && !defined(_STYPES) */

#endif	/* _SYS_UTSNAME_H */
