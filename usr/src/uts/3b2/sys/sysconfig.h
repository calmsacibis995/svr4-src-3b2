/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.sys:sys/sysconfig.h	1.2"

/* cmd values for _sysconfig system call. 
** WARNING: This is an undocumented system call,
** therefore future compatibility can not
** guaranteed. 
*/ 

#define	_CONFIG_CHOWN_RST 1	/* chown restricted enabled */
#define _CONFIG_NGROUPS 2	/* number of configured supplemental groups */
#define _CONFIG_CHILD_MAX 3	/* max # of processes per uid session */
#define _CONFIG_OPEN_FILES 4	/* max # of open files per process */
#define _CONFIG_POSIX_VER 5	/* POSIX version */
#define _CONFIG_PAGESIZE 6	/* system page size */
